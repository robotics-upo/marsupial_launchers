#!/bin/bash
sudo cp $HOME/marsupial_ws/src/marsupial_launchers/service/drone.service /etc/systemd/system
sudo cp $HOME/marsupial_ws/src/marsupial_launchers/scripts/start_drone.sh /usr/bin

sudo systemctl stop drone.service
sudo systemctl daemon-reload
sleep 2
sudo systemctl start drone.service