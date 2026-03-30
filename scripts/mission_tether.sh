#!/bin/bash
roslaunch marsupial_mission_interface mission_execution.launch map_name:=rellano45_withobst mission_file:=rellano45_1 \
          able_tracker_uav:=false able_tracker_tether:=true able_tracker_ugv:=false
