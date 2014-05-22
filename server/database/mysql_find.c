#include "../global.h"

bool db_get_job_files(DedupDb * db, int64_t job_id, Queue * filelist)
{
	DedupDb *mdb = db;
	
	printf("%s,%d:%s\n", __FILE__,__LINE__,"enter get job files");
    memset(mdb->cmd, 0, 1024);
    sprintf(mdb->cmd, "SELECT HashCode,FilePath FROM File WHERE FileId in"
		"(select FileId from VersionFile where JobId=%d)", job_id);
    if(mysql_query(&mdb->mysql, mdb->cmd)){
		return false;
	}
	mdb->result = mysql_store_result(&mdb->mysql);
	if((mdb->numRows = mysql_num_rows(mdb->result)) == 0){
		mysql_free_result(mdb->result);
		return false;
	}

	int i;
	unsigned long path_length;
	for(i = 0; i < mdb->numRows; i++) {
		
		mdb->row = mysql_fetch_row(mdb->result);
		//lengths = mysql_fetch_lengths(mdb->result);
		
		path_length = strlen(mdb->row[1]);
		char *buf = (char *)malloc(FINGER_LENGTH+path_length+sizeof(int32_t));
		memset(buf, 0, FINGER_LENGTH+path_length+sizeof(int32_t));
		int32_t total_length = FINGER_LENGTH + path_length;
		
		memcpy(buf, &total_length, sizeof(int32_t));
		memcpy(buf+4, mdb->row[0], FINGER_LENGTH);
		memcpy(buf+4+FINGER_LENGTH, mdb->row[1], path_length);
		queue_push(filelist, buf);
		//printf("db get file info %s\n", buf+4+FINGER_LENGTH);
	//	free(lengths);

	}
	
	mysql_free_result(mdb->result);
    return true;

}

bool db_get_job_list(DedupDb * db,Queue * filelist)
{
	
	DedupDb *mdb = db;
	if (db ==NULL) {
		err_msg1("get job list, db is NULL!");
		return false;
	}
	int buf_len = sizeof(int32_t)+32+FILE_NAME_LEN;
	
	printf("%s,%d, into db get job list\n", __FILE__,__LINE__);

	memset(mdb->cmd, 0, 1024);
    sprintf(mdb->cmd, "SELECT JobId, FileSet FROM Job,Object where "
		"Job.ObjectId=Object.ObjectId");
    if(mysql_query(&mdb->mysql, mdb->cmd)){
		err_msg1("get job list query error!");
		printf( "mysql_query err: %s", mysql_error(&mdb->mysql)); 
		return false;
	}
	mdb->result = mysql_store_result(&mdb->mysql);
	if((mdb->numRows = mysql_num_rows(mdb->result)) == 0){
		mysql_free_result(mdb->result);
		return true;
	}
	
	int i;
	char *tmp = (char *)malloc(buf_len);
	for(i = 0; i < mdb->numRows; i++) {
		
		mdb->row = mysql_fetch_row(mdb->result);
		
		char *buf = (char *)malloc(buf_len);
		
		memset(tmp, 0, buf_len);
		memset(buf, 0, buf_len);
		
		sprintf(tmp, list_jobinfo, mdb->row[0], mdb->row[1]);
		int32_t len = strlen(tmp);
		memcpy(buf, &len, sizeof(len));
		memcpy(buf+sizeof(len), tmp, len);
		queue_push(filelist, buf);
	}
	
	free(tmp);

	mysql_free_result(mdb->result);
	
	return true;
}
