package proxy

import (
	"bytes"
	"context"
	"ee515_honeypot/db"
	"ee515_honeypot/types"
	"errors"
	"fmt"
	"io"
	"log"
	"net/http"
	"net/http/httputil"
	"net/url"
	"strings"
	"time"
)

var ListenAddr = ":2376"
var DockerURL = "http://127.0.0.1:2378"
var ContainerdURL = "http://127.0.0.1:2376"
var CRIProxy *Server

const dockerInterceptedEndpoint = "/containers/json"

func init() {
	var err error

	CRIProxy, err = NewServer()
	if err != nil {
		log.Fatalf("Failed to start CRI proxy: %v", err)
	}
}

// Server structure
type Server struct {
	proxy        *httputil.ReverseProxy
	docker       *url.URL
	containerd   *url.URL
	dockerServer *http.Server
	ctx          context.Context
	cancel       context.CancelFunc
	errChan      chan error
}

// NewServer function
func NewServer() (*Server, error) {
	docker, err := url.Parse(DockerURL)
	if err != nil {
		return nil, err
	}

	containerd, err := url.Parse(ContainerdURL)
	if err != nil {
		return nil, err
	}

	return &Server{
		containerd: containerd,
		docker:     docker,
		errChan:    make(chan error),
		ctx:        context.Background(),
	}, nil
}

// ServeDocker function
func (s *Server) ServeDocker() error {
	s.proxy = httputil.NewSingleHostReverseProxy(s.docker)
	originalDirector := s.proxy.Director
	s.proxy.Director = func(req *http.Request) {
		originalDirector(req)
		req.Host = s.docker.Host
	}

	s.proxy.ModifyResponse = modifyDockerResponse
	server := &http.Server{
		Addr: ListenAddr,
		Handler: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			// Copy body
			body, err := io.ReadAll(r.Body)
			if err != nil {
				body = []byte{}
			}

			r.Body = io.NopCloser(bytes.NewBuffer(body))

			go func() {
				cLog, _ := types.GenerateCRILog(types.CRIDocker, r, body, false)
				db.InsertCRILog(cLog)
			}()

			//log.Printf("[Docker] Requested Endpoint %s [%s]", r.URL.Path, r.Method)
			s.proxy.ServeHTTP(w, r)
		}),
	}
	s.dockerServer = server

	// Start server in a goroutine
	go func() {
		log.Printf("[Docker] Starting proxy server on %s", ListenAddr)
		if err := s.dockerServer.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
			s.errChan <- fmt.Errorf("[Docker] Failed to start proxy server: %v", err)
		}
	}()

	// Wait for either context cancellation or server error
	select {
	case <-s.ctx.Done():
		return nil
	case err := <-s.errChan:
		return err
	}
}

// StopDocker function
func (s *Server) StopDocker() error {
	if s.dockerServer == nil {
		return fmt.Errorf("server not started")
	}

	log.Println("[Docker] Stopping proxy server...")

	// Create a timeout context for shutdown
	ctx, cancel := context.WithTimeout(context.Background(), 1*time.Second)
	defer cancel()

	// Cancel the server context
	if s.cancel != nil {
		s.cancel()
	}

	// Attempt graceful shutdown
	if err := s.dockerServer.Shutdown(ctx); err != nil {
		return fmt.Errorf("[Docker] failed to gracefully shut down server: %v", err)
	}

	log.Println("[Docker] Proxy server stopped successfully")
	return nil
}

// modifyDockerResponse function
func modifyDockerResponse(resp *http.Response) error {
	// create command
	if strings.Contains(resp.Request.URL.Path, "containers/create") {

		// Copy body
		body, err := io.ReadAll(resp.Body)
		if err != nil {
			body = []byte{}
		}

		resp.Body = io.NopCloser(bytes.NewBuffer(body))

		go func() {
			cLog, _ := types.GenerateCRILog(types.CRIDocker, resp.Request, body, true)
			db.InsertCRILog(cLog)
		}()
	}

	if !strings.Contains(resp.Request.URL.Path, dockerInterceptedEndpoint) {
		return nil
	}

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return err
	}

	err = resp.Body.Close()
	if err != nil {
		return err
	}

	// replace image name from isukim to real looking registry from K8s
	modifiedBody := strings.ReplaceAll(string(body), "isukim/ee515_", "registry.k8s.io/")

	resp.Body = io.NopCloser(bytes.NewBufferString(modifiedBody))
	resp.ContentLength = int64(len(modifiedBody))
	resp.Header.Set("Content-Length", string(rune(len(modifiedBody))))

	return nil
}
