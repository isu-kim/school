package containers

import (
	"context"
	"fmt"
	"log"
	"sync"
	"time"

	"github.com/docker/docker/client"
	docker "github.com/fsouza/go-dockerclient"
)

// ==================== //
// == Docker Handler == //
// ==================== //

func init() {
	Dh = NewDockerHandler()
}

var Dh *DockerHandler

// DockerHandler structure
type DockerHandler struct {
	client      *docker.Client
	containers  map[string]*docker.Container
	ctx         context.Context
	running     bool
	initialized bool
	mu          sync.RWMutex
}

// NewDockerHandler function
func NewDockerHandler() *DockerHandler {
	return &DockerHandler{
		containers:  make(map[string]*docker.Container),
		ctx:         context.Background(),
		running:     false,
		initialized: false,
		mu:          sync.RWMutex{},
		client:      nil,
	}
}

// Init function
func (dh *DockerHandler) Init() error {
	c, err := docker.NewClientFromEnv()
	if err != nil {
		return fmt.Errorf("failed to create docker client: %w", err)
	}

	dh.client = c
	dh.initialized = true
	return nil
}

// IsRunning returns the current running state of the handler
func (dh *DockerHandler) IsRunning() bool {
	info, err := dh.client.Info()
	if err != nil {
		return false
	}

	log.Printf("[Docker] Status: %s", info.SystemStatus)
	return true
}

// StartAllContainers function
func (dh *DockerHandler) StartAllContainers() error {
	for _, imageName := range containerImages {
		// Create container configuration
		containerConfig := &docker.Config{
			Image: imageName,
		}

		// Host configuration
		hostConfig := &docker.HostConfig{
			AutoRemove: true, // Remove container when it exits
		}

		// Create the container
		container, err := dh.client.CreateContainer(docker.CreateContainerOptions{
			Config:     containerConfig,
			HostConfig: hostConfig,
		})
		if err != nil {
			log.Printf("[Docker] failed to create container with image %s: %v", imageName, err)
		}

		// Start the container
		if err := dh.client.StartContainer(container.ID, nil); err != nil {
			log.Printf("[Docker] failed to start container %s, image %s: %v", container.ID, imageName, err)
		}

		dh.containers[container.ID] = container
	}

	return nil
}

// StopAllContainers function
func (dh *DockerHandler) StopAllContainers() error {
	ctrs, err := dh.client.ListContainers(docker.ListContainersOptions{All: true})
	if err != nil {
		return err
	}

	for _, ctr := range ctrs {
		for _, imageName := range containerImages {
			if ctr.Image == imageName {
				err := dh.client.RemoveContainer(docker.RemoveContainerOptions{
					ID:    ctr.ID,
					Force: true,
				})

				if err != nil {
					log.Printf("[Docker] failed to stop container %s: %v", ctr.ID, err)
				}

				dh.containers[ctr.ID] = nil
			}
		}
	}

	return nil
}

// GetContainerInfo function with additional error handling
// Container ID, Container Image, Container Name
func (dh *DockerHandler) GetContainerInfo(containerID string) (string, string, string) {
	if containerID == "" {
		log.Printf("Empty container ID provided")
		return "", "", ""
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	ctr, err := dh.client.InspectContainerWithOptions(
		docker.InspectContainerOptions{
			Context: ctx,
			ID:      containerID,
			Size:    false,
		},
	)

	if err != nil {
		if client.IsErrNotFound(err) {
			log.Printf("[Docker] Container %s not found", containerID)
		} else {
			log.Printf("[Docker] Failed to inspect container %s: %v", containerID, err)
		}

		return "", "", ""
	}

	return ctr.ID, ctr.Config.Image, ctr.Name
}

// ForceStopContainer forcefully stops a specific container
func (dh *DockerHandler) ForceStopContainer(containerID string) error {
	dh.mu.Lock()
	defer dh.mu.Unlock()

	container, exists := dh.containers[containerID]
	if !exists {
		return fmt.Errorf("container %s not found", containerID)
	}

	// Force stop with 0 timeout
	err := dh.client.StopContainer(container.ID, 0)
	if err != nil {
		return fmt.Errorf("failed to force stop container %s: %w", containerID, err)
	}

	return nil
}

// StartRoutine begins the container monitoring routine
// async
func (dh *DockerHandler) StartRoutine() {
	go func() {
		err := dh.Init()
		if err != nil {
			log.Fatalf("[Docker] Failed to init Docker: %v", err)
		}

		err = dh.StartAllContainers()
		if err != nil {
			log.Fatalf("[Docker] Failed to start Docker container(s) %v", err)
		}
	}()
}
