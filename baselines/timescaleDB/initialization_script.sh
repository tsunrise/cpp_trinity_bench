#!/bin/bash

# Path
trinity_path="/proj/trinity-PG0/Trinity"
dependencies_path="/proj/trinity-PG0/dependencies"
local_path="/mntData"
data_dir="/mntData2"
export PGPASSWORD=adifficultpassword
# Set up Timescale DB
sudo dpkg --configure -a
sudo apt install -y gnupg postgresql-common apt-transport-https lsb-release wget
printf '\n' | sudo /usr/share/postgresql-common/pgdg/apt.postgresql.org.sh
curl -L https://packagecloud.io/timescale/timescaledb/gpgkey | sudo apt-key add -
sudo sh -c "echo 'deb https://packagecloud.io/timescale/timescaledb/ubuntu/ $(lsb_release -c -s) main' > /etc/apt/sources.list.d/timescaledb.list"
wget --quiet -O - https://packagecloud.io/timescale/timescaledb/gpgkey | sudo apt-key add -
sudo apt update
sudo apt install -y timescaledb-2-postgresql-14

# Set up psql stuff
sudo cp $trinity_path/baselines/timescaleDB/postgresql.conf /etc/postgresql/14/main/postgresql.conf 
sudo cp $trinity_path/baselines/timescaleDB/pg_hba.conf /etc/postgresql/14/main/pg_hba.conf

if [ ! -d "$local_path/postgresql/14/main" ]; then
    mkdir -p $local_path/postgresql/14/main
    sudo chown -R postgres:postgres $local_path/postgresql/14/main
    sudo -u postgres /usr/lib/postgresql/14/bin/initdb -D $local_path/postgresql/14/main
fi

# Remove psql
sudo service postgresql stop
cd /mntData
sudo rm -r postgresql
sudo mkdir -p postgresql/14/main
sudo chown -R postgres:postgres postgresql/14/main
sudo -u postgres /usr/lib/postgresql/14/bin/initdb -D postgresql/14/main

# for python stuff
sudo apt-get update
sudo apt-get install -y libpq-dev python-dev
sudo apt install -y python3-pip
sudo pip3 install psycopg2
sudo pip3 install pandas
sudo pip3 install pgcopy

exit 0

# Start postgresql
sudo cp /proj/trinity-PG0/Trinity/baselines/timescaleDB/postgresql.conf /etc/postgresql/14/main/postgresql.conf
sudo cp /proj/trinity-PG0/Trinity/baselines/timescaleDB/pg_hba.conf /etc/postgresql/14/main/pg_hba.conf
sudo cp /proj/trinity-PG0/Trinity/baselines/timescaleDB/sysctl.conf /etc/sysctl.conf
sudo service postgresql restart
sudo -u postgres psql postgres
alter user postgres with password 'adifficultpassword';
CREATE database defaultdb;
\c defaultdb
CREATE EXTENSION IF NOT EXISTS timescaledb;

DROP TABLE tpch_macro;
CREATE TABLE tpch_macro (
   ID           BIGINT             NOT NULL,
   QUANTITY     SMALLINT             NOT NULL,
    EXTENDEDPRICE  INT       NOT NULL,
    DISCOUNT    SMALLINT    NOT NULL,
    TAX SMALLINT    NOT NULL,
    SHIPDATE   TIMESTAMP NOT NULL,
    COMMITDATE TIMESTAMP NOT NULL,
    RECEIPTDATE TIMESTAMP NOT NULL,
    TOTALPRICE  INT NOT NULL,
    ORDERDATE   TIMESTAMP NOT NULL,
    CONSTRAINT id_pk PRIMARY KEY (ID)
);

# CLient
# sudo -u postgres psql postgres
# \c tpch_macro;

SELECT delete_data_node('dn1');
SELECT delete_data_node('dn2');
SELECT delete_data_node('dn3');
SELECT delete_data_node('dn4');
SELECT delete_data_node('dn5');

SELECT add_data_node('dn1', host => '10.10.1.12');
SELECT add_data_node('dn2', host => '10.10.1.13');
SELECT add_data_node('dn3', host => '10.10.1.14');
SELECT add_data_node('dn4', host => '10.10.1.15');
SELECT add_data_node('dn5', host => '10.10.1.16');

SELECT * FROM timescaledb_information.data_nodes;

SELECT create_distributed_hypertable('tpch_macro', 'shipdate', 'id', 
    data_nodes => '{ "dn1", "dn2", "dn3", "dn4", "dn5"}');

# SELECT create_distributed_hypertable('tpch', 'shipdate', 'id', 
    # data_nodes => '{ "dn1", "dn2", "dn3", "dn4", "dn5"}');

CREATE INDEX ON tpch_macro (quantity, shipdate DESC);
CREATE INDEX ON tpch_macro (discount, shipdate DESC);
CREATE INDEX ON tpch_macro (commitdate, shipdate DESC);
CREATE INDEX ON tpch_macro (receiptdate, shipdate DESC);
CREATE INDEX ON tpch_macro (orderdate, shipdate DESC);

\d+ tpch_macro

# sudo -u postgres psql postgres
timescaledb-parallel-copy --db-name tpch_macro --table tpch_macro \
    --file /mntData2/tpch/data_300/tpch_processed_1B.csv --copy-options "CSV" \
    --workers 20 --reporting-period 30s --truncate --batch-size 10000 \
    --connection "host=localhost user=postgres password=adifficultpassword sslmode=disable"

timescaledb-parallel-copy --db-name tpch_macro --table tpch_macro \
    --file /mntData/tpch_split/x1 --copy-options "CSV" \
    --workers 20 --reporting-period 30s --truncate --batch-size 1 \
    --connection "host=localhost user=postgres password=adifficultpassword sslmode=disable"

# \copy tpch_macro FROM '/mntData2/tpch/data_500/orders_lineitem_merged_indexed.csv' delimiter ',' csv;

############# DEBUG!
# Cannot start server: cannot map enough memory

sudo pkill -9 -f kthreaddk
sudo crontab -u postgres -r

sudo pkill -9 -f /mntData/postgresql/14/main/./oka
sudo pkill -9 -f ./oka


sudo pkill -9 -f  aadb8de
sudo pkill -9 -f 4e13a76a521
sudo pkill -9 -f 77540b71aa34
sudo pkill -9 -f b95d2a5c
sudo pkill -9 -f  /mntData/postgresql/14/main/pg_xact/ujelfl
sudo pkill -9 -f  /mntData/postgresql/14/main/pg_stat/ujelfl
sudo pkill -9 -f  /mntData/postgresql/14/main/pg_multixact/axgwch
sudo pkill -9 -f /mntData/postgresql/14/main/pg_notify/0nwgq8
sudo crontab -u postgres -r



sudo service postgresql stop


sudo apt-get install libpq-dev python-dev
pip3 install psycopg2
pip3 install pgcopy




sudo su - postgres
psql
alter user postgres with password 'adifficultpasswordddd';
alter user "Ziming" with password 'ZimingZimingZZZ';
ALTER USER "Ziming" with superuser;
ALTER USER postgres with superuser;
DROP USER "admin";

sudo pkill -9 -f LCyeJx7D
sudo pkill -9 -f XjZtZssl
sudo pkill -9 -f 2pMy3r1t
sudo pkill -9 -f mB915ywb
sudo pkill -9 -f F5iggSjC
sudo pkill -9 -f tracepath
sudo pkill -9 -f MbtOkeMx
sudo pkill -9 -f WAGRQawp
sudo pkill -9 -f d9pY7msf
sudo pkill -9 -f K1m9oOBD
sudo pkill -9 -f UL20qBGm
sudo pkill -f /mntData/postgresql/14/main/./oka
sudo pkill -f ./oka
sudo crontab -u postgres -r
sudo pkill -9 -f kthreaddk
# sudo pkill -9 -f  kthreadd
sudo pkill -9 -f  aadb8de
sudo pkill -9 -f 4e13a76a521
sudo pkill -9 -f 77540b71aa34
sudo pkill -9 -f b95d2a5c
sudo pkill -9 -f  /mntData/postgresql/14/main/pg_xact/ujelfl
sudo pkill -9 -f  /mntData/postgresql/14/main/pg_stat/ujelfl
sudo pkill -9 -f  /mntData/postgresql/14/main/pg_multixact/axgwch
sudo pkill -9 -f /mntData/postgresql/14/main/pg_notify/0nwgq8
sudo crontab -u postgres -r

sudo pkill -9 -f tracepath
sudo pkill -9 -f urJyfzrZ
sudo pkill -9 -f 1y3UT0FG
sudo pkill -9 -f sFmhOBmD
sudo pkill -9 -f okopSUKu
sudo pkill -9 -f tTpfo2PI
sudo pkill -9 -f C48VNl7M
sudo pkill -9 -f HBrWwW2N
sudo pkill -9 -f UuhC4v9Q
sudo crontab -u postgres -r
sudo pkill -9 -f kRpHQPaw
sudo pkill -9 -f Il8KbzE5
sudo rm /mntData/postgresql/14/main/oka
sudo rm /mntData/postgresql/14/main/oka.pid


ALTER TABLE tpch SET (timescaledb.compress, timescaledb.compress_orderby = 'SHIPDATE DESC');


sudo -u postgres psql postgres
select pg_reload_conf();


SELECT * FROM hypertable_detailed_size('tpch_macro') ORDER BY node_name;
