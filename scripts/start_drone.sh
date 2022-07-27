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

#Check if we have ping to the base station host:
BASE_STATION=srlmsi
sleep 10
ping -c1 srlmsi
if [ $? -eq 0 ]
then # If there is ping, wait for ros master
    export ROS_MASTER_URI=http://srlmsi:11311
    until rostopic list ; do echo "Waiting for roscore..."; sleep 1; done
else
    roscore &
    sleep 2
fi
roslaunch marsupial_launchers sdk.launch &
roslaunch marsupial_launchers sensors.launch  &
roslaunch matrice_traj_tracker matrice_marsupial_traj_tracker.launch &

sleep 10
roslaunch marsupial_launchers dll.launch 

