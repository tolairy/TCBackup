#ifndef MSQL_FIND_H_
#define MSQL_FIND_H_


bool db_get_job_files(DedupDb * db,int64_t job_id, Queue *filelist);
bool db_get_job_list(DedupDb *db, Queue *filelist); 

#endif
