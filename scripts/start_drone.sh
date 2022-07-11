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
roslaunch marsupial_launchers os1_sensor.launch  


# roslaunch ouster_ros os1.launch os1_hostname:=10.5.5.97 os1_udp_dest:=10.5.5.1 lidar_mode:=1024x20 

# roslaunch mbzirc_launch perception_mbzirc.launch &
# roslaunch mbzirc_launch full_planner_mbzirc.launch &
# roslaunch siar_driver start.launch publish_tf:=true



#sleep 8
#rosrun sparrow pound --node-id 0


 # #!/bin/bash
