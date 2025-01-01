package types

import (
	"ee515_honeypot/containers"
	"encoding/json"
	"fmt"
	"net/http"
	"regexp"
	"strconv"
	"strings"
	"time"
)

func init() {
	unmatchedMap = make(map[string]CRILog)
}

const (
	CRIDocker     = 1
	CRIContainerd = 2
)

var unmatchedMap map[string]CRILog

// CRILog structure
type CRILog struct {
	CRIType        string    `bson:"cri_type" json:"cri_type"`
	TimeStamp      time.Time `bson:"time_stamp" json:"time_stamp"`
	ContainerName  string    `bson:"container_name" json:"container_name"`
	ContainerID    string    `bson:"container_id" json:"container_id"`
	ContainerImage string    `bson:"container_image" json:"container_image"`
	Action         string    `bson:"action" json:"action"`
	Args           string    `bson:"args" json:"args"`
	RemoteIP       string    `bson:"remote_ip" json:"remote_ip"`
	APIEndpoint    string    `bson:"api_endpoint" json:"api_endpoint"`
	APIMethod      string    `bson:"api_method" json:"api_method"`
}

// GenerateCRILog function
func GenerateCRILog(triggerType int, req *http.Request, body []byte, needMatch bool) (CRILog, error) {

    // Get the real client address from our custom header
    remoteAddr := req.Header.Get("X-Combined-Addr")
    if remoteAddr == "" {
        // Fallback to normal RemoteAddr if header not present
        remoteAddr = req.RemoteAddr
    }
    req.Header.Del("X-Combined-Addr")

	if needMatch {
		type responseArgs struct {
			ID string `json:"Id`
		}

		rp := responseArgs{}
		err := json.Unmarshal(body, &rp)
		if err != nil {
			return CRILog{}, nil
		}

		prev := unmatchedMap[remoteAddr]
		prev.ContainerID = rp.ID

		delete(unmatchedMap, remoteAddr)
		return prev, nil
	}

	switch triggerType {
	case 1: // Docker
		return generateCRILogDocker(req, body), nil
	// Add other CRI types if needed
	default:
		return CRILog{}, nil
	}
}

// generateCRILogDocker processes Docker API requests and generates CRI logs
func generateCRILogDocker(req *http.Request, body []byte) CRILog {
	type requestArgs struct {
		Command []string `json:"Cmd"`
		TTY     bool     `json:"Tty"`
		Image   string   `json:"Image"`
	}

	args := requestArgs{}
	_ = json.Unmarshal(body, &args)

    // Get the real client address from our custom header
    remoteAddr := req.Header.Get("X-Combined-Addr")
    if remoteAddr == "" {
        // Fallback to normal RemoteAddr if header not present
        remoteAddr = req.RemoteAddr
    }
    req.Header.Del("X-Combined-Addr")

	// Initialize the CRI log
	cl := CRILog{
		CRIType:        "Docker",
		TimeStamp:      time.Now(),
		APIEndpoint:    req.URL.Path,
		APIMethod:      req.Method,
		RemoteIP:       remoteAddr,
		Args:           fmt.Sprintf("Commands=\"%s\", TTY=%s", strings.Join(args.Command, " "), strconv.FormatBool(args.TTY)),
		ContainerImage: args.Image,
	}

	// Parse the URL path to determine action and container details
	pathParts := getPathPartsWithoutVersion(req.URL.Path)

	if len(pathParts) >= 2 {
		// Set the action based on the API endpoint
		switch pathParts[0] {
		case "containers":
			cl.Action = determineContainerAction(req.Method, pathParts)
			if len(pathParts) > 1 {
				if pathParts[1] == "create" {
					cl.Action = "container_create"
					cl.ContainerImage = args.Image
					unmatchedMap[remoteAddr] = cl
				} else {
					cl.ContainerID, cl.ContainerImage, cl.ContainerName = containers.Dh.GetContainerInfo(pathParts[1])
				}
			}

		case "images":
			cl.Action = "image_" + strings.ToLower(req.Method)
		case "volumes":
			cl.Action = "volume_" + strings.ToLower(req.Method)
		default:
			cl.Action = strings.ToLower(req.Method)
		}
	}

	return cl
}

// Skip version prefix if present (e.g., "v1.41")
func getPathPartsWithoutVersion(path string) []string {
	// Split path into parts
	parts := strings.Split(strings.Trim(path, "/"), "/")

	// If we have parts and the first part matches version pattern (v\d+\.\d+)
	if len(parts) > 0 && regexp.MustCompile(`^v\d+\.\d+$`).MatchString(parts[0]) {
		// Remove the version part
		parts = parts[1:]
	}

	return parts
}

// determineContainerAction helper function to determine the specific container action
func determineContainerAction(method string, pathParts []string) string {
	if len(pathParts) < 3 {
		return "container_" + strings.ToLower(method)
	}

	// Handle specific container actions
	switch pathParts[2] {
	case "start":
		return "container_start"
	case "stop":
		return "container_stop"
	case "restart":
		return "container_restart"
	case "kill":
		return "container_kill"
	case "exec":
		return "container_exec"
	case "logs":
		return "container_logs"
	default:
		return "container_" + strings.ToLower(method)
	}
}

// generateCRILogContainerd
func generateCRILogContainerd(req *http.Request) CRILog {
	// @todo implement this
	return CRILog{}
}
