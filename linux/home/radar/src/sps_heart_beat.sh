#!/bin/bash
# Heartbeat script for SuperDARN radar ops
# 1) Detects when fitacf files are no longer being written
#   1.1) Emails alert after 10 minutes
#   1.2) Restarts all radar processes after 2 hours
# 2) Detects if ROS server is inaccessible
#   2.1) Emails Alert every 8 hours
####################################

####################################
# Variables to adjust for each radar
####################################
DIR="/data/ros/fitacf/"
QNX_ROS_STOP="ssh root@sps-ros /root/current_ros/stop.ros"
QNX_ROS_START="ssh root@sps-ros /root/current_ros/start.ros"
QNX_REBOOT=""
LNX_ROS_STOP="stop.radar"
LNX_ROS_START="start.radar"
LNX_ROS_REBOOT=""
QNX_TEST_CMD="ssh root@sps-ros  date -u"

EMAIL_RADAR="SPS"
EMAIL_FROM="SPS_radar_ops@superdarn.gi.alaska.edu"
EMAIL_TO="jdspaleta@alaska.edu"
EMAIL_CC="wabristow@alaska.edu, CUSPLab@usap.gov"
EMAIL_ALERT_FILE="$HOME/radops_email_alart.txt"

RADAR_RUNNING_FILE="$HOME/radops_active.txt"

####################################
# Script operation starts here
#################################### 
source ${HOME}/.bash_profile

if [ -e $EMAIL_ALERT_FILE ]
then
  emtime=`stat -c %Y ${EMAIL_ALERT_FILE}`
else
  touch -t 197001010000 -m ${EMAIL_ALERT_FILE}
fi
cdir=`pwd`
cd ${DIR}
errcond=$?
if [ $errcond -ne 0 ]
then
  echo "cd error: $DIR"
  exit $errcond
fi
  
file=`ls -rc *.fitacf|tail -n 1`
echo $file
fullpath="${DIR}/${file}"
echo $fullpath
filemtime=`stat -c %Y $fullpath`
errcond=$?
if [ $errcond -ne 0 ]
then
  echo "stat error: $fullpath"
  exit $errcond
fi

currtime=`date +%s`
errcond=$?
if [ $errcond -ne 0 ]
then
  echo "date error:"
  exit $errcond
fi

#echo "$currtime  $filemtime"
diff=$(( (currtime - filemtime) ))
echo "FitACF write Elapsed Sec:" $diff

echo "Check to see if radar ops are suppose to be running:"
if [ -e $RADAR_RUNNING_FILE ]
then
  echo "  Radarops should be running"
  echo "  Proceeding to check for FITACF file writes as operational heartbeat"
else
  echo "  Radarops does not appear to be running"
  echo "  Exiting"
  exit
fi

echo "Running QNX test cmd : ${QNX_TEST_CMD}"
${QNX_TEST_CMD} > /dev/null 2>&1
errcond=$?
if [ $errcond -ne 0 ]
then
  echo "QNX test cmd error: ${QNX_TEST_CMD}"
  emtime=`stat -c %Y ${EMAIL_ALERT_FILE}`
  errcond=$?
  if [ $errcond -eq 0 ]
  then
    ediff=$(( (currtime - emtime) ))
    if [ $ediff -gt 28800 ]
    then
      echo "  Send email: ROS Access Alert"
      EMAIL_SUBJECT="${EMAIL_RADAR} : RADAR ROS Access Alert"
      mail -s "${EMAIL_SUBJECT}" -c "${EMAIL_CC}" "${EMAIL_TO}" << END_MAIL
Alert: Radar Unable to ssh into ROS machine 
END_MAIL
     touch -m ${EMAIL_ALERT_FILE}
    else
      echo "  Last Email Alert Elapsed Secs:" $ediff
      echo "  Alert Sent in the last 8 hours"
    fi
  else
    touch -t 197001010000 -m ${EMAIL_ALERT_FILE}
  fi
  exit $errcond
fi

#echo "Commands to restart of qnx processes:"
#echo "${QNX_ROS_STOP}"
#echo "${QNX_ROS_START}"
#echo "Command to reboot of qnx processes:"
#echo "${QNX_ROS_REBOOT}"
#echo "Commands to restart of lnx processes:"
#echo "${LNX_ROS_STOP}"
#echo "${LNX_ROS_START}"
#echo "Command to reboot of lnx processes:"
#echo "${LNX_ROS_REBOOT}"

if [ $diff -gt 7200 ]
then
	  echo "Last fitacf write over 2 hours ago"
          emtime=`stat -c %Y ${EMAIL_ALERT_FILE}`
          errcond=$?
          if [ $errcond -eq 0 ]
          then
            ediff=$(( (currtime - emtime) ))
            if [ $ediff -gt 3600 ]
            then
              touch -m ${EMAIL_ALERT_FILE}
	      echo "  Send email"
              EMAIL_SUBJECT="${EMAIL_RADAR} : RADAR Restart Alert"
              mail -s "${EMAIL_SUBJECT}" -c "${EMAIL_CC}" "${EMAIL_TO}" << END_MAIL
Alert: Radar Auto-restart initiated  
END_MAIL
	      echo "  Initiating restart of qnx processes: "
              echo ${QNX_ROS_STOP}
              ${QNX_ROS_STOP}
              echo ${QNX_ROS_START}
              ${QNX_ROS_START}
	      sleep 10 
	      echo "  Initiating restart of lnx processes  "
              echo ${LNX_ROS_STOP}
              ${LNX_ROS_STOP}
              echo ${LNX_ROS_START}
              ${LNX_ROS_START}
            else
              echo "  Last Email Alert Elapsed Secs:" $ediff
	      echo "  Alert sent within the last 1 hours"
            fi
            exit
          fi
fi
if [ $diff -gt 600 ]
then
	  echo "Last fitacf write over 10 minutes ago"
          emtime=`stat -c %Y ${EMAIL_ALERT_FILE}`
          errcond=$?
          if [ $errcond -eq 0 ]
          then
            ediff=$(( (currtime - emtime) ))
            if [ $ediff -gt 7200 ]
            then
	      echo "  Send email"
              EMAIL_SUBJECT="${EMAIL_RADAR} : RADAR Crash Alert"
              mail -s "${EMAIL_SUBJECT}" -c "${EMAIL_CC}" "${EMAIL_TO}" << END_MAIL
Alert:  Radar Operations appears to be hung
END_MAIL
              touch -m ${EMAIL_ALERT_FILE}
            else 
              echo "  Last Email Alert Elapsed Secs:" $ediff
	      echo "  Alert sent within the last 2 hours"
            fi
          else
	    echo "  No alert file"
            touch -t 197001010000 -m ${EMAIL_ALERT_FILE}
          fi

          exit
fi
if [ $diff -gt 60 ]
then
	  echo "Last fitacf write over 1 minute ago"
          exit
fi

echo "Operations nominal"

