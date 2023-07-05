#!/bin/bash

# Run from Cloudlab
# git clone https://github.com/MaoZiming/Trinity.git
# bash /proj/trinity-PG0/Trinity/scripts/setup_one_node.sh

# Update Github
git config --global user.name "MaoZiming"
git config --global user.email "ziming.mao@yale.edu"

# Path
dependencies_path="/proj/trinity-PG0/dependencies"

# Basic setup
sudo apt update
# sudo apt upgrade
sudo apt install htop
sudo apt-get install dstat
cd $dependencies_path

# Install Cmake
if [ ! -d "cmake-3.23.0-rc5-linux-x86_64" ]; then
    wget -nc https://github.com/Kitware/CMake/releases/download/v3.23.0-rc5/cmake-3.23.0-rc5-linux-x86_64.tar.gz
    tar -xvf cmake-3.23.0-rc5-linux-x86_64.tar.gz
fi
sudo cp -r cmake-3.23.0-rc5-linux-x86_64/* /usr 
PATH=/usr/:$PATH
cd $dependencies_path

# Install libevent
if [ ! -d "libevent-2.1.10-stable" ]; then
    wget -nc  https://github.com/libevent/libevent/releases/download/release-2.1.10-stable/libevent-2.1.10-stable.tar.gz
    tar -xvf libevent-2.1.10-stable.tar.gz
    cd libevent-2.1.10-stable
    sudo ./configure --prefix=/usr 
    sudo make -j
    cd ..
fi
cd libevent-2.1.10-stable
sudo make install
cd $dependencies_path

# Install thrift
if [ ! -d "thrift"]; then
    git clone -b 0.15.0 https://github.com/apache/thrift.git
    cd thrift
    ./bootstrap.sh
    sudo ./configure --without-erlang
    sudo make -j
    cd ..
fi
cd thrift
sudo make install
cd $dependencies_path

# Install Other Packages for Trinity
sudo apt-get install -y libboost-test-dev  
sudo apt-get install -y  libboost-all-dev
sudo apt-get install -y  curl
sudo apt-get install -y  libssl-dev libcurl4-openssl-dev
sudo apt install -y libboost-thread-dev
sudo apt-get install -y libbz2-dev
sudo apt-get install -y python3-dev  # for python3.x installs
sudo apt-get install -y libevent-dev
sudo apt-get install -y clang-format

# Other Config / installation
sudo apt install -y python3-pip
pip3 install pandas
echo 'include /etc/ld.so.conf.d/*.conf /usr/local/lib' | sudo tee /etc/ld.so.conf
sudo /sbin/ldconfig
# ulimit -n 163840
ulimit -n 100000
sudo rm -r /etc/security/limits.d/
sudo cp /proj/trinity-PG0/Trinity/scripts/limits.conf  /etc/security/limits.conf
sudo cp /proj/trinity-PG0/Trinity/scripts/user.conf  /etc/systemd/user.conf
sudo cp /proj/trinity-PG0/Trinity/scripts/system.conf  /etc/security/system.conf

# Golang
cd $dependencies_path
cd go
if [ ! -d "go1.18.3.linux-amd64.tar.gz" ]; then
    wget https://go.dev/dl/go1.18.3.linux-amd64.tar.gz
fi
sudo tar -C /usr/bin -xzf go1.18.3.linux-amd64.tar.gz
export PATH=$PATH:/usr/bin/go/bin
cd $dependencies_path

sudo apt-get install -y maven
sudo apt-get install -y python3-pip

sudo mkdir -p /mntData/tpch
sudo chmod 775 /mntData/tpch
sudo apt -y install pixz

exit 0

# FILE DESCRIPTOR!!
# https://unix.stackexchange.com/a/691947
# Why is VScode terminal showing different behaviors