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

# Create controller manager client certificates
RUN openssl genrsa -out /etc/kubernetes/pki/controller-manager.key 2048 \
    && openssl req -new -key /etc/kubernetes/pki/controller-manager.key -out /etc/kubernetes/pki/controller-manager.csr -subj "/CN=system:kube-controller-manager" \
    && openssl x509 -req -in /etc/kubernetes/pki/controller-manager.csr -CA /etc/kubernetes/pki/ca.crt -CAkey /etc/kubernetes/pki/ca.key -CAcreateserial -out /etc/kubernetes/pki/controller-manager.crt -days 365 -sha256

# Create service account key pair
RUN openssl genrsa -out /etc/kubernetes/pki/sa.key 2048 \
    && openssl rsa -in /etc/kubernetes/pki/sa.key -pubout -out /etc/kubernetes/pki/sa.pub

# Set standard controller manager environment variables
ENV KUBERNETES_SERVICE_HOST=192.168.1.10
ENV KUBERNETES_SERVICE_PORT=6443
ENV KUBERNETES_PORT=tcp://192.168.1.10:6443
ENV KUBERNETES_PORT_443_TCP=tcp://192.168.1.10:6443
ENV KUBERNETES_PORT_443_TCP_ADDR=192.168.1.10
ENV KUBERNETES_PORT_443_TCP_PORT=6443
ENV KUBERNETES_PORT_443_TCP_PROTO=tcp

# Set controller manager specific environment variables
ENV CONTROLLER_MANAGER_PORT=--port=10252
ENV CONTROLLER_MANAGER_SECURE_PORT=--secure-port=10257
ENV CONTROLLER_MANAGER_KUBECONFIG=--kubeconfig=/etc/kubernetes/conf/controller-manager.conf
ENV CONTROLLER_MANAGER_LEADER_ELECT=--leader-elect=true
ENV CONTROLLER_MANAGER_NODE_MONITOR_PERIOD=--node-monitor-period=5s
ENV CONTROLLER_MANAGER_NODE_MONITOR_GRACE_PERIOD=--node-monitor-grace-period=40s
ENV CONTROLLER_MANAGER_POD_EVICTION_TIMEOUT=--pod-eviction-timeout=5m0s

# Set proper permissions
RUN chmod 600 /etc/kubernetes/pki/*.key \
    && chmod 644 /etc/kubernetes/pki/*.crt \
    && chmod 644 /etc/kubernetes/pki/*.pub

COPY sleep.c .
RUN gcc -o kube-controller-manager sleep.c
RUN mv kube-controller-manager /usr/bin
RUN rm sleep.c

WORKDIR /usr/bin

# Set actual controller-manager arguments that will be visible in ps
CMD ["/usr/bin/kube-controller-manager", "--allocate-node-cidrs=true \
--authentication-kubeconfig=/etc/kubernetes/conf/controller-manager.conf \
--authorization-kubeconfig=/etc/kubernetes/conf/controller-manager.conf \
--bind-address=127.0.0.1 \
--client-ca-file=/etc/kubernetes/pki/ca.crt \
--cluster-cidr=10.244.0.0/16 \
--cluster-name=kubernetes \
--cluster-signing-cert-file=/etc/kubernetes/pki/ca.crt \
--cluster-signing-key-file=/etc/kubernetes/pki/ca.key \
--controllers=*,bootstrapsigner,tokencleaner \
--kubeconfig=/etc/kubernetes/conf/controller-manager.conf \
--leader-elect=true \
--node-monitor-grace-period=40s \
--node-monitor-period=5s \
--pod-eviction-timeout=5m0s \
--root-ca-file=/etc/kubernetes/pki/ca.crt \
--service-account-private-key-file=/etc/kubernetes/pki/sa.key \
--use-service-account-credentials=true \
--v=2"]
