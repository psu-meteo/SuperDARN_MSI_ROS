# .bashrc

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi

# User specific aliases and functions
PATH=$PATH:$HOME/bin

export PATH

# KSR ROS operational : 20130906
export ROSHOST="192.168.0.4"
export RSTPATH=$HOME/ros.3.6/
source $RSTPATH/.profile.bash

export EDITOR=vi

