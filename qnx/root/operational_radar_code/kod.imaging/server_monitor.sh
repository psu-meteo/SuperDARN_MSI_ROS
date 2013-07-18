#!/bin/sh
LOGFILE="/tmp/ros_monitor.log"
ACTIVEFILE="/tmp/ros_active.log"
NAME="server_monitor"
STOP_CMD="/root/current_ros/stop.ros"
#START_CMD="/root/current_ros/start.ros"
START_CMD="/root/current_ros/start.ros"
SSH_DDS_CMD="ssh root@192.168.10.2"
ICS_REQUIRED=4
GCXXX_REQUIRED=7

CURRENT_TIME=`date -t`
CURRENT_TIMESTR=`date -u`
TIME_0=`expr $CURRENT_TIME - 120`
RUNNING="/tmp/ros_running"
ACTIVE_SERVER_FILE="/tmp/server_activity_time"
ACTIVE_RECV_FILE="/tmp/recv_activity_time"
LAST_SERVER_CMD_FILE="/tmp/server_cmd_time"
LAST_RECV_CMD_FILE="/tmp/recv_cmd_time"
LAST_RESTART=0
SERVER_COUNT=`ps -A|grep -c arby_server`
RECV_COUNT=`ps -A|grep -c receiver_tcp_driver`
GCXXX_COUNT=`ps -A|grep -c _gc314FS`
LOSS0=`grep -c -h -i loss /tmp/gc314FS-0.log`
LOSS1=`grep -c -h -i loss /tmp/gc314FS-1.log`
LOSS2=`grep -c -h -i loss /tmp/gc314FS-2.log`
LOSS3=`grep -c -h -i loss /tmp/gc314FS-3.log`
LOSS4=`grep -c -h -i loss /tmp/gc314FS-4.log`
LOSS5=`grep -c -h -i loss /tmp/gc314FS-5.log`
LOSS6=`grep -c -h -i loss /tmp/gc314FS-6.log`
DDS_COUNT=`$SSH_DDS_CMD "ps -A|grep -c dds_tcp_driver"`
TIMING_COUNT=`$SSH_DDS_CMD "ps -A|grep -c timing_tcp_driver"`
DIO_COUNT=`$SSH_DDS_CMD "ps -A|grep -c dio_tcp_driver"`
GPS_COUNT=`$SSH_DDS_CMD "ps -A|grep -c gps_tcp_driver"`
ICS_COUNT=`$SSH_DDS_CMD "ps -A|grep -c _ics660-drv"`

RESTART=0
REBOOT=0
echo "server: $SERVER_COUNT"
echo "recv: $RECV_COUNT"
echo "gcxxx: $GCXXX_COUNT"
echo "dds: $DDS_COUNT"
echo "ics: $ICS_COUNT"
echo "timing: $TIMING_COUNT"
echo "dio: $DIO_COUNT"
echo "gps: $GPS_COUNT"

if [ -f $ACTIVEFILE ]
then
   ACTIVE_PID=`cat $ACTIVEFILE`
   ps -p $ACTIVE_PID | grep $NAME
   OUT=$?
   if [ $OUT -eq 0 ]; then
     echo "$CURRENT_TIMESTR : monitor already running"  
     echo "$CURRENT_TIMESTR : monitor already running" >> $LOGFILE 
     exit -1
   else
     echo "PID $ACTIVE_PID is not running $NAME"
   fi
fi
touch $ACTIVEFILE
echo "$$" > $ACTIVEFILE

if [ -f $RUNNING ]
then
   echo "$CURRENT_TIMESTR : Server Running" >> $LOGFILE 
#  if [ -f /tmp/ros_restart_time ]
#  then
#    echo "restart time file does exist"
#    LAST_RESTART=`cat /tmp/ros_restart_time`
#    RESTART=0
#  else
#    RESTART=1
#    echo "restart time file does not exist"
#  fi

  if [ $SERVER_COUNT -ne 1 ]
  then
    if [ $SERVER_COUNT -eq 0 ]
    then
      echo "$CURRENT_TIMESTR : No Server Process Detected" >> $LOGFILE
    else
      echo "$CURRENT_TIMESTR : Multiple Servers Detected" >> $LOGFILE
    fi
    RESTART=1
  fi

  if [ $RECV_COUNT -ne 1 ]
  then
    if [ $RECV_COUNT -eq 0 ]
    then
      echo "$CURRENT_TIMESTR : No RECV Process Detected"  >> $LOGFILE
    else
      echo "$CURRENT_TIMESTR : Multiple RECV Detected"    >> $LOGFILE
    fi
    RESTART=1
  fi
  
  if [ $DDS_COUNT -ne 1 ]
  then
    if [ $DDS_COUNT -eq 0 ]
    then
      echo "$CURRENT_TIMESTR : No DDS Process Detected"  >> $LOGFILE
    else
      echo "$CURRENT_TIMESTR : Multiple DDS Detected"    >> $LOGFILE
    fi
    RESTART=1
  fi

  if [ $DIO_COUNT -ne 1 ]
  then
    if [ $DIO_COUNT -eq 0 ]
    then
      echo "$CURRENT_TIMESTR : No DIO Process Detected"  >> $LOGFILE
    else
      echo "$CURRENT_TIMESTR : Multiple DIO Detected"    >> $LOGFILE
    fi
    RESTART=1
  fi

  if [ $GPS_COUNT -ne 1 ]
  then
    if [ $GPS_COUNT -eq 0 ]
    then
      echo "$CURRENT_TIMESTR : No GPS Process Detected"  >> $LOGFILE
    else
      echo "$CURRENT_TIMESTR : Multiple GPS Detected"    >> $LOGFILE
    fi
    RESTART=1
  fi

  if [ $TIMING_COUNT -ne 1 ]
  then
    if [ $TIMING_COUNT -eq 0 ]
    then
      echo "$CURRENT_TIMESTR : No TIMING Process Detected"  >> $LOGFILE
    else
      echo "$CURRENT_TIMESTR : Multiple TIMING Detected"    >> $LOGFILE
    fi
    RESTART=1
  fi

  if [ $GCXXX_COUNT -ne $GCXXX_REQUIRED ]
  then
    echo "$CURRENT_TIMESTR : Wrong GCxxx Process Count Detected"  >> $LOGFILE
    RESTART=1
  fi

  if [ ${LOSS0} -ne 0 ]
  then
    echo "$CURRENT_TIMESTR : GCXXX_0 Clock Loss Detected"  >> $LOGFILE
    REBOOT=1
  fi

  if [ ${LOSS1} -ne 0 ]
  then
    echo "$CURRENT_TIMESTR : GCXXX_1 Clock Loss Detected"  >> $LOGFILE
    REBOOT=1
  fi

  if [ ${LOSS2} -ne 0 ]
  then
    echo "$CURRENT_TIMESTR : GCXXX_2 Clock Loss Detected"  >> $LOGFILE
    REBOOT=1
  fi

  if [ ${LOSS3} -ne 0 ]
  then
    echo "$CURRENT_TIMESTR : GCXXX_3 Clock Loss Detected"  >> $LOGFILE
    REBOOT=1
  fi

  if [ ${LOSS4} -ne 0 ]
  then
    echo "$CURRENT_TIMESTR : GCXXX_4 Clock Loss Detected"  >> $LOGFILE
    REBOOT=1
  fi

  if [ ${LOSS5} -ne 0 ]
  then
    echo "$CURRENT_TIMESTR : GCXXX_5 Clock Loss Detected"  >> $LOGFILE
    REBOOT=1
  fi

  if [ ${LOSS6} -ne 0 ]
  then
    echo "$CURRENT_TIMESTR : GCXXX_6 Clock Loss Detected"  >> $LOGFILE
    REBOOT=1
  fi

  if [ $ICS_COUNT -ne $ICS_REQUIRED ]
  then
    echo "$CURRENT_TIMESTR : Wrong ICS Process Count Detected"  >> $LOGFILE
    RESTART=1
  fi

  if [ -f $ACTIVE_SERVER_FILE ]
  then
    ACTIVE_SERVER_TIME=`cat $ACTIVE_SERVER_FILE`
    echo "$ACTIVE_SERVER_TIME $TIME_0"
    if [ $ACTIVE_SERVER_TIME -lt $TIME_0 ]
    then
      echo "$CURRENT_TIMESTR : Server appears to be hung"  >> $LOGFILE
      RESTART=1
    fi
  fi

  if [ -f $ACTIVE_RECV_FILE ]
  then
    ACTIVE_RECV_TIME=`cat $ACTIVE_RECV_FILE`
    if [ $ACTIVE_RECV_TIME -lt $TIME_0 ]
    then
      echo "$CURRENT_TIMESTR : RECV appears to be hung"  >> $LOGFILE
      RESTART=1
    fi
  fi

  if [ -f $LAST_SERVER_CMD_FILE ]
  then
    LAST_SERVER_CMD=`cat $LAST_SERVER_CMD_FILE`
    if [ $LAST_SERVER_CMD -lt $TIME_0 ]
    then
      echo "$CURRENT_TIMESTR : Server appears to be hung no msg processed"  >> $LOGFILE
      RESTART=1
    fi
  fi

  if [ -f $LAST_RECV_CMD_FILE ]
  then
     LAST_RECV_CMD=`cat $LAST_RECV_CMD_FILE`
    if [ $LAST_RECV_CMD -lt $TIME_0 ]
    then
      echo "$CURRENT_TIMESTR : RECV appears to be hung no msg processed"  >> $LOGFILE
      RESTART=1
    fi
  fi  

  if [ $REBOOT -eq 1 ]
  then
    echo "$CURRENT_TIMESTR : Rebooting ROS computer"  >> $LOGFILE
    echo "$CURRENT_TIMESTR : Rebooting ROS computer"  

    $STOP_CMD
    $SSH_DDS_CMD $STOP_CMD 
    sleep 10
    $SSH_DDS_CMD $START_CMD
    sleep 30
    echo "$CURRENT_TIMESTR : shutting down now"  >> $LOGFILE
    shutdown -f
  fi

  if [ $RESTART -eq 1 ]
  then
    echo "$CURRENT_TIMESTR : Restarting ROS"  >> $LOGFILE
#    mkdir  /tmp/jef_${TIME_0}
#    cp -f /tmp/*.log /tmp/jef_${TIME_0}/
#    cp -f /tmp/*lock /tmp/jef_${TIME_0}/
    $STOP_CMD
    $SSH_DDS_CMD $STOP_CMD 
    sleep 10
    $SSH_DDS_CMD $START_CMD
    sleep 30
    $START_CMD
    echo "$CURRENT_TIMESTR : Restart Complete"  >> $LOGFILE
  fi
fi
