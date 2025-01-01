// This program demonstrates attaching an eBPF program to a kernel symbol.
// The eBPF program will be attached to the start of the sys_execve
// kernel function and prints out the number of times it has been called
// every second.
package main

import (
	"bufio"
	"encoding/binary"
	"context"
	"errors"
	"fmt"
	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/cilium/ebpf/rlimit"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/client"
	"github.com/vishvananda/netns"
	"log"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"syscall"
)

//go:generate go run github.com/cilium/ebpf/cmd/bpf2go -target amd64 -type packet_info bpf bpf.c

const (
	mapTcEgress   = 0x01
	mapTcIngress  = 0x02
	mapXdpEgress  = 0x03
	mapXdpIngress = 0x04
)

const (
	docker0Interface = 6
	mainInterface    = 4
)

var defaultKey = uint64(0)
var defaultPkt = bpfPacketInfo{
	Hash:       0,
	LenIfindex: 0,
	Timestamp:  0,
}

var containerInterfaceMapping map[int]string
var containerNameIDMapping map[string]string
var containerNSPathMapping map[string]string
var vethInterfaceMapping map[int]string

var bpfTcInterfaceMapping map[string]link.Link
var bpfXdpInterfaceMapping map[string]link.Link
var bpfSkbInterfaceMapping map[string]link.Link

// stat structure
type stat struct {
	timestamp     uint64
	packetHash    uint32
	ifIndex       uint32
	len           uint32
	containerName string
	containerID   string
	vethName      string
	hook          string
}

// scanNetworkInterfaces scans for veth interfaces and maps them to Docker containers
func scanNetworkInterfaces() {
	// Initialize Docker client
	cli, err := client.NewClientWithOpts(client.FromEnv)
	if err != nil {
		fmt.Printf("Error creating Docker client: %v\n", err)
		return
	}

	defer func(cli *client.Client) {
		err := cli.Close()
		if err != nil {
			return
		}
	}(cli)

	// Get running containers
	containers, err := cli.ContainerList(context.Background(), container.ListOptions{})
	if err != nil {
		fmt.Printf("Error listing containers: %v\n", err)
		return
	}

	// Read and parse /sys/class/net/veth*/ifindex for each veth interface
	files, err := filepath.Glob("/sys/class/net/veth*/ifindex")
	if err != nil {
		fmt.Printf("Error reading network interfaces: %v\n", err)
		return
	}

	for _, file := range files {
		// Get interface name from path
		hostVethName := filepath.Base(filepath.Dir(file))

		// Read interface index (host side)
		ifindexBytes, err := os.ReadFile(file)
		if err != nil {
			continue
		}
		ifIndexStr := strings.TrimSpace(string(ifindexBytes))
		hostIfIndex, err := strconv.Atoi(ifIndexStr)
		if err != nil {
			continue
		}

		// Read peer index from /sys/class/net/veth*/iflink
		peerIndexBytes, err := os.ReadFile(filepath.Join(filepath.Dir(file), "iflink"))
		if err != nil {
			continue
		}
		peerIndexStr := strings.TrimSpace(string(peerIndexBytes))
		containerIfIndex, err := strconv.Atoi(peerIndexStr)
		if err != nil {
			continue
		}

		// Store the host veth interface mapping immediately
		vethInterfaceMapping[hostIfIndex] = hostVethName

		// For each container, check its network namespace
		for _, ctr := range containers {
			// Get container PID
			inspect, err := cli.ContainerInspect(context.Background(), ctr.ID)
			if err != nil {
				continue
			}

			// Read container's interfaces from its network namespace
			nsPath := fmt.Sprintf("/proc/%d/ns/net", inspect.State.Pid)
			if _, err := os.Stat(nsPath); err != nil {
				continue
			}

			containerNSPathMapping[fmt.Sprintf("container:%s", ctr.Names[0])] = nsPath

			// Get ALL interfaces in the container
			cmdStr := fmt.Sprintf("nsenter --net=%s -- ip link show", nsPath)
			cmd := exec.Command("bash", "-c", cmdStr)
			output, err := cmd.Output()
			if err == nil {
				// Process all interfaces in the output
				lines := strings.Split(string(output), "\n")
				for i := 0; i < len(lines)-1; i += 2 { // ip link show outputs 2 lines per interface
					line := lines[i]
					if !strings.Contains(line, "@") {
						continue
					}

					// Parse interface index and name
					parts := strings.Split(line, ":")
					if len(parts) < 2 {
						continue
					}

					idxStr := strings.TrimSpace(parts[0])
					idx, err := strconv.Atoi(idxStr)
					if err != nil {
						continue
					}

					// Extract interface name
					nameParts := strings.Fields(strings.TrimSpace(parts[1]))
					if len(nameParts) < 1 {
						continue
					}
					name := strings.Split(nameParts[0], "@")[0]

					// If this is our target container interface
					if idx == containerIfIndex {
						// Store mappings for both interfaces
						vethInterfaceMapping[containerIfIndex] = name
						containerInterfaceMapping[hostIfIndex] = fmt.Sprintf("host:%s", ctr.Names[0])
						containerInterfaceMapping[containerIfIndex] = fmt.Sprintf("container:%s", ctr.Names[0])
						containerNameIDMapping[fmt.Sprintf("host:%s", ctr.Names[0])] = ctr.ID
						containerNameIDMapping[fmt.Sprintf("container:%s", ctr.Names[0])] = ctr.ID
						break
					}
				}
			}
		}
	}

	// Print results
	if len(containerInterfaceMapping) == 0 {
		fmt.Println("No Docker network interfaces (veth) were found.")
		return
	}

	fmt.Println("\nDocker Interface to Container Mapping:")
	fmt.Println("====================================")

	var indices []int
	for idx := range containerInterfaceMapping {
		indices = append(indices, idx)
	}
	sort.Ints(indices)

	for _, ifIndex := range indices {
		mapping := containerInterfaceMapping[ifIndex]
		parts := strings.SplitN(mapping, ":", 3)
		if len(parts) != 3 {
			continue
		}

		location := parts[0]
		containerName := parts[1]
		containerID := parts[2]
		ifname := vethInterfaceMapping[ifIndex]

		fmt.Printf("Interface %d (%s) -> Container %s %s (%s)\n",
			ifIndex,
			ifname,
			containerID[:9],
			containerName,
			location)
	}
}

// attachTcInterfaces function, will attach even inside a namespace
func attachTcInterfaces(objs *bpfObjects) error {
	// Lock OS thread since we'll be switching namespaces
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	// Store original namespace to return to it later
	originalNS, err := netns.Get()
	if err != nil {
		return fmt.Errorf("failed to get current network namespace: %v", err)
	}
	defer originalNS.Close()

	bpfTcInterfaceMapping = make(map[string]link.Link)
	partialFailure := false

	// Helper function to attach TC programs in a given namespace
	attachInNamespace := func(ifIdx int, name string, isIngress bool) error {
		var li link.Link
		var err error

		program := objs.TcIngressPktFunction
		attachType := ebpf.AttachTCXIngress
		logPrefix := "Ingress"

		if !isIngress {
			program = objs.TcEgressPktFunction
			attachType = ebpf.AttachTCXEgress
			logPrefix = "Egress"
		}

		// If it's a container interface, switch to its namespace before attaching
		if strings.Contains(name, "container") {
			nsPath := containerNSPathMapping[name]
			containerNS, err := netns.GetFromPath(nsPath)
			if err != nil {
				return fmt.Errorf("failed to get container namespace: %v", err)
			}
			defer containerNS.Close()

			// Switch to container's namespace
			if err := netns.Set(containerNS); err != nil {
				return fmt.Errorf("failed to switch to container namespace: %v", err)
			}

			// Attach TC program inside the container namespace
			log.Printf("[TC - %s] Attaching to %s(%d), %s(%s) in container namespace",
				logPrefix, vethInterfaceMapping[ifIdx], ifIdx, name, containerNameIDMapping[name])

			li, err = link.AttachTCX(link.TCXOptions{
				Interface: ifIdx,
				Program:   program,
				Attach:    attachType,
			})

			// Switch back to original namespace
			if err := netns.Set(originalNS); err != nil {
				return fmt.Errorf("failed to switch back to original namespace: %v", err)
			}
		} else {
			// Switch back to original namespace
			if err := netns.Set(originalNS); err != nil {
				return fmt.Errorf("failed to switch back to original namespace: %v", err)
			}

			// For non-container interfaces, attach in current namespace
			log.Printf("[TC - %s] Attaching to %s(%d), %s(%s)",
				logPrefix, vethInterfaceMapping[ifIdx], ifIdx, name, containerNameIDMapping[name])

			li, err = link.AttachTCX(link.TCXOptions{
				Interface: ifIdx,
				Program:   program,
				Attach:    attachType,
			})
		}

		if err != nil {
			return fmt.Errorf("error attaching TC %s interface: %v", strings.ToLower(logPrefix), err)
		}

		bpfTcInterfaceMapping[fmt.Sprintf("%d-%s", ifIdx, strings.ToLower(logPrefix))] = li
		return nil
	}

	// Attach ingress programs
	for ifIdx, name := range containerInterfaceMapping {
		if err := attachInNamespace(ifIdx, name, true); err != nil {
			log.Printf("[TC - Ingress] %v", err)
			partialFailure = true
		}
	}

	err = objs.TcIngressPktCount.Update(defaultKey, defaultPkt, ebpf.UpdateAny)
	if err != nil {
		log.Printf("[TC - Ingress] Failed to update default key: %v", err)
		return err
	}

	// Attach egress programs
	for ifIdx, name := range containerInterfaceMapping {
		if err := attachInNamespace(ifIdx, name, false); err != nil {
			log.Printf("[TC - Egress] %v", err)
			partialFailure = true
		}
	}

	err = objs.TcEgressPktCount.Update(defaultKey, defaultPkt, ebpf.UpdateAny)
	if err != nil {
		log.Printf("[TC - Egress] Failed to update default key: %v", err)
		return err
	}

	if partialFailure {
		return fmt.Errorf("failed to attach TC interfaces, partially failed")
	}

	return nil
}

// attachXdpInterfaces function
func attachXdpInterfaces(objs *bpfObjects) error {
	// Lock OS thread since we'll be switching namespaces
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	// Store original namespace to return to it later
	originalNS, err := netns.Get()
	if err != nil {
		return fmt.Errorf("failed to get current network namespace: %v", err)
	}
	defer originalNS.Close()

	bpfXdpInterfaceMapping = make(map[string]link.Link)
	partialFailure := false

	// Helper function to attach XDP program in a given namespace
	attachInNamespace := func(ifIdx int, name string) error {
		var li link.Link
		var err error

		// If it's a container interface, switch to its namespace before attaching
		if strings.Contains(name, "container") {
			nsPath := containerNSPathMapping[name]
			containerNS, err := netns.GetFromPath(nsPath)
			if err != nil {
				return fmt.Errorf("failed to get container namespace: %v", err)
			}
			defer containerNS.Close()

			// Switch to container's namespace
			if err := netns.Set(containerNS); err != nil {
				return fmt.Errorf("failed to switch to container namespace: %v", err)
			}

			// Attach XDP program inside the container namespace
			log.Printf("[XDP - Ingress] Attaching to %s(%d), %s(%s) in container namespace",
				vethInterfaceMapping[ifIdx], ifIdx, name, containerNameIDMapping[name])

			li, err = link.AttachXDP(link.XDPOptions{
				Interface: ifIdx,
				Program:   objs.XdpIngressPktFunction,
			})

			if err != nil {
				log.Printf("[XDP - Ingress] Failed attaching to %s(%d), %s(%s): %v",
					vethInterfaceMapping[ifIdx], ifIdx, name, containerNameIDMapping[name], err)
			}

			// Switch back to original namespace
			if err := netns.Set(originalNS); err != nil {
				return fmt.Errorf("failed to switch back to original namespace: %v", err)
			}
		} else {
			// Switch back to original namespace
			if err := netns.Set(originalNS); err != nil {
				return fmt.Errorf("failed to switch back to original namespace: %v", err)
			}

			// For non-container interfaces, attach in current namespace
			log.Printf("[XDP - Ingress] Attaching to %s(%d), %s(%s)",
				vethInterfaceMapping[ifIdx], ifIdx, name, containerNameIDMapping[name])

			li, err = link.AttachXDP(link.XDPOptions{
				Interface: ifIdx,
				Program:   objs.XdpIngressPktFunction,
			})

			if err != nil {
				log.Printf("[XDP - Ingress] Failed attaching to %s(%d), %s(%s): %v",
					vethInterfaceMapping[ifIdx], ifIdx, name, containerNameIDMapping[name], err)
			}
		}

		if err != nil {
			return fmt.Errorf("error attaching XDP interface: %v", err)
		}

		bpfXdpInterfaceMapping[fmt.Sprintf("%d-ingress", ifIdx)] = li
		return nil
	}

	// Attach XDP programs
	for ifIdx, name := range containerInterfaceMapping {
		if err := attachInNamespace(ifIdx, name); err != nil {
			log.Printf("[XDP] %v", err)
			partialFailure = true
		}
	}

	err = objs.XdpEgressPktCount.Update(defaultKey, defaultPkt, ebpf.UpdateAny)
	if err != nil {
		log.Printf("[XDP - Egress] Failed to update default key: %v", err)
		return err
	}

	if partialFailure {
		return fmt.Errorf("failed to attach XDP interfaces, partially failed")
	}

	return nil
}

// attachSkbInterfaces function
func attachSkbInterfaces(objs *bpfObjects) error {
	bpfSkbInterfaceMapping = make(map[string]link.Link)
	partialFailure := false

	cgroupPath, err := detectCgroupPath()
	if err != nil {
		log.Printf("[SKB] Failed to retrieve cgroup path: %v", err)
		return err
	}

	log.Printf("[SKB - Ingress] Attaching to %s", cgroupPath)
	li, err := link.AttachCgroup(link.CgroupOptions{
		Path:    cgroupPath,
		Program: objs.SkbIngressPktFunction,
	})

	if err != nil {
		log.Printf("Error attaching SKB ingress %s: %v\n", cgroupPath, err)
		partialFailure = true
	}
	bpfSkbInterfaceMapping["ingress"] = li

	log.Printf("[SKB - Egress] Attaching to %s", cgroupPath)
	le, err := link.AttachCgroup(link.CgroupOptions{
		Path:    cgroupPath,
		Program: objs.SkbEgressPktFunction,
	})

	if err != nil {
		log.Printf("Error attaching SKB egress %s: %v\n", cgroupPath, err)
		partialFailure = true
	}

	bpfSkbInterfaceMapping["egress"] = le

	err = objs.SkbIngressPktCount.Update(defaultKey, defaultPkt, ebpf.UpdateAny)
	if err != nil {
		log.Printf("[SKB - Ingress] Failed to update default key: %v", err)
		return err
	}

	err = objs.SkbEgressPktCount.Update(defaultKey, defaultPkt, ebpf.UpdateAny)
	if err != nil {
		log.Printf("[SKB - Egress] Failed to update default key: %v", err)
		return err
	}

	if partialFailure {
		return fmt.Errorf("failed to attach SKB interfaces, partially failed")
	}

	return nil
}

// detectCgroupPath returns the first-found mount point of type cgroup2
func detectCgroupPath() (string, error) {
	f, err := os.Open("/proc/mounts")
	if err != nil {
		return "", err
	}
	defer func(f *os.File) {
		err := f.Close()
		if err != nil {
			return
		}
	}(f)

	scanner := bufio.NewScanner(f)
	for scanner.Scan() {
		// example fields: cgroup2 /sys/fs/cgroup/unified cgroup2 rw,nosuid,nodev,noexec,relatime 0 0
		fields := strings.Split(scanner.Text(), " ")
		if len(fields) >= 3 && fields[2] == "cgroup2" {
			return fields[1], nil
		}
	}

	return "", errors.New("cgroup2 not mounted")
}

// getStats function
func getStats(objs *bpfObjects) {
	log.Printf("Getting stats...")

	pkt := bpfPacketInfo{}
	key := uint64(0)

	skbEgressEntries := objs.SkbEgressPktCount.Iterate()
	stats := make([]stat, 0)
	for skbEgressEntries.Next(&key, &pkt) {
		ifIndex := pkt.LenIfindex >> 16
		pktLen := pkt.LenIfindex & 0xFFFF

		data := stat{
			timestamp:     pkt.Timestamp,
			packetHash:    pkt.Hash,
			ifIndex:       ifIndex,
			len:           pktLen,
			containerName: containerInterfaceMapping[int(ifIndex)],
			containerID:   containerNameIDMapping[containerInterfaceMapping[int(ifIndex)]],
			vethName:      vethInterfaceMapping[int(ifIndex)],
			hook:          "skb_egress",
		}

		stats = append(stats, data)
	}

	skbIngressEntries := objs.SkbIngressPktCount.Iterate()
	for skbIngressEntries.Next(&key, &pkt) {
		ifIndex := pkt.LenIfindex >> 16
		pktLen := pkt.LenIfindex & 0xFFFF

		data := stat{
			timestamp:     pkt.Timestamp,
			packetHash:    pkt.Hash,
			ifIndex:       ifIndex,
			len:           pktLen,
			containerName: containerInterfaceMapping[int(ifIndex)],
			containerID:   containerNameIDMapping[containerInterfaceMapping[int(ifIndex)]],
			vethName:      vethInterfaceMapping[int(ifIndex)],
			hook:          "skb_ingress",
		}

		stats = append(stats, data)
	}

	TCEgressEntries := objs.TcEgressPktCount.Iterate()
	for TCEgressEntries.Next(&key, &pkt) {
		ifIndex := pkt.LenIfindex >> 16
		pktLen := pkt.LenIfindex & 0xFFFF

		data := stat{
			timestamp:     pkt.Timestamp,
			packetHash:    pkt.Hash,
			ifIndex:       ifIndex,
			len:           pktLen,
			containerName: containerInterfaceMapping[int(ifIndex)],
			containerID:   containerNameIDMapping[containerInterfaceMapping[int(ifIndex)]],
			vethName:      vethInterfaceMapping[int(ifIndex)],
			hook:          "tc_egress",
		}

		stats = append(stats, data)
	}

	TCIngressEntries := objs.TcIngressPktCount.Iterate()
	for TCIngressEntries.Next(&key, &pkt) {
		ifIndex := pkt.LenIfindex >> 16
		pktLen := pkt.LenIfindex & 0xFFFF

		data := stat{
			timestamp:     pkt.Timestamp,
			packetHash:    pkt.Hash,
			ifIndex:       ifIndex,
			len:           pktLen,
			containerName: containerInterfaceMapping[int(ifIndex)],
			containerID:   containerNameIDMapping[containerInterfaceMapping[int(ifIndex)]],
			vethName:      vethInterfaceMapping[int(ifIndex)],
			hook:          "tc_ingress",
		}

		stats = append(stats, data)
	}

	xdpIngressEntries := objs.XdpIngressPktCount.Iterate()
	for xdpIngressEntries.Next(&key, &pkt) {
		ifIndex := pkt.LenIfindex >> 16
		buf := make([]byte, 2)
		binary.BigEndian.PutUint16(buf, uint16(pkt.LenIfindex & 0xFFFF))
		pktLen := binary.LittleEndian.Uint16(buf)

		data := stat{
			timestamp:     pkt.Timestamp,
			packetHash:    pkt.Hash,
			ifIndex:       ifIndex,
			len:           uint32(pktLen),
			containerName: containerInterfaceMapping[int(ifIndex)],
			containerID:   containerNameIDMapping[containerInterfaceMapping[int(ifIndex)]],
			vethName:      vethInterfaceMapping[int(ifIndex)],
			hook:          "xdp_ingress",
		}
		
		stats = append(stats, data)
	}
	
	xdpEgressEntries := objs.XdpEgressPktCount.Iterate()
	for xdpEgressEntries.Next(&key, &pkt) {
		ifIndex := pkt.LenIfindex >> 16
		buf := make([]byte, 2)
		binary.BigEndian.PutUint16(buf, uint16(pkt.LenIfindex & 0xFFFF))
		pktLen := binary.LittleEndian.Uint16(buf)

		data := stat{
			timestamp:     pkt.Timestamp,
			packetHash:    pkt.Hash,
			ifIndex:       ifIndex,
			len:           uint32(pktLen),
			containerName: containerInterfaceMapping[int(ifIndex)],
			containerID:   containerNameIDMapping[containerInterfaceMapping[int(ifIndex)]],
			vethName:      vethInterfaceMapping[int(ifIndex)],
			hook:          "xdp_egress",
		}
		
		stats = append(stats, data)
	}

	err := WriteStatsToCSV(&stats, "stats.csv", 10)
	if err != nil {
		log.Printf("Failed to write CSV stats.csv: %v", err)
	}
}

// getInterfaceByIndex function
func getInterfaceByIndex(index int) string {
	// Get all network interfaces
	interfaces, err := net.Interfaces()
	if err != nil {
		return fmt.Sprintf("iface=%d", index)
	}

	// Find interface with matching index
	for _, iface := range interfaces {
		if iface.Index == index {
			return iface.Name
		}
	}

	return fmt.Sprintf("iface=%d", index)
}

// cleanUpLinks function
func cleanUpLinks() {
	for i, lnk := range bpfXdpInterfaceMapping {
		log.Printf("Unlinking %s", i)
		err := lnk.Close()
		if err != nil {
			log.Printf("Failed to close link for %s: %v", i, err)
		}
	}

	for i, lnk := range bpfTcInterfaceMapping {
		log.Printf("Unlinking %s", i)
		err := lnk.Close()
		if err != nil {
			log.Printf("Failed to close link for %s: %v", i, err)
		}
	}

	for i, lnk := range bpfSkbInterfaceMapping {
		log.Printf("Unlinking %s", i)
		err := lnk.Close()
		if err != nil {
			log.Printf("Failed to close link for %s: %v", i, err)
		}
	}
}

// main function
func main() {
	containerInterfaceMapping = make(map[int]string)
	containerNameIDMapping = make(map[string]string)
	containerNSPathMapping = make(map[string]string)
	vethInterfaceMapping = make(map[int]string)

	// Allow the current process to lock memory for eBPF resources.
	if err := rlimit.RemoveMemlock(); err != nil {
		log.Fatal(err)
	}

	scanNetworkInterfaces()

	vethInterfaceMapping[1] = getInterfaceByIndex(1)
	//vethInterfaceMapping[mainInterface] = getInterfaceByIndex(mainInterface)
	vethInterfaceMapping[docker0Interface] = getInterfaceByIndex(docker0Interface)
	containerInterfaceMapping[1] = "host_lo"
	//containerInterfaceMapping[mainInterface] = "host_nic"
	containerInterfaceMapping[docker0Interface] = "docker0"
	containerNameIDMapping["host_lo"] = "host"
	//containerNameIDMapping["host_nic"] = "host"
	containerNameIDMapping["docker0"] = "docker0"

	// Load pre-compiled programs and maps into the kernel.
	objs := bpfObjects{}
	if err := loadBpfObjects(&objs, nil); err != nil {
		log.Fatalf("loading objects: %v", err)
	}
	defer func(objs *bpfObjects) {
		err := objs.Close()
		if err != nil {
			return
		}
	}(&objs)

	stopper := make(chan os.Signal, 1)
	signal.Notify(stopper, os.Interrupt, syscall.SIGTERM)

	err := attachSkbInterfaces(&objs)
	if err != nil {
		log.Fatal(err)
	}

	err = attachTcInterfaces(&objs)
	if err != nil {
		log.Fatal(err)
	}

	err = attachXdpInterfaces(&objs)
	if err != nil {
		log.Fatal(err)
	}

	log.Println("Waiting for events..")

	sigTermCount := 0
	for {
		select {
		case <-stopper:
			sigTermCount++
			if sigTermCount > 1 {
				cleanUpLinks()
				log.Printf("Stopping completely")
				return
			} else {
				log.Printf("Getting stats... SIGTERM Once more to quit")
				getStats(&objs)
				log.Printf("Done!")
			}
		}
	}
}
