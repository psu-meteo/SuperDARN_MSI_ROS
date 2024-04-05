import argparse, ftplib, logging, os, smtplib, socket, string, sys
from ConfigParser import ConfigParser
from datetime import datetime, timedelta
from email.mime.text import MIMEText
from HTMLParser import HTMLParser
from StringIO import StringIO
from urllib2 import URLError, urlopen


def setup_logging(log_file, log_level):
    if not log_file:
        return
    logging.basicConfig(filename=log_file,
                        filemode='a+',
                        level=log_level,
                        format="%(asctime)s\t%(levelname)s\t%(module)s\t%(message)s",
                        datefmt="%Y-%m-%dT%H:%M:%S")


def parse_config(config_file):
    '''
    Returns all config, and a dictionary of radar configs.
    
    Each radar config is itself a dictionary containing the following:
        ID:             radar ID (eg. HAL)
        sched_name      name of special schedule file (eg. hal-special.scd)
        sched_dir       directory containing special schedule file (eg. /data/scd)
        ftp_server      name of radar ftp server
        ftp_username    ftp server username
        ftp_password    ftp server password
    '''
    conf = ConfigParser()
    conf.optionxform = str
    conf.read(config_file)
    radars = dict()
    for radar_section in [section for section in conf.sections() if section.startswith('radar_')]:
        radar_conf = dict()
        for item, value in conf.items(radar_section):
            if value=="": value=None
            radar_conf[item] = value
        radars[radar_conf['ID']] = radar_conf
    return conf, radars
    
    
def get_url_content(conf):
    logging.info('Checking current Dst value')
    
    dst_url = conf.get('main', 'dst_url')
    dst_url_timeout = conf.getint('main', 'dst_url_timeout')
    
    logging.debug('Opening URL: ' + dst_url)
    f = urlopen(dst_url, None, dst_url_timeout)
    
    logging.debug('URL opened; Reading content')
    content = f.readlines()
    return content
    
    
def parse_url_content(content):
    table_start = content.index('<!-- ^^^^^ E yyyymm_part2.html ^^^^^ -->\n') + 7
    table_end = content.index('<!-- vvvvv S yyyymm_part3.html vvvvv -->\n')
    
    # Subtracting 30 minutes from the current time combines with the hour offset formula to ensure
    # the latest value is always taken from the table on the Dst website
    #
    # For example, the 16:30 Dst value is written under hour 17 in the table, and 17:00 value
    # overwrites it. Both 16:30 and 17:00 must therefore reference the same point in the table
    time = datetime.utcnow() - timedelta(minutes=30)
    day = time.day
    current_day_data = [line for line in content[table_start:table_end] if line.startswith(str(day).rjust(2))][0]
    
    # calculate position of entry for given hour, taking account of 4 characters used for each hour,
    # spaces after hours 8 and 16, and 3 characters used for day number at start of line
    hour = time.hour
    hour_offset = (hour*4) + divmod(hour, 8)[0] + 3
    current_dst = current_day_data[hour_offset:hour_offset + 4]
    logging.info('Current Dst value: ' + current_dst)
    
    return int(current_dst)
    
    
def update_schedule(conf, radar, current_dst):
    NEW_EVENT, END_EVENT, RESET_EVENT, NO_EVENT = [0, 1, 2, 3]
    onset_threshold = conf.getint('main', 'dst_onset_threshold')
    end_threshold = conf.getint('main', 'dst_end_threshold')
    
    logging.info('Updating schedule for radar: ' + radar['ID'])
    
    active = read_current_schedule(radar)
    if active == 'True':
        if current_dst <= end_threshold:
            event = RESET_EVENT
            logging.info('Previous state: Active; Restarting current event')
        else:
            event = END_EVENT
            logging.info('Previous state: Active; Ending current event')
    else:
        if current_dst <= onset_threshold:
            event = NEW_EVENT
            logging.info('Previous state: Not active; Starting new event')
        else:
            event = NO_EVENT
            logging.info('Previous state: Not active; No event')
    
    rcp_str = write_schedule(radar, event)
    send_email(conf, radar, event, current_dst, rcp_str)
    
    
def read_current_schedule(radar):
    server = radar['ftp_server']
    user = radar['ftp_username']
    password = radar['ftp_password']
    sched_file = os.path.join(radar['sched_dir'], radar['sched_name'])
    
    logging.debug('Reading current schedule from: %s:%s' % (server, sched_file))
    
    active = 'False'
    temp_sched = []
    if server is not None:
      ftp = ftplib.FTP(server, user, password)
      try:
        ftp.retrlines('RETR ' + sched_file, temp_sched.append)
        ftp.quit()
      except:
        # if file does not yet exist, assume not active and continue
        ftp.quit()
    else:
        print "Reading local file location: %s" % sched_file
        # read in the current schedule file location locally
        if os.path.isfile(sched_file):
          with open(sched_file) as f:
            temp_sched = f.readlines()
        else:
          print "File: %s, does not exist" % sched_file
         

    for line in temp_sched:
        if line.startswith('# ::ACTIVE::'):
            active = line.split()[-1]
            logging.debug('Active: ' + active)
    return active

    
def write_schedule(radar, event):
    NEW_EVENT, END_EVENT, RESET_EVENT, NO_EVENT = [0, 1, 2, 3]
    
    server = radar['ftp_server']
    user = radar['ftp_username']
    password = radar['ftp_password']
    sched_file = os.path.join(radar['sched_dir'], radar['sched_name'])
    priority = radar['default_priority']
    duration = radar['max_duration']
    rcp_command = radar['rcp_command']
    
    logging.debug('Writing new schedule to: %s:%s' % (server, sched_file))
        
    t = datetime.now()
    timestamp = t.strftime('%Y-%m-%d %H:%M:%S')
    sched_timestamp = t.strftime('%Y %m %d %H %M')
    
    if event == NEW_EVENT or event == RESET_EVENT:
        active = True
        rcp_str = sched_timestamp + ' ' + duration + ' ' + priority + ' ' + rcp_command
        logging.debug('Writing RCP string: ' + rcp_str)
    else:
        active = False
        rcp_str = '# None'
        logging.debug('Writing RCP string: ' + rcp_str)
        
    schedule = '''
# RSBP Triggered Event Schedule
#
# This is an automatatically generated schedule produced by rbsp.py,
# please do not edit by hand
#
# ::ACTIVE:: %s
# ::UPDATED:: %s

priority %s
duration %s

# ::CURRENT_EVENT::
%s
''' % (active, timestamp, priority, duration, rcp_str)
    if (server is not None) : 
      # ftp STOR requires a file like object;
      schedStrIO = StringIO(schedule)
      ftp = ftplib.FTP(server, user, password)
      ftp.storlines('STOR ' + sched_file, schedStrIO)
      ftp.quit()
    else:
      print "Writing to local file path"
      if os.path.isdir(radar['sched_dir']):
        text_file = open(sched_file, "w")
        text_file.write(schedule)
        text_file.close()
      else:  
        print "Dir: %s, does not exist" % radar['sched_dir']
        print "new schedule file not written"
    return rcp_str
    
    
def send_email(conf, radar, event, current_dst, rcp_str):
    NEW_EVENT, END_EVENT, RESET_EVENT, NO_EVENT = [0, 1, 2, 3]
    
    onset_threshold = conf.getint('main', 'dst_onset_threshold')
    end_threshold = conf.getint('main', 'dst_end_threshold')
    sched_file = os.path.join(radar['sched_dir'], radar['sched_name'])
    sender = conf.get('email', 'sender')
    recipients = conf.get('email', 'recipients').split(',')
    smtp_server = conf.get('email', 'smtp_server')
    
    if not conf.getboolean('email', 'enable_emails'):
        return
    if event == RESET_EVENT or event == NO_EVENT:
        return
    
    logging.info('Sending notification email to: ' + ','.join(recipients))
    if event == NEW_EVENT:
        summary = 'SuperDARN CT-TRIG: New Event'
        notification = 'CT-TRIG mode activated for radar: %s\n\nRCP entry "%s" \nwritten to "%s"' % (radar['ID'], rcp_str, sched_file)
    elif event == END_EVENT:
        summary = 'SuperDARN CT-TRIG: Tailing Event'
        notification = 'CT-TRIG mode ended for radar: %s' % (radar['ID'])
    
    currentDstStr = 'Current Dst: %d (Onset Threshold: %d, End Threshold: %d)' % (current_dst, onset_threshold, end_threshold)
    
    msg = MIMEText(notification + '\n\n' + currentDstStr)
    msg['Subject'] = summary
    msg['From'] = sender
    msg['To'] = ', '.join(recipients)
    s = smtplib.SMTP(smtp_server)
    s.sendmail(sender, recipients, msg.as_string())
    s.quit()
    
    
class RBSPError(Exception):
    def __init__(self, value):
        self.value = value
        logging.error(value)
    def __str__(self):
        return repr(self.value)
        
        
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('config_file', help='Path to config file')
    parser.add_argument('--log_file', help='Path to log file (optional)', default=None)
    parser.add_argument('--log_level', help='Specify logging level', default='INFO',
                         choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"])
    args = parser.parse_args()
    setup_logging(args.log_file, args.log_level)
        
    conf, radars = parse_config(args.config_file)
    try:
        content = get_url_content(conf)
        current_dst = parse_url_content(content)
    except (URLError, socket.timeout):
        logging.error('Connection to Dst URL timed out; ending Dst check\n')
        sys.exit(1)
    except:
        logging.exception('Failed to get Dst value; ending Dst check\n')
        sys.exit(1)
        
    for radar in radars.itervalues():
        try:
            update_schedule(conf, radar, current_dst)
        except:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            logging.exception('Failed to update schedule file for radar: ' + radar['ID'])
            
    logging.info('Dst check complete\n') 
