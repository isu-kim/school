#!/bin/bash

# Function to validate input is a positive integer
validate_input() {
    if ! [[ $1 =~ ^[0-9]+$ ]]; then
        echo "Error: Please enter a valid positive integer"
        exit 1
    fi
    if [ $1 -lt 1 ]; then
        echo "Error: Number must be greater than 0"
        exit 1
    fi
}

# Function to stop and remove all running containers
cleanup_containers() {
    echo "Stopping and removing all running containers..."
    docker rm -f $(docker ps -a -q) 2>/dev/null || true
}

# Main script
echo "Enter the number of containers to deploy:"
read num_containers

# Validate input
validate_input $num_containers

# Clean up existing containers
cleanup_containers

# Deploy new containers
echo "Deploying $num_containers containers..."
for i in $(seq 1 $num_containers)
do
    port=$((8080 + i - 1))
    container_name="${i}d"
    
    echo "Deploying container $container_name on port $port"
    docker run -d --name $container_name --rm -p 0.0.0.0:$port:80/udp isukim/ee817_target
    
    # Check if container was created successfully
    if [ $? -ne 0 ]; then
        echo "Error: Failed to create container $container_name"
        exit 1
    fi
done

echo "Deployment complete! Created $num_containers containers"
echo "Ports used: 8080-$((8080 + num_containers - 1))"
docker ps
