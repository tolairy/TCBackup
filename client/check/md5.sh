#!/bin/bash

#compute the md5 of a file
#example: ./md5.sh v2 v2.md5

function walkdir()
{
    for file in `ls $1`
        do
            if [ -d $1"/"$file ]
                then
                    walkdir $1"/"$file $2
            else
                path=$1"/"$file
                    echo $path
                    md5sum $path >>$2
                    fi
                    done
}
old=`pwd`
new=$(echo ${1##*/})
#echo $old
#echo $new
if [ -f $2 ]
then
  rm -rf $2
  touch $2
else
  touch $2
fi
cd $1
cd ..
walkdir $new $old"/"$2
cd $old
