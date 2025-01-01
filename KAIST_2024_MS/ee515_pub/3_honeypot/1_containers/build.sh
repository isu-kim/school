#!/bin/bash
echo "============ Starting Building Containers ============"

cd etcd && docker build -t "isukim/ee515_etcd:3.5.7-0" .
cd ..
cd kube-controller-manager && docker build -t "isukim/ee515_kube-controller-manager:v1.29.8" .
cd ..
cd kube-scheduler && docker build -t "isukim/ee515_kube-scheduler:v1.29.8" .
cd ..
cd kube-proxy && docker build -t "isukim/ee515_kube-proxy:v1.29.8" .
cd ..
cd kubelet && docker build -t "isukim/ee515_kubelet:v1.29.8" .
cd ..
cd kube-apiserver && docker build -t "isukim/ee515_kube-apiserver:v1.29.8" .
cd ..
cd privileged-container && docker build -t "isukim/ee515_kube-privileged-container:v1" .
cd ..

echo "============ Finished Building Containers ============"