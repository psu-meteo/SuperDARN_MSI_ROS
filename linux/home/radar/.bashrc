# .bashrc

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi

# User specific aliases and functions
PATH=$PATH:$HOME/bin

export PATH

# McM  ROS operational 
export ROSHOST="192.168.1.2"
export RSTPATH=$HOME/rst/
export LIBSTR="ros"
export SITE_CFG="$RSTPATH/tables/superdarn/site/"
source $RSTPATH/.profile.bash
#Old style RST stuff here
#export RSTPATH=$HOME/rst
#source $RSTPATH/profile.superdarn-rst.linux.bash
export EDITOR=vi
unset SSH_ASKPASS
