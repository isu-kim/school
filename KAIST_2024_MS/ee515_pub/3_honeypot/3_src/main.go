package main

import (
	"ee515_honeypot/bpf"
	"ee515_honeypot/containers"
	"ee515_honeypot/proxy"
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"
)

// printLogo
func printLogo() {
	fmt.Print("______________________.____________ .________\n")
	fmt.Print("\\_   _____/\\_   _____/|   ____/_   ||   ____/\n")
	fmt.Print(" |    __)_  |    __)_ |____  \\ |   ||____  \\ \n")
	fmt.Print(" |        \\ |        \\/       \\|   |/       \\\n")
	fmt.Print("/_______  //_______  /______  /|___/______  /\n")
	fmt.Print("        \\/         \\/       \\/            \\/ ")
	fmt.Print("")
	fmt.Print("                         Docker & Containerd Honeypot\n\n")
}

func sigHandler() {
	// Create channel to receive signals
	sigChan := make(chan os.Signal, 1)

	// Register for SIGINT and SIGTERM signals
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	// Start goroutine to handle signals
	go func() {
		sig := <-sigChan
		log.Printf("Received signal: %v", sig)
		log.Println("Starting graceful shutdown...")

		err := containers.Dh.StopAllContainers()
		if err != nil {
			log.Printf("[Docker] Error stopping containers: %v", err)
			return
		}

		err = proxy.CRIProxy.StopDocker()
		if err != nil {
			log.Printf("[Proxy] Error stopping Docker CRI proxy: %v", err)
		}

		err = bpf.Bh.Stop()
		if err != nil {
			log.Printf("[BPF] Error stopping BPF: %v", err)
		}

		log.Println("Cleanup completed, shutting down")
		os.Exit(0)
	}()
}

// main function
func main() {
	printLogo()
	sigHandler()

	// Start eBPF
	err := bpf.Bh.Init()
	if err != nil {
		log.Fatalf("Error initializing BPF: %v", err)
	}

	err = bpf.Bh.Run()
	if err != nil {
		log.Fatalf("Error starting BPF: %v", err)
	}

	// Start Docker containers
	containers.Dh.StartRoutine()

	err = proxy.CRIProxy.ServeDocker()
	if err != nil {
		log.Printf("Failed to serve Docker proxy CRI: %v", err)
	}

	select {}
}
