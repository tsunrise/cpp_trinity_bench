cd /mntData/tpch_split_600
sudo mv x00 x0
sudo mv x01 x1
sudo mv x02 x2
sudo mv x03 x3
sudo mv x04 x4 
sudo mv x05 x5 
sudo mv x06 x6 
sudo mv x07 x7 
sudo mv x08 x8 
sudo mv x09 x9 
for i in {90..599}
do
    sudo mv "x$(( 8910 + $i ))" "x$(( $i ))"
done