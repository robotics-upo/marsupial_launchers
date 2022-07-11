#!/bin/bash
# Script that should be used whenever start_drone.sh is changed in order to update the service
sudo cp $HOME/marsupial_ws/src/marsupial_launchers/service/drone.service /etc/systemd/system
sudo cp $HOME/marsupial_ws/src/marsupial_launchers/scripts/start_drone.sh /usr/bin

sudo systemctl stop drone.service
sudo systemctl daemon-reload
sleep 2
sudo systemctl start drone.service