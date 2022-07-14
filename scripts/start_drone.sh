#!/bin/bash
source /home/drone/marsupial_ws/devel/setup.bash


ip addr flush dev eno1
sleep 1
sudo ip addr add 192.168.10.1/24 dev eno1
sleep 1
sudo ip link set eno1 up
sleep 1
sudo dnsmasq -C /dev/null -kd -F 192.168.10.20,192.168.10.21 -i eno1 --bind-dynamic &
sleep 8

roscore &
sleep 2
roslaunch marsupial_launchers sdk.launch &
roslaunch marsupial_launchers sensors.launch  &

sleep 10
roslaunch marsupial_launchers dll.launch

