# .bashrc

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi

# User specific aliases and functions
PATH=$PATH:$HOME/bin

export PATH

# ROS operational environment parameters 
# Should be adjusted
export ROSHOST="127.0.0.1"
export RSTPATH=$HOME/ros.3.6/
source $RSTPATH/.profile.bash

export EDITOR=vi

