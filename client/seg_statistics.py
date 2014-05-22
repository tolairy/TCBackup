#!/usr/bin/python
import os
import sys
import linecache
import re

#The version number of statistics
version_number = 10

if len(sys.argv) > 2:
    print 'input error'
else:
    if len(sys.argv) == 2:
        version_number = int(sys.argv[1])

#save the segments
segments = []

#Statistics the lines of file
count = -1
for count, line in enumerate(open("./tmp/static_output.txt", 'rU')):
    pass
count += 1

if version_number > count:
    version_number = count

dict = {}
#Reading the specified rows from the last row
i = 1
while i <= version_number:
    line = linecache.getline('./tmp/static_output.txt', count - i + 1)
    line = line[:-1]
    list = line.split('/')
    for item in list:
    	info = item.split(',')
    	if item[0] not in dict:
    		dict[item[0]] = item[1]
    
    		
      #  if segment not in segments:
       #     segments.append(segment)
    i += 1
    
print ('The last %d version used %d segments' % (version_number, len(segments)))
print segments
