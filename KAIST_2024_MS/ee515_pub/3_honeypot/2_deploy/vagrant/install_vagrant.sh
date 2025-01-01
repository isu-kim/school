#!/bin/bash
echo ========= Installing Vagrant =========
wget -O - https://apt.releases.hashicorp.com/gpg | sudo gpg --dearmor -o /usr/share/keyrings/hashicorp-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/hashicorp-archive-keyring.gpg] https://apt.releases.hashicorp.com $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/hashicorp.list
sudo apt update && sudo apt install vagrant

echo ========= Installing libvirt =========
sudo apt-get install -y qemu-kvm virt-manager libvirt-daemon-system virtinst libvirt-clients bridge-utils qemu-system libvirt-daemon libvirt-dev
sudo systemctl enable libvirtd
sudo systemctl start libvirtd

echo ========= Installing libvirt Vagrant =========
vagrant plugin install vagrant-libvirt