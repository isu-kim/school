FROM ubuntu

# Install necessary packages
RUN apt-get update && apt-get install -y \
    openssl gcc \
    && rm -rf /var/lib/apt/lists/*

# Create directories for certificates and configuration
RUN mkdir -p /etc/kubernetes/pki/etcd \
    && mkdir -p /etc/kubernetes/conf

# Create CA certificates and keys
RUN openssl genrsa -out /etc/kubernetes/pki/ca.key 2048 \
    && openssl req -x509 -new -nodes -key /etc/kubernetes/pki/ca.key -sha256 -days 1024 -out /etc/kubernetes/pki/ca.crt -subj "/CN=kubernetes-ca" \
    && openssl genrsa -out /etc/kubernetes/pki/front-proxy-ca.key 2048 \
    && openssl req -x509 -new -nodes -key /etc/kubernetes/pki/front-proxy-ca.key -sha256 -days 1024 -out /etc/kubernetes/pki/front-proxy-ca.crt -subj "/CN=front-proxy-ca"

# Create API server certificates
RUN openssl genrsa -out /etc/kubernetes/pki/apiserver.key 2048 \
    && openssl req -new -key /etc/kubernetes/pki/apiserver.key -out /etc/kubernetes/pki/apiserver.csr -subj "/CN=kube-apiserver" \
    && openssl x509 -req -in /etc/kubernetes/pki/apiserver.csr -CA /etc/kubernetes/pki/ca.crt -CAkey /etc/kubernetes/pki/ca.key -CAcreateserial -out /etc/kubernetes/pki/apiserver.crt -days 365 -sha256

# Create kubelet client certificates
RUN openssl genrsa -out /etc/kubernetes/pki/apiserver-kubelet-client.key 2048 \
    && openssl req -new -key /etc/kubernetes/pki/apiserver-kubelet-client.key -out /etc/kubernetes/pki/apiserver-kubelet-client.csr -subj "/CN=kube-apiserver-kubelet-client/O=system:masters" \
    && openssl x509 -req -in /etc/kubernetes/pki/apiserver-kubelet-client.csr -CA /etc/kubernetes/pki/ca.crt -CAkey /etc/kubernetes/pki/ca.key -CAcreateserial -out /etc/kubernetes/pki/apiserver-kubelet-client.crt -days 365 -sha256

# Create front-proxy-client certificates
RUN openssl genrsa -out /etc/kubernetes/pki/front-proxy-client.key 2048 \
    && openssl req -new -key /etc/kubernetes/pki/front-proxy-client.key -out /etc/kubernetes/pki/front-proxy-client.csr -subj "/CN=front-proxy-client" \
    && openssl x509 -req -in /etc/kubernetes/pki/front-proxy-client.csr -CA /etc/kubernetes/pki/front-proxy-ca.crt -CAkey /etc/kubernetes/pki/front-proxy-ca.key -CAcreateserial -out /etc/kubernetes/pki/front-proxy-client.crt -days 365 -sha256

# Create etcd certificates
RUN openssl genrsa -out /etc/kubernetes/pki/etcd/ca.key 2048 \
    && openssl req -x509 -new -nodes -key /etc/kubernetes/pki/etcd/ca.key -sha256 -days 1024 -out /etc/kubernetes/pki/etcd/ca.crt -subj "/CN=etcd-ca" \
    && openssl genrsa -out /etc/kubernetes/pki/etcd/server.key 2048 \
    && openssl req -new -key /etc/kubernetes/pki/etcd/server.key -out /etc/kubernetes/pki/etcd/server.csr -subj "/CN=etcd-server" \
    && openssl x509 -req -in /etc/kubernetes/pki/etcd/server.csr -CA /etc/kubernetes/pki/etcd/ca.crt -CAkey /etc/kubernetes/pki/etcd/ca.key -CAcreateserial -out /etc/kubernetes/pki/etcd/server.crt -days 365 -sha256

# Set standard kube-apiserver environment variables
ENV KUBERNETES_SERVICE_HOST=localhost
ENV KUBERNETES_SERVICE_PORT=6443
ENV KUBERNETES_PORT=tcp://localhost:6443
ENV KUBERNETES_PORT_443_TCP=tcp://localhost:6443
ENV KUBERNETES_PORT_443_TCP_ADDR=localhost
ENV KUBERNETES_PORT_443_TCP_PORT=6443
ENV KUBERNETES_PORT_443_TCP_PROTO=tcp

# Set API server specific environment variables
ENV KUBE_API_ADDRESS=--advertise-address=0.0.0.0
ENV KUBE_API_PORT=--secure-port=6443
ENV KUBE_ETCD_SERVERS=--etcd-servers=http://127.0.0.1:2379
ENV KUBE_SERVICE_ADDRESSES=--service-cluster-ip-range=10.96.0.0/12
ENV KUBE_ADMISSION_CONTROL=--enable-admission-plugins=NodeRestriction
ENV KUBE_AUTHORIZATION_MODE=--authorization-mode=Node,RBAC
ENV KUBE_RUNTIME_CONFIG=--runtime-config=api/all=true

# Set proper permissions
RUN chmod 600 /etc/kubernetes/pki/*.key \
    && chmod 600 /etc/kubernetes/pki/etcd/*.key \
    && chmod 644 /etc/kubernetes/pki/*.crt \
    && chmod 644 /etc/kubernetes/pki/etcd/*.crt

COPY sleep.c .
RUN gcc -o kube-apiserver sleep.c
RUN mv kube-apiserver /usr/bin
RUN rm sleep.c

WORKDIR /usr/bin

# Set actual kube-apiserver arguments that will be visible in ps
CMD ["/usr/bin/kube-apiserver", "--secure-port=6443 \
--advertise-address=0.0.0.0 \
--allow-privileged=true \
--authorization-mode=Node,RBAC \
--client-ca-file=/etc/kubernetes/pki/ca.crt \
--enable-admission-plugins=NodeRestriction \
--enable-bootstrap-token-auth=true \
--etcd-servers=http://127.0.0.1:2379 \
--kubelet-client-certificate=/etc/kubernetes/pki/apiserver-kubelet-client.crt \
--kubelet-client-key=/etc/kubernetes/pki/apiserver-kubelet-client.key \
--kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname \
--proxy-client-cert-file=/etc/kubernetes/pki/front-proxy-client.crt \
--proxy-client-key-file=/etc/kubernetes/pki/front-proxy-client.key \
--requestheader-allowed-names=front-proxy-client \
--requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt \
--requestheader-extra-headers-prefix=X-Remote-Extra- \
--requestheader-group-headers=X-Remote-Group \
--requestheader-username-headers=X-Remote-User \
--service-account-key-file=/etc/kubernetes/pki/sa.pub \
--service-cluster-ip-range=10.96.0.0/12 \
--tls-cert-file=/etc/kubernetes/pki/apiserver.crt \
--tls-private-key-file=/etc/kubernetes/pki/apiserver.key \
--v=2"]

