server {
    listen 2376;

    # For container-related endpoints
    location ~ ^/v[0-9\.]+/containers/ {
        proxy_pass http://172.23.14.65:2376;

        # Add custom headers for container endpoints
        proxy_set_header X-Original-Remote-Addr $remote_addr;
        proxy_set_header X-Original-Remote-Port $remote_port;

        set $original_addr $http_x_combined_addr;
        if ($original_addr = '') {
            set $original_addr "$remote_addr:$remote_port";
        }
        proxy_set_header X-Combined-Addr $original_addr;
    }

    # For exec endpoints with WebSocket support
    location ~ ^/v[0-9\.]+/exec/ {
        proxy_pass http://172.23.14.65:2376;

        # WebSocket support
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";

        # Keep connection alive
        proxy_read_timeout 3600s;
        proxy_send_timeout 3600s;
    }

    # Default location for all other endpoints
    location / {
        proxy_pass http://172.23.14.65:2376;
    }
}