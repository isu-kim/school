FROM ubuntu

# Install necessary packages
RUN apt-get update && apt-get install -y \
    openssl gcc \
    && rm -rf /var/lib/apt/lists/*

# Create directories for certificates and configuration
RUN mkdir -p /etc/kubernetes/pki \
    && mkdir -p /etc/kubernetes/conf

# Create CA certificates and keys (reusing existing CA if running with API server)
RUN openssl genrsa -out /etc/kubernetes/pki/ca.key 2048 \
    && openssl req -x509 -new -nodes -key /etc/kubernetes/pki/ca.key -sha256 -days 1024 -out /etc/kubernetes/pki/ca.crt -subj "/CN=kubernetes-ca"

# Create scheduler client certificates
RUN openssl genrsa -out /etc/kubernetes/pki/scheduler.key 2048 \
    && openssl req -new -key /etc/kubernetes/pki/scheduler.key -out /etc/kubernetes/pki/scheduler.csr -subj "/CN=system:kube-scheduler" \
    && openssl x509 -req -in /etc/kubernetes/pki/scheduler.csr -CA /etc/kubernetes/pki/ca.crt -CAkey /etc/kubernetes/pki/ca.key -CAcreateserial -out /etc/kubernetes/pki/scheduler.crt -days 365 -sha256

# Set standard scheduler environment variables
ENV KUBERNETES_SERVICE_HOST=192.168.1.10
ENV KUBERNETES_SERVICE_PORT=6443
ENV KUBERNETES_PORT=tcp://192.168.1.10:6443
ENV KUBERNETES_PORT_443_TCP=tcp://192.168.1.10:6443
ENV KUBERNETES_PORT_443_TCP_ADDR=192.168.1.10
ENV KUBERNETES_PORT_443_TCP_PORT=6443
ENV KUBERNETES_PORT_443_TCP_PROTO=tcp

# Set scheduler specific environment variables
ENV SCHEDULER_PORT=--port=10251
ENV SCHEDULER_SECURE_PORT=--secure-port=10259
ENV SCHEDULER_KUBECONFIG=--kubeconfig=/etc/kubernetes/conf/scheduler.conf
ENV SCHEDULER_LEADER_ELECT=--leader-elect=true
ENV SCHEDULER_ALGORITHM_PROVIDER=--algorithm-provider=DefaultProvider
ENV SCHEDULER_PROFILING=--profiling=false
ENV SCHEDULER_BIND_ADDRESS=--bind-address=127.0.0.1
ENV SCHEDULER_PORT_PLACEHOLDER=--port=0

# Set proper permissions
RUN chmod 600 /etc/kubernetes/pki/*.key \
    && chmod 644 /etc/kubernetes/pki/*.crt

COPY sleep.c .
RUN gcc -o kube-scheduler sleep.c
RUN mv kube-scheduler /usr/bin
RUN rm sleep.c

WORKDIR /usr/bin

# Set actual scheduler arguments that will be visible in ps
CMD ["/usr/bin/kube-scheduler", "--authentication-kubeconfig=/etc/kubernetes/conf/scheduler.conf \
--authorization-kubeconfig=/etc/kubernetes/conf/scheduler.conf \
--bind-address=127.0.0.1 \
--kubeconfig=/etc/kubernetes/conf/scheduler.conf \
--leader-elect=true \
--port=0 \
--secure-port=10259 \
--profiling=false \
--tls-cert-file=/etc/kubernetes/pki/scheduler.crt \
--tls-private-key-file=/etc/kubernetes/pki/scheduler.key \
--v=2 \
--config=/etc/kubernetes/conf/scheduler-config.yaml"]
