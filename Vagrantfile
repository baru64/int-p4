# -*- mode: ruby -*-
# vi: set ft=ruby :
Vagrant.configure("2") do |config|
  config.vm.box = "ubuntu/xenial64"
  config.vm.network "forwarded_port", guest: 9090, host: 9090, host_ip: "127.0.0.1"
  config.vm.network "forwarded_port", guest: 3000, host: 3000, host_ip: "127.0.0.1"
  config.vm.network "private_network", ip: "192.168.33.10"
  config.vm.provider "virtualbox" do |vb|
    vb.memory = "2048"
    vb.cpus = "2"
  end
  config.vm.provision "shell", inline: <<-SHELL
    apt-get update
    apt-get install -y linux-image-4.15.0-72-generic linux-headers-4.15.0-72-generic
    apt-get install -y docker.io
    curl -L "https://github.com/docker/compose/releases/download/1.25.0/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    chmod +x /usr/local/bin/docker-compose
    git clone https://github.com/baru64/p4app --recursive
    git clone https://github.com/baru64/int-p4
    cd p4app
    docker build -t baru64/int_p4app:0.1 .
    cp /home/vagrant/int-p4/p4app /usr/local/bin/p4app 
  SHELL
end
