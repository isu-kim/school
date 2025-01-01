FROM ubuntu

# Install necessary packages
RUN apt-get update && apt-get install -y \
    openssl gcc \
    && rm -rf /var/lib/apt/lists/*

# Create directories for certificates and configuration
RUN mkdir -p /etc/kubernetes/pki/etcd \
    && mkdir -p /var/lib/etcd \
    && mkdir -p /var/lib/etcd/member/snap \
    && mkdir -p /var/lib/etcd/member/wal \
    && mkdir -p /var/lib/etcd-backup

# Create mock etcd data files
RUN dd if=/dev/zero of=/var/lib/etcd/member/snap/db bs=1M count=32 \
    && dd if=/dev/zero of=/var/lib/etcd/member/wal/0000000000000000-0000000000000000.wal bs=1M count=1 \
    && dd if=/dev/zero of=/var/lib/etcd/member/wal/0000000000000001-0000000000000001.wal bs=1M count=1

# Create version file
RUN echo '{"etcdserver":"3.5.9","etcdcluster":"3.5.0"}' > /var/lib/etcd/version

# Create member file
RUN echo '{"name":"etcd-server","id":"1234567890"}' > /var/lib/etcd/member.txt

# Create etcd CA and server certificates
RUN openssl genrsa -out /etc/kubernetes/pki/etcd/ca.key 2048 \
    && openssl req -x509 -new -nodes -key /etc/kubernetes/pki/etcd/ca.key -sha256 -days 1024 -out /etc/kubernetes/pki/etcd/ca.crt -subj "/CN=etcd-ca" \
    && openssl genrsa -out /etc/kubernetes/pki/etcd/server.key 2048 \
    && openssl req -new -key /etc/kubernetes/pki/etcd/server.key -out /etc/kubernetes/pki/etcd/server.csr -subj "/CN=etcd-server" \
    && openssl x509 -req -in /etc/kubernetes/pki/etcd/server.csr -CA /etc/kubernetes/pki/etcd/ca.crt -CAkey /etc/kubernetes/pki/etcd/ca.key -CAcreateserial -out /etc/kubernetes/pki/etcd/server.crt -days 365 -sha256

# Create peer certificate
RUN openssl genrsa -out /etc/kubernetes/pki/etcd/peer.key 2048 \
    && openssl req -new -key /etc/kubernetes/pki/etcd/peer.key -out /etc/kubernetes/pki/etcd/peer.csr -subj "/CN=etcd-peer" \
    && openssl x509 -req -in /etc/kubernetes/pki/etcd/peer.csr -CA /etc/kubernetes/pki/etcd/ca.crt -CAkey /etc/kubernetes/pki/etcd/ca.key -CAcreateserial -out /etc/kubernetes/pki/etcd/peer.crt -days 365 -sha256

# Set environment variables
ENV ETCD_LISTEN_CLIENT_URLS=https://192.168.1.10:2379
ENV ETCD_LISTEN_PEER_URLS=https://192.168.1.10:2380
ENV ETCD_ADVERTISE_CLIENT_URLS=https://192.168.1.10:2379
ENV ETCD_INITIAL_ADVERTISE_PEER_URLS=https://192.168.1.10:2380
ENV ETCD_INITIAL_CLUSTER=etcd-server=https://192.168.1.10:2380
ENV ETCD_INITIAL_CLUSTER_TOKEN=etcd-cluster
ENV ETCD_INITIAL_CLUSTER_STATE=new
ENV ETCD_DATA_DIR=/var/lib/etcd
ENV ETCD_CERT_FILE=/etc/kubernetes/pki/etcd/server.crt
ENV ETCD_KEY_FILE=/etc/kubernetes/pki/etcd/server.key
ENV ETCD_PEER_CERT_FILE=/etc/kubernetes/pki/etcd/peer.crt
ENV ETCD_PEER_KEY_FILE=/etc/kubernetes/pki/etcd/peer.key
ENV ETCD_TRUSTED_CA_FILE=/etc/kubernetes/pki/etcd/ca.crt
ENV ETCD_PEER_TRUSTED_CA_FILE=/etc/kubernetes/pki/etcd/ca.crt
ENV ETCD_SNAPSHOT_COUNT=10000
ENV ETCD_AUTO_COMPACTION_RETENTION=8
ENV ETCD_QUOTA_BACKEND_BYTES=8589934592

# Set proper permissions
RUN chmod 700 /var/lib/etcd \
    && chmod 700 /var/lib/etcd/member \
    && chmod 700 /var/lib/etcd/member/snap \
    && chmod 700 /var/lib/etcd/member/wal \
    && chmod 600 /etc/kubernetes/pki/etcd/*.key \
    && chmod 644 /etc/kubernetes/pki/etcd/*.crt \
    && chmod 600 /var/lib/etcd/member/snap/db \
    && chmod 600 /var/lib/etcd/member/wal/*.wal \
    && chmod 600 /var/lib/etcd/version \
    && chmod 600 /var/lib/etcd/member \
    && chown -R root:root /var/lib/etcd \
    && chown -R root:root /etc/kubernetes

COPY sleep.c .
RUN gcc -o etcd sleep.c
RUN mv etcd /usr/local/bin/
RUN rm sleep.c

WORKDIR /usr/local/bin

# Set actual etcd arguments that will be visible in ps
CMD ["/usr/local/bin/etcd", "--name=etcd-server \
--data-dir=/var/lib/etcd \
--listen-client-urls=https://192.168.1.10:2379 \
--listen-peer-urls=https://192.168.1.10:2380 \
--advertise-client-urls=https://192.168.1.10:2379 \
--initial-advertise-peer-urls=https://192.168.1.10:2380 \
--initial-cluster=etcd-server=https://192.168.1.10:2380 \
--initial-cluster-token=etcd-cluster \
--initial-cluster-state=new \
--cert-file=/etc/kubernetes/pki/etcd/server.crt \
--key-file=/etc/kubernetes/pki/etcd/server.key \
--peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt \
--peer-key-file=/etc/kubernetes/pki/etcd/peer.key \
--trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt \
--peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt \
--snapshot-count=10000 \
--quota-backend-bytes=8589934592 \
--auto-compaction-retention=8"]
