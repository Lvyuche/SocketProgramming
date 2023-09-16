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


