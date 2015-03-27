#!/usr/bin/python
import os
import sys
import linecache
import re

#useage
#./seg_static path version_num

#The version number of statistics
version_number = 10
path = ""

if len(sys.argv) != 3:
    print 'input error'
else:
    if len(sys.argv) == 3:
        version_number = int(sys.argv[2])
        path = str(sys.argv[1])

#save the segments
segments = []

#Statistics the lines of file
count = -1
for count, line in enumerate(open(path, 'rU')):
    #pass
    count += 1

#print count

if version_number > count:
    version_number = count

#Reading the specified rows from the last row

cur_count = version_number
while cur_count <= count:

    i = 1
    dict = {}
    length = 0
    while i <= version_number:
        line = linecache.getline(path, cur_count - i + 1)
        line = line[:-1]
        list = line.split('/')
    # print list
        for item in list:
    	    info = item.split(',')
            #print info
            if info[0] not in dict:
                dict[info[0]] = info[1]
                length += int(info[1])
        i += 1
    
 #   print ('The last %d version used %d segments' % (version_number, len(dict)))
    length = length / (1024*1024)
    print (length)

       # print dict

    cur_count += 1
