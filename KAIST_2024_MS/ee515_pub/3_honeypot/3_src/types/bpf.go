package types

import (
	"ee515_honeypot/containers"
	"encoding/binary"
	"fmt"
	"net"
	"strings"
	"time"
)

const (
	bpfNetDirEgress  = 0
	bpfNetDirIngress = 1

	bpfFileOpRead  = 0
	bpfFileOpWrite = 1
)

// BPFNetLog structure
type BPFNetLog struct {
	CRIType        string    `bson:"cri_type" json:"cri_type"`
	TimeStamp      time.Time `bson:"time_stamp" json:"time_stamp"`
	ContainerName  string    `bson:"container_name" json:"container_name"`
	ContainerID    string    `bson:"container_id" json:"container_id"`
	ContainerImage string    `bson:"container_image" json:"container_image"`
	Direction      string    `bson:"direction" json:"direction"`
	DstIP          string    `bson:"dst_ip" json:"dst_ip"`
	DstPort        uint16    `bson:"dst_port" json:"dst_port"`
}

// NewBPFNetLog function
func NewBPFNetLog(cgroupName [32]uint8, dstIP uint32, dstPort uint16, proto uint8, direction uint8, criType uint8) BPFNetLog {
	criString := "unknown"
	id := ""
	image := ""
	name := ""

	switch criType {
	case CRIDocker:
		criString = "Docker"
		// remove docker- tag
		id, image, name = containers.Dh.GetContainerInfo(
			strings.ReplaceAll(fmt.Sprintf("%s", cgroupName[:len(cgroupName)-2]), "docker-", ""))

		// @todo add containerd as well
	}

	dirStr := "unknown"
	switch direction {
	case bpfNetDirEgress:
		dirStr = "egress"
	case bpfNetDirIngress:
		dirStr = "ingress"
	}

	ip := make(net.IP, 4)
	binary.LittleEndian.PutUint32(ip, dstIP)
	ipStr := ip.String()

	return BPFNetLog{
		CRIType:        criString,
		TimeStamp:      time.Now(),
		ContainerName:  name,
		ContainerID:    id,
		ContainerImage: image,
		Direction:      dirStr,
		DstIP:          ipStr,
		DstPort:        (dstPort >> 8) | ((dstPort & 0xFF) << 8),
	}
}

// BPFFdLog structure
type BPFFdLog struct {
	CRIType        string    `bson:"cri_type" json:"cri_type"`
	TimeStamp      time.Time `bson:"time_stamp" json:"time_stamp"`
	ContainerName  string    `bson:"container_name" json:"container_name"`
	ContainerID    string    `bson:"container_id" json:"container_id"`
	ContainerImage string    `bson:"container_image" json:"container_image"`
	FileName       string    `bson:"file_name" json:"file_name"`
	Operation      string    `bson:"operation" json:"operation"`
}

// NewBPFFdLog function
func NewBPFFdLog(cgroupName [32]uint8, fileName string, operation uint8, criType uint8) BPFFdLog {
	criString := "unknown"
	id := ""
	image := ""
	name := ""

	switch criType {
	case CRIDocker:
		criString = "Docker"
		// remove docker- tag
		id, image, name = containers.Dh.GetContainerInfo(
			strings.ReplaceAll(fmt.Sprintf("%s", cgroupName[:len(cgroupName)-2]), "docker-", ""))

		// @todo add containerd as well
	}

	opStr := "unknown"
	switch operation {
	case bpfFileOpRead:
		opStr = "read"
	case bpfFileOpWrite:
		opStr = "write"
	}

	return BPFFdLog{
		CRIType:        criString,
		TimeStamp:      time.Now(),
		ContainerName:  name,
		ContainerID:    id,
		ContainerImage: image,
		Operation:      opStr,
		FileName:       fileName,
	}
}
