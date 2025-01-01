package db

import (
	"context"
	"ee515_honeypot/types"
	"fmt"
	"log"
	"time"
	"strings"

	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	"go.mongodb.org/mongo-driver/mongo/readpref"
)

const (
	defaultHost            = "localhost"
	defaultPort            = 27017
	defaultDatabase        = "honeypot"
	collectionContainerLog = "container_logs" // Using snake_case as it's more conventional in MongoDB
	collectionBpfNetLog    = "bpf_network_logs"
	collectionBpfFdLog     = "bpf_fd_logs"
	connectionTimeout      = 10 * time.Second
)

var Mdb *MongoDB

func init() {
	var err error
	Mdb, err = newMongoDB()
	if err != nil {
		log.Fatalf("Failed to init MongoDB: %v", err)
	}
}

// MongoDB structure
type MongoDB struct {
	client           *mongo.Client
	colContainerLogs *mongo.Collection
	colBpfFdLogs     *mongo.Collection
	colBpfNetLogs    *mongo.Collection
}

// newMongoDB creates a new MongoDB connection
func newMongoDB() (*MongoDB, error) {
	ctx, cancel := context.WithTimeout(context.Background(), connectionTimeout)
	defer cancel()

	// Build connection URI
	uri := fmt.Sprintf("mongodb://%s:%d", defaultHost, defaultPort)

	// Create client options
	clientOptions := options.Client().ApplyURI(uri)

	// Connect to MongoDB
	client, err := mongo.Connect(ctx, clientOptions)
	if err != nil {
		return nil, fmt.Errorf("failed to connect to MongoDB: %v", err)
	}

	// Ping the database to verify connection
	if err = client.Ping(ctx, readpref.Primary()); err != nil {
		return nil, fmt.Errorf("failed to ping MongoDB: %v", err)
	}

	return &MongoDB{
		client:           client,
		colContainerLogs: client.Database(defaultDatabase).Collection(collectionContainerLog),
		colBpfNetLogs:    client.Database(defaultDatabase).Collection(collectionBpfNetLog),
		colBpfFdLogs:     client.Database(defaultDatabase).Collection(collectionBpfFdLog),
	}, nil
}

// InsertCRILog function async
func InsertCRILog(clog types.CRILog) {
	go func() {
		err := Mdb.internalInsertCRILog(clog)
		if err != nil {
			// log.Printf("Failed to store log: %v", err)
			return
		}
	}()
}

// InsertBPFFdLog function async
func InsertBPFFdLog(clog types.BPFFdLog) {
	go func() {
		err := Mdb.internalInsertBPFFdLog(clog)
		if err != nil {
			// log.Printf("Failed to store log: %v", err)
			return
		}
	}()
}

// InsertBPFNetLog function async
func InsertBPFNetLog(clog types.BPFNetLog) {
	go func() {
		err := Mdb.internalInsertBPFNetLog(clog)
		if err != nil {
			// log.Printf("Failed to store log: %v", err)
			return
		}
	}()
}

// internalInsertBPFFdLog inserts a CRI log entry into MongoDB
func (mdb *MongoDB) internalInsertBPFFdLog(bfl types.BPFFdLog) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if bfl.ContainerID == "" {
		return fmt.Errorf("failed to insert BPF fd log, no containerID")
	}

	// Insert the document
	if (strings.Contains(bfl.ContainerImage, "mongo") || strings.Contains(bfl.FileName, "proc")) {
		return nil
	}

	_, err := mdb.colBpfFdLogs.InsertOne(ctx, bfl)
	if err != nil {
		return fmt.Errorf("failed to insert BPF fd log: %v", err)
	}

	log.Printf("[BPF] FD event : container=%s(%s), file_name=%s, operation=%s",
		bfl.ContainerID, bfl.ContainerImage, bfl.FileName, bfl.Operation)

	return nil
}

// internalInsertBPFNetLog inserts a CRI log entry into MongoDB
func (mdb *MongoDB) internalInsertBPFNetLog(bnl types.BPFNetLog) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if bnl.ContainerID == "" {
		return fmt.Errorf("failed to insert BPF net log, no containerID")
	}

	if (strings.Contains(bnl.ContainerImage, "mongo")) {
		return nil
	}

	// Insert the document
	_, err := mdb.colBpfNetLogs.InsertOne(ctx, bnl)
	if err != nil {
		return fmt.Errorf("failed to insert BPF net log: %v", err)
	}

	log.Printf("[BPF] Network event : container=%s(%s), dst=%s, port=%d, direction=%s",
		bnl.ContainerID, bnl.ContainerImage, bnl.DstIP, bnl.DstPort, bnl.Direction)

	return nil
}

// internalInsertCRILog inserts a CRI log entry into MongoDB
func (mdb *MongoDB) internalInsertCRILog(clog types.CRILog) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if clog.ContainerID == "" {
		return fmt.Errorf("failed to insert CRI log, no containerID")
	}

	// Insert the document
	_, err := mdb.colContainerLogs.InsertOne(ctx, clog)
	if err != nil {
		return fmt.Errorf("failed to insert CRI log: %v", err)
	}


	if (strings.Contains(clog.ContainerImage, "mongo")) {
		return nil
	}

	log.Printf("Injecting log: [%s] Type=%s ID=%s(%s), args=%s, action=%s", clog.RemoteIP,
		clog.CRIType, clog.ContainerID, clog.ContainerImage, clog.Args, clog.Action)

	return nil
}

// Close closes the MongoDB connection
func (mdb *MongoDB) Close() error {
	if mdb.client != nil {
		ctx, cancel := context.WithTimeout(context.Background(), connectionTimeout)
		defer cancel()

		if err := mdb.client.Disconnect(ctx); err != nil {
			return fmt.Errorf("failed to disconnect from MongoDB: %v", err)
		}
	}
	return nil
}
