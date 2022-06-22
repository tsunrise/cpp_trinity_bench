#!/usr/bin/bash 

trinity_path="/proj/trinity-PG0/Trinity"
dependencies_path="/proj/trinity-PG0/dependencies"
local_path="/mntData"
data_dir="/mntData2"

cd $dependencies_path/aerospike

if [ ! -d "aerospike-server-community-5.7.0.17-ubuntu18.04" ]; then
    wget -O aerospike.tgz https://download.aerospike.com/artifacts/aerospike-server-community/5.7.0.17/aerospike-server-community-5.7.0.17-ubuntu18.04.tgz
    tar -xvf aerospike.tgz
fi
cd aerospike-server-community-5.7.0.17-ubuntu18.04
sudo ./asinstall
sudo cp /proj/trinity-PG0/Trinity/baselines/aerospike/aerospike.conf /etc/aerospike/aerospike.conf

cd $dependencies_path/aerospike
if [ ! -d "aerospike-loader" ]; then
    git clone https://github.com/aerospike/aerospike-loader.git
fi
cd aerospike-loader
sudo ./build
pip3 install aerospike
sudo mkdir -p /var/log/aerospike/
sudo chmod 775 /var/log/aerospike/
sudo touch /var/log/aerospike/aerospike.log
sudo mkdir -p /mntData/aerospike/

exit 0

# Control command
sudo systemctl start aerospike
sudo systemctl status aerospike
sudo systemctl restart aerospike
sudo systemctl stop aerospike
# Remember to update IP address
sudo cp /proj/trinity-PG0/Trinity/baselines/aerospike/aerospike.conf /etc/aerospike/aerospike.conf
# Force clear
sudo rm /mntData/aerospike/tpch

# asadm
enable

# Manage Indexes
manage sindex delete QUANTITY_index ns tpch
manage sindex delete EXTENDEDPRICE_index ns tpch
manage sindex delete DISCOUNT_index ns tpch
manage sindex delete TAX_index ns tpch
manage sindex delete SHIPDATE_index ns tpch
manage sindex delete COMMITDATE_index ns tpch
manage sindex delete RECEIPTDATE_index ns tpch
manage sindex delete TOTALPRICE_index ns tpch
manage sindex delete ORDERDATE_index ns tpch

manage sindex create numeric QUANTITY_index ns tpch set tpch_macro bin QUANTITY
manage sindex create numeric EXTENDEDPRICE_index ns tpch set tpch_macro bin EXTENDEDPRICE
manage sindex create numeric DISCOUNT_index ns tpch set tpch_macro bin DISCOUNT
manage sindex create numeric TAX_index ns tpch bin set tpch_macro bin TAX
manage sindex create numeric SHIPDATE_index ns tpch set tpch_macro bin SHIPDATE
manage sindex create numeric COMMITDATE_index ns tpch set tpch_macro bin COMMITDATE
manage sindex create numeric RECEIPTDATE_index ns tpch set tpch_macro bin RECEIPTDATE
manage sindex create numeric TOTALPRICE_index ns tpch set tpch_macro bin TOTALPRICE
manage sindex create numeric ORDERDATE_index ns tpch set tpch_macro bin ORDERDATE

# aql
SHOW SETS
TRUNCATE tpch.tpch_macro
SHOW INDEXES

# loader
/proj/trinity-PG0/dependencies/aerospike/aerospike-loader/run_loader -h localhost -n tpch -c /proj/trinity-PG0/Trinity/baselines/aerospike/column.json -wa CREATE_ONLY /mntData/tpch_split
# Might need to reinstall if loader issue
# sudo ./build

