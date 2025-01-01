FROM ubuntu

# Install necessary packages
RUN apt-get update && apt-get install -y \
    openssl ca-certificates curl gcc \
    && rm -rf /var/lib/apt/lists/*

# Create service account directories
RUN mkdir -p /var/run/secrets/kubernetes.io/serviceaccount \
    && mkdir -p /var/run/kubernetes

# Generate a mock CA certificate
RUN openssl genrsa -out /tmp/ca.key 2048 \
    && openssl req -x509 -new -nodes -key /tmp/ca.key -sha256 -days 1024 \
        -out /tmp/ca.crt -subj "/CN=kubernetes"

# Create a mock service account token (JWT format)
RUN echo 'eyJhbGciOiJSUzI1NiIsImtpZCI6IiJ9.eyJpc3MiOiJrdWJlcm5ldGVzL3NlcnZpY2VhY2NvdW50Iiwia3ViZXJuZXRlcy5pby9zZXJ2aWNlYWNjb3VudC9uYW1lc3BhY2UiOiJkZWZhdWx0Iiwia3ViZXJuZXRlcy5pby9zZXJ2aWNlYWNjb3VudC9zZWNyZXQubmFtZSI6ImRlZmF1bHQtdG9rZW4iLCJrdWJlcm5ldGVzLmlvL3NlcnZpY2VhY2NvdW50L3NlcnZpY2UtYWNjb3VudC5uYW1lIjoiZGVmYXVsdCIsImt1YmVybmV0ZXMuaW8vc2VydmljZWFjY291bnQvc2VydmljZS1hY2NvdW50LnVpZCI6IjEyMzQ1Iiwic3ViIjoic3lzdGVtOnNlcnZpY2VhY2NvdW50OmRlZmF1bHQ6ZGVmYXVsdCJ9.signature' > /tmp/token

# Create mock namespace file
RUN echo 'default' > /tmp/namespace

# Place service account files in both common locations
RUN cp /tmp/ca.crt /var/run/secrets/kubernetes.io/serviceaccount/ca.crt \
    && cp /tmp/token /var/run/secrets/kubernetes.io/serviceaccount/token \
    && cp /tmp/namespace /var/run/secrets/kubernetes.io/serviceaccount/namespace \
    && cp /tmp/ca.crt /var/run/kubernetes/ca.crt \
    && cp /tmp/token /var/run/kubernetes/token \
    && cp /tmp/namespace /var/run/kubernetes/namespace

# Set proper permissions (similar to what Kubernetes would set)
RUN chmod 644 /var/run/secrets/kubernetes.io/serviceaccount/ca.crt \
    && chmod 644 /var/run/secrets/kubernetes.io/serviceaccount/token \
    && chmod 644 /var/run/secrets/kubernetes.io/serviceaccount/namespace \
    && chmod 644 /var/run/kubernetes/ca.crt \
    && chmod 644 /var/run/kubernetes/token \
    && chmod 644 /var/run/kubernetes/namespace

# Clean up temporary files
RUN rm /tmp/ca.key /tmp/ca.crt /tmp/token /tmp/namespace

# Set environment variables that Kubernetes typically sets
ENV KUBERNETES_PORT=tcp://192.168.1.10:6443
ENV KUBERNETES_PORT_443_TCP=tcp://192.168.1.10:6443
ENV KUBERNETES_PORT_443_TCP_ADDR=192.168.1.10
ENV KUBERNETES_PORT_443_TCP_PORT=6443
ENV KUBERNETES_PORT_443_TCP_PROTO=tcp
ENV KUBERNETES_SERVICE_HOST=192.168.1.10
ENV KUBERNETES_SERVICE_PORT=443
ENV KUBERNETES_SERVICE_PORT_HTTPS=443


COPY sleep.c .
RUN gcc -o api-server sleep.c
RUN mkdir app
RUN mv api-server /app
WORKDIR /app

# Default command to keep container running
CMD ["/app/api-server", "--priviliged-container=true --mode=service_account --host=0.0.0.0 --root=true"]
