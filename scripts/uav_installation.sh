#!/bin/sh

read -p "Installing optimizer for marsupial robot configuration based in CERES_SOLVER. Press a ENTER to contine. (CTRL+C) to cancel"

#! /bin/bash
sudo apt-get install ros-$ROS_DISTRO-octomap-msgs ros-$ROS_DISTRO-octomap-ros -y
sudo apt-get install ros-$ROS_DISTRO-costmap-2d ros-$ROS_DISTRO-octomap-server ros-$ROS_DISTRO-octomap-mapping -y
sudo apt-get intall libpcl-dev libpcl libpcl-kdtree1.10

cd ~
mkdir ~/results_marsupial_optimizer
cd ~
mkdir ~/residuals_optimization_data
cd ~
mkdir -p ~/marsupial_ws/src
cd ~/marsupial_ws/
catkin_make
cd ~/marsupial_ws/src

# For localization stuff
echo Downloading localization packages: DLL, Odom 2 Tf, ALoam
git clone -devel_marsupial https://github.com/robotics-upo/dll.git
git clone https://github.com/robotics-upo/odom_to_tf.git
git clone https://github.com/

# Mav comms
echo "\n Installing mav_comm \n\n"
git clone https://github.com/ethz-asl/mav_comm.git

# To get .bt form gazebo world
# sim_gazebo_plugins # Tengo que subir este paquete

#To install Behavior tree
## Dependencies for Behavior Tree
echo "\n Installing BT Dependencies \n\n"
sudo apt-get install ros-$ROS_DISTRO-ros-type-introspection -y
sudo apt-get install qtbase5-dev libqt5svg5-dev -y
sudo apt install libdw-dev -y
echo "\n Installing BT Packages \n\n"
# git clone https://github.com/robotics-upo/behavior-tree-upo-actions.git
git clone https://github.com/robotics-upo/BehaviorTree.CPP.git
git clone https://github.com/robotics-upo/Groot.git

# To install planner lazy-theta*
echo "\n Installing Lazy Theta* Planner \n\n"
git clone -b marsupial https://github.com/robotics-upo/lazy_theta_star_planners.git

# To install NIX_common package
echo "\n Installing NIX common package \n\n"
git clone https://github.com/robotics-upo/nix_common.git

# To install Random planners
echo "\n Installing Random Planner \n\n"
git clone -b develop https://github.com/SaimonMR/rrt_star_planners.git

# To install Marsupial Optimizer
echo "\n Installing Marsupial Optimizer \n\n"
git clone -b noetic https://github.com/robotics-upo/marsupial_optimizer.git

# To get action for actionlib
echo "\n Installing UPO Actions \n\n"
git clone https://github.com/robotics-upo/upo_actions.git

#T o get a marker in the desired frame_link
echo "\n Installing UPO Markers \n\n"
git clone https://github.com/robotics-upo/upo_markers.git


git clone -b melodic-devel https://github.com/MoriKen254/timed_roslaunch.git

# CERES Solver Installation
echo "\n Installing CERES \n\n"
## CMake
sudo apt-get install cmake
## google-glog + gflags
sudo apt-get install libgoogle-glog-dev libgflags-dev -y
## BLAS & LAPACK
sudo apt-get install libatlas-base-dev -y
## Eigen3
sudo apt-get install libeigen3-dev -y
## SuiteSparse and CXSparse (optional)
sudo apt-get install libsuitesparse-dev -y
cd ~
git clone https://ceres-solver.googlesource.com/ceres-solver
# tar zxf ceres-solver-2.0.0.tar.gz
mkdir ceres-bin
cd ceres-bin
# cmake ../ceres-solver-2.0.0
cmake ../ceres-solver
make -j3
make test
## Optionally install Ceres, it can also be exported using CMake which
## allows Ceres to be used without requiring installation, see the documentation
## for the EXPORT_BUILD_DIR option for more information.
sudo make install

## Install the welcome message to the ssh
roscd marsupial_launcher/cfg
sudo cp welcome.txt /etc/motd

