#!/usr/bin/python
import xlwt
import re
import string
import sys

#example ./wtxls.py filename
wbk=xlwt.Workbook()
sheet = wbk.add_sheet('sheet 1')
style = xlwt.XFStyle()

file_object = open(sys.argv[1], 'rb')
job_id = 1
step = 1

time = 0

sheet.write(job_id-1, 1, "dedup time")
sheet.write(job_id-1, 2, "read time")
sheet.write(job_id-1, 3, "chunk time")
sheet.write(job_id-1, 4, "hash time")
sheet.write(job_id-1, 5, "total time")


for line in file_object:
    if step == 1:
        if re.match("dedup time.+", line):
            match = re.search("\d+.\d+", line);
            if match:
                result = match.group();
                time = float(result)
                sheet.write(job_id, 1, time) 
            step = 2
    if step == 2:
        if re.match("read time.+", line):
            match = re.search("\d+.\d+", line);
            if match:
                result = match.group();
                time = float(result)
                sheet.write(job_id, 2,time) 
            step = 3
    if step == 3:
        if re.match("chunk time.+", line):
            match = re.search("\d+.\d+", line);
            if match:
                result = match.group();
                time = float(result)
                sheet.write(job_id, 3,time) 
            step = 4
    if step == 4:
        if re.match("hash time.+", line):
            match = re.search("\d+.\d+", line);
            if match:
                result = match.group();
                time = float(result)
                sheet.write(job_id, 4,time) 
            step = 5
    if step == 5:
        if re.match("total time.+", line):
            match = re.search("\d+.\d+", line);
            if match:
                result = match.group();
                time = float(result)
                sheet.write(job_id, 5,time) 
            step = 1
            job_id = job_id +1
wbk.save(sys.argv[1] +'_time'+'.xls')



