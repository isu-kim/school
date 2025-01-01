#!/bin/bash

# Function to stop and remove all running containers
cleanup_containers() {
    echo "Stopping and removing all running containers..."
    docker rm -f $(docker ps -a -q) 2>/dev/null || true
}

# Get number of instances from user
read -p "Enter the number of instances to run: " num_instances

# Validate input is a number
if ! [[ "$num_instances" =~ ^[0-9]+$ ]]; then
    echo "Please enter a valid number"
    exit 1
fi

cleanup_containers

# Start Docker containers
for i in $(seq 1 $num_instances); do
    port=$((8000 + $i - 1))
    docker run -d --name "${i}s" --rm -p ${port}:8000 isukim/ee817_traffic
    echo "Started container ${i}s on port ${port}"
    sleep 1  # Small delay to ensure container is ready
done

# Wait a moment for all containers to be fully ready
sleep 3

# Send curl requests to all instances in background
for i in $(seq 0 $(($num_instances-1))); do
    port=$((8000 + $i))
    dst_port=$((8080 + $i))
    curl -X POST http://localhost:${port}/generate_traffic \
        -H "Content-Type: application/json" \
        -d "{\"dst_ip\": \"10.10.1.2\", \"dst_port\": $dst_port, \"protocol\": \"udp\", \"packet_count\": 1000}" &
    echo "Sent request to port ${port} with destination port ${dst_port}"
done

# Wait for all background processes to complete
wait

echo "All requests completed"
