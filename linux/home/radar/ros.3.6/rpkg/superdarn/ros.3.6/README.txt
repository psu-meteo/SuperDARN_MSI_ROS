RST $project$ $package$ $version$ ($date$) 
==========================================
R.J.Barnes

Version Log
===========

3.1	First release of ROS 3



Introduction
------------

The RST Radar Operating Software (ROS) is the software used to
operate the SuperDARN radar network. The software is distributed in
the form of source code, data tables and pre-compiled libraries and binaries
for various platforms.

This package does not include the hardware interface software, instead
it contains a "dummy" radar task that simulates the responses of the radar
hardware.

Features
--------

The RST/ROS package includes the following software:

+ radar 	 - A radar simulator
+ errlog	 - The error logger
+ fitacfwrite	 - Stores fitacf data files
+ rawacfwrite	 - Stores rawacf data files
+ iqwrite	 - Stores iqdat data files
+ rtserver	 - Real-time data server for TCP/IP clients
+ shellserver	 - Provides an interface between the control program
  		   and the interactive shell
+ radarshell	 - The interactive radar shell that allows control program
  		   parameters to be changed
+ scheduler	 - The radar scheduler

The package also includes some sample control programs and site libraries.

Installation
------------

1. Linux self-extracting archive:
   "$project$-$package$.$version$.darwin.sh"

Copy the self extracting archive to the directory that you wish to install 
and execute the archive:

. $project$-$package$.$version$.linux.sh

This will create an "rst" sub-directory containing the software.  

The software requires a number of environment variables that are defined in
the file "rst/profile.$project$-$package$.linux.bash". Edit this file as
needed and either include or copy it into your profile.

Recompiling the code
--------------------

The software is shipped with pre-compiled libraries and binaries. If
you need to recompile the software at any time; first recompile the 
build architecture by typing:

make.build

To recompile the software type:

make.code $project$ $package$

Logs of the compilation process are stored in the "log" sub-directory.


Data Directories
----------------

The directories in which data and log files are recorded are defined by 
environment variables. By default the following directories are used:

/data/ros/rawacf       - rawacf files
/data/ros/iqdat	       - iqdat files
/data/ros/fitacf       - fitacf files
/data/ros/errlog       - error logs
/data/ros/scdlog       - scheduler logs


Running the radar
-----------------

Three scripts control the operation of the radar:

start.radar   - starts the supporting tasks and the dummy radar process
start.scd     - starts the scheduler and loads radar.scd
stop.radar    - stops the supporting tasks







