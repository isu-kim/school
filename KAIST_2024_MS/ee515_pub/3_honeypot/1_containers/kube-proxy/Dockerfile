FROM ubuntu

# Install necessary packages
RUN apt-get update && apt-get install -y \
    openssl gcc iptables \
    && rm -rf /var/lib/apt/lists/*

# Create directories
RUN mkdir -p /etc/kubernetes/pki \
    && mkdir -p /var/lib/kube-proxy \
    && mkdir -p /var/log/kubernetes

# Generate CA certificates
RUN openssl genrsa -out /etc/kubernetes/pki/ca.key 2048 \
    && openssl req -x509 -new -nodes -key /etc/kubernetes/pki/ca.key -sha256 -days 1024 \
        -out /etc/kubernetes/pki/ca.crt -subj "/CN=kubernetes-ca"

# Generate kube-proxy client certificates
RUN openssl genrsa -out /etc/kubernetes/pki/kube-proxy.key 2048 \
    && openssl req -new -key /etc/kubernetes/pki/kube-proxy.key \
        -out /etc/kubernetes/pki/kube-proxy.csr \
        -subj "/CN=system:kube-proxy/O=system:node-proxier" \
    && openssl x509 -req -in /etc/kubernetes/pki/kube-proxy.csr \
        -CA /etc/kubernetes/pki/ca.crt \
        -CAkey /etc/kubernetes/pki/ca.key \
        -CAcreateserial \
        -out /etc/kubernetes/pki/kube-proxy.crt \
        -days 365 -sha256

# Create kube-proxy kubeconfig
RUN echo 'apiVersion: v1\n\
kind: Config\n\
clusters:\n\
- cluster:\n\
    certificate-authority: /etc/kubernetes/pki/ca.crt\n\
    server: https://192.168.1.10:6443\n\
  name: kubernetes\n\
contexts:\n\
- context:\n\
    cluster: kubernetes\n\
    user: system:kube-proxy\n\
  name: system:kube-proxy@kubernetes\n\
current-context: system:kube-proxy@kubernetes\n\
users:\n\
- name: system:kube-proxy\n\
  user:\n\
    client-certificate: /etc/kubernetes/pki/kube-proxy.crt\n\
    client-key: /etc/kubernetes/pki/kube-proxy.key' > /etc/kubernetes/kube-proxy.conf

# Create kube-proxy config file
RUN echo 'apiVersion: kubeproxy.config.k8s.io/v1alpha1\n\
kind: KubeProxyConfiguration\n\
bindAddress: 0.0.0.0\n\
clientConnection:\n\
  acceptContentTypes: ""\n\
  burst: 10\n\
  contentType: application/vnd.kubernetes.protobuf\n\
  kubeconfig: /etc/kubernetes/kube-proxy.conf\n\
  qps: 5\n\
clusterCIDR: "10.244.0.0/16"\n\
configSyncPeriod: 15m0s\n\
conntrack:\n\
  maxPerCore: 32768\n\
  min: 131072\n\
  tcpCloseWaitTimeout: 1h0m0s\n\
  tcpEstablishedTimeout: 24h0m0s\n\
enableProfiling: false\n\
healthzBindAddress: 0.0.0.0:10256\n\
hostnameOverride: ""\n\
iptables:\n\
  masqueradeAll: false\n\
  masqueradeBit: 14\n\
  minSyncPeriod: 0s\n\
  syncPeriod: 30s\n\
ipvs:\n\
  excludeCIDRs: null\n\
  minSyncPeriod: 0s\n\
  scheduler: ""\n\
  strictARP: false\n\
  syncPeriod: 30s\n\
  tcpFinTimeout: 0s\n\
  tcpTimeout: 0s\n\
  udpTimeout: 0s\n\
metricsBindAddress: 127.0.0.1:10249\n\
mode: "iptables"\n\
nodePortAddresses: null\n\
oomScoreAdj: -999\n\
portRange: ""\n\
udpIdleTimeout: 250ms\n\
winkernel:\n\
  enableDSR: false\n\
  networkName: ""\n\
  sourceVip: ""' > /var/lib/kube-proxy/config.conf

# Set environment variables
ENV KUBE_PROXY_OPTS="--config=/var/lib/kube-proxy/config.conf"
ENV KUBERNETES_SERVICE_HOST=192.168.1.10
ENV KUBERNETES_SERVICE_PORT=6443
ENV KUBERNETES_SERVICE_PORT_HTTPS=6443
ENV NODE_NAME=mock-node
ENV KUBE_PROXY_MODE=iptables
ENV KUBE_PROXY_HEALTHZ_BIND_ADDRESS=0.0.0.0:10256
ENV KUBE_PROXY_METRICS_BIND_ADDRESS=127.0.0.1:10249
ENV KUBE_PROXY_CONNTRACK_MAX_PER_CORE=32768
ENV KUBE_PROXY_CONNTRACK_MIN=131072

# Set proper permissions
RUN chmod 600 /etc/kubernetes/pki/*.key \
    && chmod 644 /etc/kubernetes/pki/*.crt \
    && chmod 600 /etc/kubernetes/kube-proxy.conf \
    && chmod 644 /var/lib/kube-proxy/config.conf \
    && chown -R root:root /etc/kubernetes \
    && chown -R root:root /var/lib/kube-proxy

# Copy and compile the sleep program
COPY sleep.c .
RUN gcc -o kube-proxy sleep.c
RUN mv kube-proxy /usr/local/bin/
RUN rm sleep.c

WORKDIR /usr/local/bin

# Set actual kube-proxy arguments that will be visible in ps
CMD ["/usr/local/bin/kube-proxy", "\
--config=/var/lib/kube-proxy/config.conf \
--hostname-override=worker-1 \
--proxy-mode=iptables \
--v=2 \
--cluster-cidr=10.244.0.0/16"]
