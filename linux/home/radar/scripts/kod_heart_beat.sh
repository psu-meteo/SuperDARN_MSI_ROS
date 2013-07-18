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
QNX_ROS_STOP="ssh root@kod-imaging /root/current_ros/stop.ros"
QNX_ROS_START="ssh root@kod-imaging /root/current_ros/start.ros"
DDS_ROS_STOP="ssh root@kod-dds /root/current_ros/stop.ros"
DDS_ROS_START="ssh root@kod-dds /root/current_ros/start.ros"
QNX_ROS_REBOOT="ssh root@kod-imaging shutdown -f"
DDS_ROS_REBOOT="ssh root@kod-dds shutdown -f"
LNX_ROS_STOP="stop.radar &"
LNX_ROS_START="start.radar &"
LNX_ROS_REBOOT=""
QNX_TEST_CMD="ssh root@kod-imaging  date -u"
DDS_TEST_CMD="ssh root@kod-dds  date -u"

EMAIL_RADAR="KOD"
EMAIL_FROM="KOD_radar_ops@superdarn.gi.alaska.edu"
EMAIL_TO="jdspaleta@alaska.edu"
EMAIL_CC="wabristow@alaska.edu"
EMAIL_ALERT_FILE="$HOME/radops_email_alart.txt"
LAST_ALERT_FILE="$HOME/radops_last_alart.txt"

RADAR_RUNNING_FILE="$HOME/radops_active.txt"

####################################
# Script operation starts here
#################################### 
source ${HOME}/.bash_profile
currtime=`date +%s`
timestamp=`date -u`
echo $timestamp

if [ -e $EMAIL_ALERT_FILE ]
then
  emtime=`stat -c %Y ${EMAIL_ALERT_FILE}`
else
  touch -t 197001010000 -m ${EMAIL_ALERT_FILE}
fi
cdir=`pwd`
cd ${DIR}
errcond=$?
recverr=$?
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
errcond=0;
echo "Running QNX test cmd : ${QNX_TEST_CMD}"
${QNX_TEST_CMD} > /dev/null 2>&1
econd=$?
if [ $econd -ne 0 ]
then
  echo "QNX test cmd error: ${QNX_TEST_CMD}"
  errcond=$econd
else
  recvcurrtime=`ssh root@kod-imaging  cat /tmp/ros_restart_time`
  currlength=${#recvcurrtime}
  recverr=0
  recv_loss_counts=`ssh root@kod-imaging  grep -c -h -i loss /tmp/gc314*.${recvcurrtime}.log`
  for loss_count in $recv_loss_counts
  do
    count=${loss_count:18+$currlength}
    echo ${loss_count} ${count}
    if [ ${count} -ne 0 ]
    then
      echo "GCXXX_0 Recv Clock Loss Detected"
      recverr=-1
    fi
  done
  recv_loss_counts=`ssh root@kod-imaging  grep -c -h -i failed /tmp/gc314*.${recvcurrtime}.log`
  for loss_count in $recv_loss_counts
  do
    count=${loss_count:18+$currlength}
    echo ${loss_count} ${count}
    if [ ${count} -ne 0 ]
    then
      echo "GCXXX_0 Recv Clock Failed Config Detected"
      recverr=-2
    fi
  done
fi

echo "Running DDS test cmd : ${DDS_TEST_CMD}"
${DDS_TEST_CMD} > /dev/null 2>&1
econd=$?
if [ $econd -ne 0 ]
then
  echo "DDS test cmd error: ${DDS_TEST_CMD}"
  errcond=$econd
fi

if [ $errcond -ne 0 ]
then
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
$timestamp
END_MAIL
     touch -m ${EMAIL_ALERT_FILE}
     echo "Alert: Radar Unable to ssh into ROS machine" > ${LAST_ALERT_FILE}
     echo "$timestamp" >> ${LAST_ALERT_FILE}
    else
      echo "  ROS Access Alert:" 
      echo "    Last Email Alert Elapsed Secs:" $ediff
      echo "    Alert Sent in the last 8 hours"
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
if [ $recverr -ne 0 ]
then
	  echo "Recv Driver Clock or Config issue"
          emtime=`stat -c %Y ${EMAIL_ALERT_FILE}`
          errcond=$?
          if [ $errcond -eq 0 ]
          then
            ediff=$(( (currtime - emtime) ))
            if [ $ediff -gt 300 ]
            then
	      echo "  Send email"
              EMAIL_SUBJECT="${EMAIL_RADAR} : RADAR Reboot Alert"
              mail -s "${EMAIL_SUBJECT}" -c "${EMAIL_CC}" "${EMAIL_TO}" << END_MAIL
Alert: Radar Auto-reboot of Qnx recv machine initiated  
Recv clock loss condition: $recverr
$timestamp
END_MAIL
              touch -m ${EMAIL_ALERT_FILE}
              echo "Alert: Radar Auto-reboot of Qnx recv machine initiated" > ${LAST_ALERT_FILE}  
              echo "Recv clock loss condition: $recverr" >> ${LAST_ALERT_FILE} 
              echo "$timestamp" >> ${LAST_ALERT_FILE}
 
	      echo "  Initiating reboot of qnx machine: "
              echo ${LNX_ROS_STOP}
              ${LNX_ROS_STOP}
              sleep 10
              echo ${QNX_ROS_STOP}
              ${QNX_ROS_STOP}
              echo ${DDS_ROS_REBOOT}
              ${DDS_ROS_REBOOT}
              echo ${QNX_ROS_REBOOT}
              ${QNX_ROS_REBOOT}
              sleep 60
              echo ${LNX_ROS_START}
              nohup ${LNX_ROS_START} > /dev/null
            else
              echo "  Last Email Alert Elapsed Secs:" $ediff
	      echo "  Alert sent within the last 5 minutes"
            fi
            exit
          fi
fi

if [ $diff -gt 1800 ]
then
	  echo "Last fitacf write over 30 minutes ago"
          emtime=`stat -c %Y ${EMAIL_ALERT_FILE}`
          errcond=$?
          if [ $errcond -eq 0 ]
          then
            ediff=$(( (currtime - emtime) ))
            if [ $ediff -gt 900 ]
            then
	      echo "  Send email"
              EMAIL_SUBJECT="${EMAIL_RADAR} : RADAR Restart Alert"
              mail -s "${EMAIL_SUBJECT}" -c "${EMAIL_CC}" "${EMAIL_TO}" << END_MAIL
Alert: Radar Auto-restart initiated  
Recv clock loss condition: $recverr
$timestamp
END_MAIL
              touch -m ${EMAIL_ALERT_FILE}
              echo "Alert: Radar Auto-restart initiated" > ${LAST_ALERT_FILE}
              echo "Recv clock loss condition: $recverr" >> ${LAST_ALERT_FILE} 
              echo "$timestamp" >> ${LAST_ALERT_FILE}

	      echo "  Initiating stop of radar processes: "
              echo ${LNX_ROS_STOP}
              ${LNX_ROS_STOP}
              sleep 10
              echo ${QNX_ROS_STOP}
              ${QNX_ROS_STOP}
              echo ${DDS_ROS_STOP}
              ${DDS_ROS_STOP}
	      echo "  Initiating restart of qnx processes: "
              echo ${DDS_ROS_START}
              ${DDS_ROS_START}
	      sleep 10 
              echo ${QNX_ROS_START}
              ${QNX_ROS_START}
	      sleep 10 
	      echo "  Initiating restart of lnx processes  "
              echo ${LNX_ROS_START}
              nohup ${LNX_ROS_START} > /dev/null
            else
              echo "  Last Email Alert Elapsed Secs:" $ediff
	      echo "  Alert sent within the last 15 minutes"
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
            if [ $ediff -gt 1800 ]
            then
	      echo "  Send email"
              EMAIL_SUBJECT="${EMAIL_RADAR} : RADAR Crash Alert"
              mail -s "${EMAIL_SUBJECT}" -c "${EMAIL_CC}" "${EMAIL_TO}" << END_MAIL
Alert:  Radar Operations appears to be hung for 10 minutes
Recv clock loss condition: $recverr
$timestamp
END_MAIL
              touch -m ${EMAIL_ALERT_FILE}
              echo "Alert:  Radar Operations appears to be hung for 10 minutes" > ${LAST_ALERT_FILE}
              echo "Recv clock loss condition: $recverr" >> ${LAST_ALERT_FILE}
              echo "$timestamp" >> ${LAST_ALERT_FILE}
            else 
              echo "  Last Email Alert Elapsed Secs:" $ediff
	      echo "  Alert sent within the last 30 minutes"
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
          echo "Recv clock loss condition: $recverr"
          exit
fi

echo "Operations nominal"
echo "Recv clock loss condition: $recverr"

