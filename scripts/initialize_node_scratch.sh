#!/bin/sh

# Run from Cloudlab
# git clone https://github.com/MaoZiming/Trinity.git
# sh /proj/Trinity/scripts/initialize_node_scratch.sh

# Update Github

trinity_path="/proj/trinity-PG0/Trinity/"
dependencies_path="/mntData2/dependencies/"
local_path="/mntData/"

cd "$trinity_path"
git config --global user.name "MaoZiming"
git config --global user.email "ziming.mao@yale.edu"
# git pull origin main

# Basic setup
sudo apt update
sudo apt install htop

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
    sudo ./configure
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

# Other Config / installation
sudo apt install -y python3-pip
pip3 install pandas
echo 'include /etc/ld.so.conf.d/*.conf /usr/local/lib' | sudo tee /etc/ld.so.conf
sudo /sbin/ldconfig
ulimit -n 16384

# Installing CLickhouse DB
pip3 install clickhouse-driver[lz4]
sudo apt-get install -y apt-transport-https ca-certificates dirmngr
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 8919F6BD2B48D754
echo "deb https://packages.clickhouse.com/deb stable main" | sudo tee /etc/apt/sources.list.d/clickhouse.list
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y clickhouse-server 
sudo apt-get install -y clickhouse-client
sudo timedatectl set-timezone America/New_York # Set time zone

# Configure clickhouse DB
sudo cp "$trinity_path"scripts/clickhouse_config.xml /etc/clickhouse-server/config.xml
sudo cp "$trinity_path"scripts/clickhouse_users.xml /etc/clickhouse-server/users.xml

# Set up Timescale DB
sudo dpkg --configure -a
sudo apt install -y gnupg postgresql-common apt-transport-https lsb-release wget
printf '\n' | sudo /usr/share/postgresql-common/pgdg/apt.postgresql.org.sh
curl -L https://packagecloud.io/timescale/timescaledb/gpgkey | sudo apt-key add -
sudo sh -c "echo 'deb https://packagecloud.io/timescale/timescaledb/ubuntu/ $(lsb_release -c -s) main' > /etc/apt/sources.list.d/timescaledb.list"
wget --quiet -O - https://packagecloud.io/timescale/timescaledb/gpgkey | sudo apt-key add -
sudo apt update
sudo apt install -y timescaledb-2-postgresql-14

# Set up clickhouse data
mkdir -p "$local_path"clickhouse/
sudo chown -R clickhouse:clickhouse "$local_path"clickhouse/

# Set up psql stuff
sudo cp "$trinity_path"scripts/postgresql.conf /etc/postgresql/14/main/postgresql.conf 
sudo cp "$trinity_path"scripts/pg_hba.conf /etc/postgresql/14/main/pg_hba.conf

if [ ! -d "'$local_path'postgresql/14/main" ]; then
    mkdir -p "$local_path"postgresql/14/main
    sudo chown -R postgres:postgres "$local_path"postgresql/14/main
    sudo -u postgres /usr/lib/postgresql/14/bin/initdb -D "$local_path"postgresql/14/main
fi
exit 0

# Start clickhouse
sudo service clickhouse-server start

# ClickHouse TPCH (Client Node)

clickhouse-client --database=default --query="DROP TABLE IF EXISTS tpch_macro";

clickhouse-client --database=default --query="CREATE TABLE IF NOT EXISTS tpch_macro (ID UInt32, QUANTITY UInt8, EXTENDEDPRICE UInt32, DISCOUNT UInt8, TAX UInt8, SHIPDATE UInt32, COMMITDATE UInt32, RECEIPTDATE UInt32, TOTALPRICE UInt32, ORDERDATE UInt32) ENGINE = Distributed(test_trinity, default, tpch_macro, rand())";

# ClickHouse TPCH (Data Node)

clickhouse-client --database=default --query="DROP TABLE IF EXISTS tpch_macro";

clickhouse-client --database=default --query="CREATE TABLE IF NOT EXISTS tpch_macro (ID UInt32, QUANTITY UInt8, EXTENDEDPRICE UInt32, DISCOUNT UInt8, TAX UInt8, SHIPDATE UInt32, COMMITDATE UInt32, RECEIPTDATE UInt32, TOTALPRICE UInt32, ORDERDATE UInt32) Engine = MergeTree ORDER BY (ID)";

# Clickhouse TPCH (insert Data)

cat /mntData2/data/tpch/data_500/orders_lineitem_merged_indexed.csv | clickhouse-client --query="INSERT INTO tpch_macro FORMAT CSV";

# Start postgresql
sudo service postgresql start
