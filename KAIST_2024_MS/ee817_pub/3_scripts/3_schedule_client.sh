#!/bin/bash

# Get number of instances from user
read -p "Enter the number of instances to check: " num_instances

# Validate input is a number
if ! [[ "$num_instances" =~ ^[0-9]+$ ]]; then
    echo "Please enter a valid number"
    exit 1
fi

# Get current time and round up to the next minute
current_time=$(date -u +"%Y-%m-%dT%H:%M:00Z")
next_minute=$(date -u -d "$current_time + 1 minute" +"%Y-%m-%dT%H:%M:00Z")

echo "Current time: $current_time"
echo "Scheduled time: $next_minute"

# Send curl requests to all instances in background
for i in $(seq 0 $(($num_instances-1))); do
    port=$((8000 + $i))
    curl -X POST http://localhost:${port}/send_future \
        -H "Content-Type: application/json" \
        -d "{
            \"pcap_file\": \"generated_packets_udp_1000.pcap\",
            \"rate\": \"10GiB\",
            \"interface\": \"eth0\",
            \"scheduled_time\": \"$next_minute\"
        }" &
    echo "Scheduled request for port ${port} at $next_minute"
done

# Wait for all background processes to complete
wait

echo "All scheduling requests completed"
