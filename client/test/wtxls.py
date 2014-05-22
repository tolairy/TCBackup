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
total_length = 1
dedup_length = 0
dedup_ratio = 0
traditional_read = 0
speed_factor = 0
restore_length = 0
restorr_factor = 0
sheet.write(job_id-1, 1, "total length")
sheet.write(job_id-1, 2, "dedup length")
sheet.write(job_id-1, 3, "dedup ratio")
sheet.write(job_id-1, 4, "backup time")
sheet.write(job_id-1, 5, "traditional restore")
sheet.write(job_id-1, 6, "speed factor")
sheet.write(job_id-1, 7, "cloud restore")
sheet.write(job_id-1, 8, "restore factor")
for line in file_object:
    if step == 1:
        if re.match("total size.+", line):
            match = re.search("\d+", line);
            if match:
                result = match.group();
                total_length = string.atoi(result)
                sheet.write(job_id, 1, total_length)
            step =2
    elif step == 2:
        if re.match("dedup size.+", line):
            match = re.search("\d+", line);
            if match:
                result = match.group();
                dedup_length = string.atoi(result)
                sheet.write(job_id, 2, dedup_length) 
            if total_length != 0:
                dedup_ratio = dedup_length/float(total_length)
                sheet.write(job_id, 3, dedup_ratio)
            step = 3
    elif step == 3:
        if re.match("total time.+",line):
            match = re.search("\d+.\d+", line);
            if match:
                result = match.group();
                time = float(result)
#                print time
                sheet.write(job_id, 4, time)    
            step = 4
    elif step == 4:
        if re.match("in traditional.+",line):
            match = re.search("\d+", line);
            if match:
                result = match.group();
                traditional_read = string.atoi(result)
                sheet.write(job_id, 5, traditional_read)
            if traditional_read != 0:
                speed_factor = 1/(float(traditional_read)/(total_length/(1024*1024)))
                sheet.write(job_id, 6, speed_factor)
            step = 5
    elif step == 5:
        if re.match("download size.+", line):
            match = re.search("\d+", line);
            if match:
                result = match.group();
                restore_length = string.atoi(result)
                sheet.write(job_id, 7, restore_length)
            if restore_length != 0:
                restore_factor = total_length/float(restore_length)
                sheet.write(job_id, 8, restore_factor)
            step = 1
            job_id = job_id +1
wbk.save(sys.argv[1] + '.xls')

