#include "../global.h"




bool db_delete_job(DedupDb *db, int64_t job_id)
{
	DedupDb *mdb;
	mdb = db;
	
	printf("%s,%d, enter delete job, job id is %d\n", __FILE__, __LINE__, job_id);
	memset(mdb->cmd, 0, 1024);
	sprintf(mdb->cmd, "delete from File where VersionCount=1 and FileId in"
		" (select FileId from VersionFile where JobId=%d)", job_id);
	 //  printf("%s,%d:%s\n", __FILE__,__LINE__,mdb->cmd);
	if(mysql_query(&mdb->mysql, mdb->cmd)){
		err_msg1("mysql query error!");
		//fprintf(stderr, "query failed: (%d) %s \n", mysql_errno(&mdb->mysql), mysql_error(&mdb->mysql));
		return false;
	}
	

	memset(mdb->cmd, 0, 1024);
	sprintf(mdb->cmd, "update File set VersionCount=VersionCount-1 where VersionCount>1 and "
		"FileId in (select FileId from VersionFile where JobId=%d)", job_id);
	if(mysql_query(&mdb->mysql, mdb->cmd)){
		err_msg1("mysql query error!");
		return false;
	}
	
	
	memset(mdb->cmd, 0, 1024);
	sprintf(mdb->cmd, "delete from VersionFile where JobId=%d", job_id);
	if(mysql_query(&mdb->mysql, mdb->cmd)){
		err_msg1("mysql query error!");
		return false;
	}
	
	
	memset(mdb->cmd, 0, 1024);
	sprintf(mdb->cmd, "delete from Job where JobId=%d", job_id);
	if(mysql_query(&mdb->mysql, mdb->cmd)){
		err_msg1("mysql query error!");
		return false;
	}
	
	memset(mdb->cmd, 0, 1024);
	sprintf(mdb->cmd, "delete from Object where ObjectId not in "
		"(select ObjectId from Job)");
	if(mysql_query(&mdb->mysql, mdb->cmd)){
		err_msg1("mysql query error!");
		return false;
	}

	return true;


}
