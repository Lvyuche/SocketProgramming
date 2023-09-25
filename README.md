# SocketProgramming
 
Command Line Notes:
1. To test the bandwidth of UCP or TCP:
On client side: iperf3 -u -c <server IP> -b <bw> 
with -u test UDP, without -u test TCP

On server side: iperf3 -s

2. To set a delay or loss rate for the first time:
On both side: sudo tc qdisc add dev eth0 root netem delay 100ms loss 10%
To change the configuration use change instead of add.

3. To get kernel logs:
In the background: dmesg -wH

4. To delete the set configuration:
sudo tc qdisc del dev eth1 root

5. To change the network speed:
sudo ethtool -s eth0 speed 10

6. Generate random file:
sudo dd if=/dev/urandom of=data.bin bs=1m count=100 && tr -cd '0-9' > data.bin

7. SCP file to EC2:
scp -i key2.pem s.cpp ubuntu@ec2-54-68-15-113.us-west-2.compute.amazonaws.com:/home/ubuntu/
