#!/bin/bash
#example ./md5c.sh /root/a/v2 a.md5
old=`pwd`
cd $1
cd ..

md5sum -c $old"/"$2 | grep -v 'È·¶¨'
if [ "$?" = "1" ]
then
echo 'very right!'
fi
cd $old
