import urllib
from HTMLParser import HTMLParser
import string
import datetime
import os
command={}
default_priority=6
max_duration=360
radars=["kod.a","kod.b","kod.c","kod.d"]
command["kod.d"]="rbspscan -stid kod -ep 44000 -sp 44001 -bp 44100 -c 4 -meribm 3 -westbm 2 -eastbm 5 -trig"
command["kod.c"]="normalsound -stid kod -ep 44000 -sp 44001 -bp 44100 -df 10750 -nf 10750 -fast -rank 1 -c 3"
command["kod.b"]="noopscan -stid kod -c 2"
command["kod.a"]="noopscan -stid kod -c 1"
scddir="/data/ros/scd/"
basescdfile="rbsp.scd"
ref_hours=1
Dst_onset_threshold= -50
Dst_end_threshold=-30
#Dst_onset_threshold= -10 
#Dst_end_threshold=-7
Dst_active=False

class MyHTMLParser(HTMLParser):
    active=False 
    table=None
    def handle_comment(self,data):
        if data.strip()==' ^^^^^ E yyyymm_part2.html ^^^^^ '.strip():
          self.active=True
        else:
	  if self.active==True:
	    pass 
          self.active=False 
    def handle_data(self, data):
        if self.active:
          self.table=data

current_time=datetime.datetime.utcnow()
entry_endtime=current_time-datetime.timedelta(days=1)
reference_time=current_time-datetime.timedelta(seconds=60*60*ref_hours)
timestr="%04d%02d" % (reference_time.year,reference_time.month)

print "Dst onset threshold: %lf" % (Dst_onset_threshold)
print "Dst end threshold: %lf" % (Dst_end_threshold)
# Now lets grab the Dst info
opener = urllib.FancyURLopener({})
print "Using URL: ","http://wdc.kugi.kyoto-u.ac.jp/dst_realtime/%s/index.html" % (timestr)
f = opener.open("http://wdc.kugi.kyoto-u.ac.jp/dst_realtime/%s/index.html" % (timestr))
text=f.read()
parser = MyHTMLParser()
parser.feed(text)
lines=parser.table.split("\n")
for line in lines[7:]:
    try:
      dy=int(line[0:2])
    except ValueError:
      dy=0
    if dy==reference_time.day:
        hour=reference_time.hour+1
        offset=-1;
        if hour>8:
          offset=offset+1
        if hour>16:
          offset=offset+1 
 
        ref_Dst=line[offset+hour*4:4+offset+hour*4]    
        ref_Dst=int(ref_Dst)
#      print "Reference Dst Hour: %d Val: %d" % (hour,ref_Dst) 
        print "Day: %d Hour: %d  Dst value: %lf" % (dy,hour,ref_Dst)

# Read existing RSBP schedule file and find ongoing event if it exists
for radar in radars:
  scdfilename="%s-%s" % (radar,basescdfile)
  scdfile=os.path.join(scddir,scdfilename)
  print scdfile
  if os.path.exists(scdfile):
    f = open(scdfile, 'r+')
    lines=f.readlines()
    f.close()
    next_line=False
    fallback_duration=max_duration
    for line in lines:
      if next_line:
        next_line=False
        segments=line.split()
        yr=int(segments[0].strip()) 
        mo=int(segments[1].strip()) 
        dy=int(segments[2].strip()) 
        hr=int(segments[3].strip()) 
        mi=int(segments[4].strip()) 
        try:
          dur=int(segments[5].strip()) 
        except ValueError:
          dur=fallback_duration
        prio=int(segments[6].strip()) 
        entry_endtime=datetime.datetime(yr,mo,dy,hr,mi)+datetime.timedelta(seconds=60*dur)
      if "::ACTIVE::" in line:
        val=line.split("::ACTIVE::")[1].strip()
        Dst_active={"true": True, "false": False}.get(val.lower())
      if "duration" in line:
        fallback_duration=int(line.split("duration")[1].strip())
      if "::CURRENT_EVENT:" in line:
        next_line=True


# Logic for new or ongoing event handling
  if Dst_active:
    if ref_Dst < Dst_end_threshold:
      print "Reset Ongoing Event Duration"
      write_entry=True
    else:
      print "Tailing Event trigger"
      write_entry=False
      if(entry_endtime < current_time):
        print "Event trigger has expired"
        Dst_active=False
        write_entry=False
  else:
    if ref_Dst < Dst_onset_threshold:
      print "New Event trigger"
      Dst_active=True
      write_entry=True
    else:
      print "No event :: Current Dst: %lf" % (ref_Dst)
      Dst_active=False
      write_entry=False

# Write new scdfile
  print "Updating %s" %(scdfile)
 
  lines=[]
  lines.append("# RSBP Triggered Event Schedule")
  lines.append("#   This is an automated generated schedule,")
  lines.append("#   please do not edit by hand")
  lines.append("#")
  lines.append("# ::ACTIVE:: %s"% (Dst_active))
  lines.append("# ::UPDATED:: %s"% (current_time))
  lines.append("#")
  lines.append("")
  lines.append("priority %d" % (default_priority))
  lines.append("duration %d" % (max_duration))
  lines.append("")
  lines.append("# ::CURRENT_EVENT: ")
  if write_entry:
    datestr="%04d %02d %02d %02d %02d" % (current_time.year,current_time.month,current_time.day,current_time.minute,current_time.minute)
    prestr="%s %02d %02d" % (datestr,max_duration,default_priority)
    lines.append("%s %s" % (prestr,command[radar]))

  f = open(scdfile, 'w+')
  for line in lines:
    f.write(line+"\n")
  f.close()
