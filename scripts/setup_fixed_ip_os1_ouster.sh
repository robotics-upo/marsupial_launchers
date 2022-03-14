#!/bin/bash

read -p "IP static configuration for OS1 ouster. Press a ENTER to contine. (CTRL+C) to cancel"
read -p "Please unplug the ethernet cable to OS1 ouster"
ip addr flush dev eno1
ip addr show dev eno1
sudo ip addr add 192.168.10.1/24 dev eno1

read -p "Please plug the ethernet cable to OS1 ouster"
sudo ip link set eno1 up
ip addr show dev eno1
sudo dnsmasq -C /dev/null -kd -F 192.168.10.20,192.168.10.21 -i eno1 --bind-dynamic

# read -p "Check connection using PING"
# ping -c1 192.168.10.20
# ping -c os1-991939000687.local


# #!/bin/bash
