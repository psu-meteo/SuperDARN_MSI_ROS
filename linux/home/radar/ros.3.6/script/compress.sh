#!/bin/bash

dir=${1}
sfx=${2}

lastHour=(`date -u +%Y%m%d.%H --date='1 hours ago'`)
thisHour=(`date -u +%Y%m%d.%H`)

# If you do not want a particular day moved to the already_transferred directory on the remote server,
# place its date in the no_remove array.  Format: YYYYmmdd.HH
no_remove=( $lastHour $thisHour )

echo
echo "	Files with the following prefixes will NOT be bziped:"
echo "	${no_remove[@]}"


# Filter out the files listed that match thisHour and lastHour
flist=`ls /data/${dir}/*${sfx} | grep -v ${thisHour} | grep -v ${lastHour}`

echo "	Total number of files to be transferred: ${#flist[@]}"

for item in ${flist[@]}
	do
	echo ${item}
	bzip2 -v ${item}
	done

exit

