#!/bin/sh
ORIG_FNAME=$1  
RES_FNAME=$2

#ffmpeg -i $ORIG_FNAME -acodec aac -ac 2 -ar 48000 -ab 96k -strict -2 -vcodec copy -bsf h264_mp4toannexb -f mpegts  $RES_FNAME > /dev/null
#&& rm -f $ORIG_FNAME
#echo "12121212121" >> 1.txt

echo $1 $2 >> 1.txt

