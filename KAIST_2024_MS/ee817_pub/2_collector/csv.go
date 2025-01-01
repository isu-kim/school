package main

import (
	"encoding/csv"
	"fmt"
	"os"
	"strconv"
	"sync"
)

// Worker represents a single worker that processes stats
type worker struct {
	id         int
	inputChan  chan stat
	outputChan chan []string
	wg         *sync.WaitGroup
}

// NewWorker creates a new worker
func newWorker(id int, inputChan chan stat, outputChan chan []string, wg *sync.WaitGroup) *worker {
	return &worker{
		id:         id,
		inputChan:  inputChan,
		outputChan: outputChan,
		wg:         wg,
	}
}

// Start starts the worker processing loop
func (w *worker) start() {
	go func() {
		defer w.wg.Done()
		for stat := range w.inputChan {
			// Convert stat to CSV row
			row := []string{
				strconv.FormatUint(stat.timestamp, 10),
				strconv.FormatUint(uint64(stat.packetHash), 10),
				strconv.FormatUint(uint64(stat.ifIndex), 10),
				strconv.FormatUint(uint64(stat.len), 10),
				stat.containerName,
				stat.containerID,
				stat.vethName,
				stat.hook, // Added new hook field
			}
			w.outputChan <- row
		}
	}()
}

// WriteStatsToCSV writes a slice of stat structures to a CSV file using parallel processing
func WriteStatsToCSV(stats *[]stat, filename string, numWorkers int) error {
	if stats == nil {
		return fmt.Errorf("stats pointer is nil")
	}

	// Create or truncate the file
	file, err := os.Create(filename)
	if err != nil {
		return fmt.Errorf("error creating file: %v", err)
	}
	defer file.Close()

	// Create CSV writer
	writer := csv.NewWriter(file)
	defer writer.Flush()

	// Write headers
	headers := []string{
		"Timestamp",
		"PacketHash",
		"IfIndex",
		"Length",
		"ContainerName",
		"ContainerID",
		"VethName",
		"Hook", // Added new hook header
	}
	if err := writer.Write(headers); err != nil {
		return fmt.Errorf("error writing headers: %v", err)
	}

	// Create channels for work distribution
	inputChan := make(chan stat, numWorkers)
	outputChan := make(chan []string, len(*stats))

	// Create wait group for workers
	var wg sync.WaitGroup
	wg.Add(numWorkers)

	// Start workers
	for i := 0; i < numWorkers; i++ {
		worker := newWorker(i, inputChan, outputChan, &wg)
		worker.start()
	}

	// Start a goroutine to close the output channel once all workers are done
	go func() {
		wg.Wait()
		close(outputChan)
	}()

	// Start a goroutine to feed data to workers
	go func() {
		for _, s := range *stats {
			inputChan <- s
		}
		close(inputChan)
	}()

	// Collect and write results
	var writeError error
	rowsWritten := 0
	expectedRows := len(*stats)

	for row := range outputChan {
		if writeError != nil {
			continue // Skip writing if we've encountered an error
		}

		if err := writer.Write(row); err != nil {
			writeError = fmt.Errorf("error writing row: %v", err)
			continue
		}
		rowsWritten++

		// Flush periodically to avoid using too much memory
		if rowsWritten%1000 == 0 {
			writer.Flush()
			if err := writer.Error(); err != nil {
				writeError = fmt.Errorf("error flushing writer: %v", err)
			}
		}
	}

	// Final flush
	writer.Flush()
	if err := writer.Error(); err != nil {
		return fmt.Errorf("error on final flush: %v", err)
	}

	if writeError != nil {
		return writeError
	}

	if rowsWritten != expectedRows {
		return fmt.Errorf("expected to write %d rows, but wrote %d", expectedRows, rowsWritten)
	}

	return nil
}
