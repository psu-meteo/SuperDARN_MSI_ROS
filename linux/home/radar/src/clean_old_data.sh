#!/bin/sh
# This checks that the specified file is less than DAYS old.
# returns 0 if younger than DAYS.
# returns 1 if older than DAYS.
DAYS=120
DIR=/home/radar2/data/rawdata/
if [ -z "$DIR" ]
then
  echo "No Directory requested"
  exit 1
fi

# bash check if directory exists
if [ -d $DIR ]; then
	echo "Directory exists" > /dev/null
else 
	echo "Directory does not exists"
        exit 1
fi 

FILES=`ls $DIR`
SECONDS=$(bc <<< "${DAYS}*24*60*60") # seconds in a day
#echo $SECONDS

for FILE in $FILES
do
  #file age in seconds = current_time - file_modification_time.
  FILEAGE=$(($(date +%s) - $(stat -c '%Y' "$DIR/$FILE")))
  test $FILEAGE -gt $SECONDS && {
    echo "$FILE is older than $DAYS days."
    rm $DIR/$FILE
  }
done
#    exit 0
exit 1
