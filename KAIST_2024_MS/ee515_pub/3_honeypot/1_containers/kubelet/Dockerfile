FROM ubuntu

# Install necessary packages
RUN apt-get update && apt-get install -y \
    openssl gcc \
    && rm -rf /var/lib/apt/lists/*

# Create directories for kubelet
RUN mkdir -p /etc/kubernetes/pki \
    && mkdir -p /etc/kubernetes/manifests \
    && mkdir -p /var/lib/kubelet \
    && mkdir -p /var/log/kubernetes \
    && mkdir -p /var/lib/kubelet/pki \
    && mkdir -p /etc/systemd/system/kubelet.service.d \
    && mkdir -p /var/lib/kubeadm

# First create the CA certificates (similar to your etcd setup)
RUN openssl genrsa -out /etc/kubernetes/pki/ca.key 2048 \
    && openssl req -x509 -new -nodes -key /etc/kubernetes/pki/ca.key -sha256 -days 1024 \
        -out /etc/kubernetes/pki/ca.crt -subj "/CN=kubernetes-ca"

# Now generate kubelet certificates using the CA we just created
RUN openssl genrsa -out /var/lib/kubelet/pki/kubelet.key 2048 \
    && openssl req -new -key /var/lib/kubelet/pki/kubelet.key \
        -out /var/lib/kubelet/pki/kubelet.csr \
        -subj "/CN=system:node:$(hostname)/O=system:nodes" \
    && openssl x509 -req -in /var/lib/kubelet/pki/kubelet.csr \
        -CA /etc/kubernetes/pki/ca.crt \
        -CAkey /etc/kubernetes/pki/ca.key \
        -CAcreateserial \
        -out /var/lib/kubelet/pki/kubelet.crt \
        -days 365 -sha256

# Create kubelet configuration file with proper line continuation
RUN echo 'kind: KubeletConfiguration\n\
apiVersion: kubelet.config.k8s.io/v1beta1\n\
authentication:\n\
  anonymous:\n\
    enabled: false\n\
  webhook:\n\
    enabled: true\n\
  x509:\n\
    clientCAFile: /etc/kubernetes/pki/ca.crt\n\
authorization:\n\
  mode: Webhook\n\
clusterDomain: "cluster.local"\n\
clusterDNS:\n\
  - "10.96.0.10"\n\
runtimeRequestTimeout: "15m"\n\
tlsCertFile: "/var/lib/kubelet/pki/kubelet.crt"\n\
tlsPrivateKeyFile: "/var/lib/kubelet/pki/kubelet.key"\n\
registerNode: true' > /var/lib/kubelet/config.yaml

# Create mock kubelet.conf
RUN echo 'apiVersion: v1\n\
kind: Config\n\
clusters:\n\
- cluster:\n\
    certificate-authority: /etc/kubernetes/pki/ca.crt\n\
    server: https://localhost:6443\n\
  name: kubernetes\n\
contexts:\n\
- context:\n\
    cluster: kubernetes\n\
    user: system:node:mock-node\n\
  name: system:node:mock-node@kubernetes\n\
current-context: system:node:mock-node@kubernetes\n\
users:\n\
- name: system:node:mock-node\n\
  user:\n\
    client-certificate: /var/lib/kubelet/pki/kubelet.crt\n\
    client-key: /var/lib/kubelet/pki/kubelet.key' > /etc/kubernetes/kubelet.conf

# Create kubeadm flags file
RUN echo 'KUBELET_KUBEADM_ARGS="--container-runtime=remote --container-runtime-endpoint=/run/containerd/containerd.sock --pod-infra-container-image=k8s.gcr.io/pause:3.6"' > /var/lib/kubelet/kubeadm-flags.env

# Set environment variables
ENV KUBELET_CONFIG_FILE=/var/lib/kubelet/config.yaml
ENV NODE_NAME=worker-1
ENV KUBELET_EXTRA_ARGS="--fail-swap-on=false --cloud-provider=external"
ENV KUBELET_CGROUP_DRIVER=systemd

# Set proper permissions
RUN chmod 700 /var/lib/kubelet \
    && chmod -R 644 /var/lib/kubelet/config.yaml \
    && chmod -R 644 /var/lib/kubelet/kubeadm-flags.env \
    && chmod 600 /var/lib/kubelet/pki/kubelet.key \
    && chmod 644 /var/lib/kubelet/pki/kubelet.crt \
    && chmod 600 /etc/kubernetes/pki/ca.key \
    && chmod 644 /etc/kubernetes/pki/ca.crt \
    && chmod 600 /etc/kubernetes/kubelet.conf \
    && chown -R root:root /var/lib/kubelet \
    && chown -R root:root /etc/kubernetes

# Copy and compile the sleep program
COPY sleep.c .
RUN gcc -o kubelet sleep.c
RUN mv kubelet /usr/local/bin/
RUN rm sleep.c

WORKDIR /usr/local/bin

# Set actual kubelet arguments that will be visible in ps
CMD ["/usr/local/bin/kubelet", "\
--config=/var/lib/kubelet/config.yaml \
--hostname-override=worker-1 \
--kubeconfig=/etc/kubernetes/kubelet.conf \
--root-dir=/var/lib/kubelet \
--v=2 \
--container-runtime=remote \
--container-runtime-endpoint=/run/containerd/containerd.sock \
--pod-infra-container-image=k8s.gcr.io/pause:3.6 \
--fail-swap-on=false \
--cloud-provider=external \
--cgroup-driver=systemd"]
