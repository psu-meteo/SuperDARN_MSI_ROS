#!/bin/bash

# Wrapper script to compress larger data files in case
# network connection to the outside world is lost.
#
# Written by: Kevin Sterne

echo
echo "compress_wrapper.sh script starting at: "
echo `date`
echo

#Compress rawacf files
echo
echo "Compressing rawacf files..."
echo
/home/radar/ros.3.6/script/compress.sh ros/rawacf rawacf
sleep 5

#Compress iqdat files
echo
echo "Compressing iqdat files..."
echo
/home/radar/ros.3.6/script/compress.sh ros/iqdat iqdat
sleep 5

#Compress fitacf files
echo
echo "Compressing fitacf files..."
echo
/home/radar/ros.3.6/script/compress.sh ros/fitacf fitacf
sleep 5

echo
echo "compress_wrapper.sh script ending at: "
echo `date`
echo

exit 1
