#include "../global.h"


#define ASSERT(a) if(!(a)){return false;}


/* find user with username */
bool find_username(DedupDb *mdb, char *username) 
{
    ASSERT(mdb != NULL);
	ASSERT(username != NULL);
    memset(mdb->cmd, 0, 1024);
    sprintf(mdb->cmd, "SELECT UserId FROM User WHERE UserName='%s'", username);

    if(mysql_query(&mdb->mysql, mdb->cmd)){
		fprintf(stderr, "query failed: (%d) %s \n", mysql_errno(&mdb->mysql), mysql_error(&mdb->mysql));
		return false;
	}
	mdb->result = mysql_store_result(&mdb->mysql);
	if((mdb->numRows = mysql_num_rows(mdb->result)) != 1){
		fprintf(stderr, "query failed: (%d) %s \n", mysql_errno(&mdb->mysql), mysql_error(&mdb->mysql));
		mysql_free_result(mdb->result);
		return false;
	}
    mdb->row = mysql_fetch_row(mdb->result);
	mdb->UsrId = atoi(mdb->row[0]);
	
	mysql_free_result(mdb->result);
    return true;
}


/* get password by username */
bool get_pass_by_username(DedupDb *mdb, char *username, char *password) 
{
    ASSERT(mdb != NULL);
	ASSERT(username != NULL);
    memset(mdb->cmd, 0, 1024);
    sprintf(mdb->cmd, "SELECT Password FROM User WHERE UserName='%s'", username);
    
    if (mysql_query(&mdb->mysql, mdb->cmd)) {
        mdb->result = mysql_store_result(&mdb->mysql);
        mdb->numRows = mysql_num_rows(mdb->result);
        if (mdb->numRows <= 0) {
            printf("User record '%s' does NOT exist!\n", username);
            mysql_free_result(mdb->result);
            return false;
        } else if (mdb->numRows == 1) {
            if ((mdb->row = mysql_fetch_row(mdb->result)) == NULL) {
                printf("error fetching row: (%d) %s \n", mysql_errno(&mdb->mysql), mysql_error(&mdb->mysql));
                mysql_free_result(mdb->result);
                return false;
            } else {
                if((mdb->row[0]) != NULL && (memcmp(password, mdb->row[0], strlen(password))==0) && (strlen(password) == strlen(mdb->row[0]))) {
                	mysql_free_result(mdb->result);
                    return true;
                }else {
                	err_msg1("password doesnot match!");
                }
                
            }
        }
        mysql_free_result(mdb->result);
    }
    return false;
}


/* insert a user into user table */
bool insert_record_into_user(DedupDb *mdb, UsrRecord *user)
{
    ASSERT(mdb != NULL);
    ASSERT(user != NULL);
    memset(mdb->cmd, 0, 1024);
    sprintf(mdb->cmd, "select UsrId,UserName from User where UserName = \"%s\";", user->UserName);

    if(mysql_query(&mdb->mysql, mdb->cmd)){
        printf("query failed: (%d) %s \n", mysql_errno(&mdb->mysql), mysql_error(&mdb->mysql));
        return false;
    }
    mdb->result = mysql_store_result(&mdb->mysql);
    if((mdb->numRows = mysql_num_rows(mdb->result)) > 0){
        printf("User record '%s' already existed!\n", user->UserName);
        mysql_free_result(mdb->result);
        return false;
    }
    mysql_free_result(mdb->result);
    memset(mdb->cmd, 0, 1024);
    sprintf(mdb->cmd, "insert into User(UserName, Password) values(\"%s\",\"%s\");", user->UserName,user->Password);
    if(mysql_query(&mdb->mysql, mdb->cmd)){
        printf("query failed: (%d) %s \n", mysql_errno(&mdb->mysql), mysql_error(&mdb->mysql));
        return false;
    }else{
        user->UserId = mysql_insert_id(mdb->db);
	 printf("%s,%d:insertion success! new UserId = %d\n", __FILE__,__LINE__,user->UserId);
    }
    mysql_commit(mdb->db);
    return true;
}

/* insert a object into object table */
bool insert_record_into_object(DedupDb *mdb, ObjectRecord *object)
{
    bool stat;
    char *szTmpDescriptor;

    ASSERT(object->UserId);
    memset(mdb->cmd, 0, 1024);
   
    sprintf(mdb->cmd, "SELECT ObjectId, ObjectName, UserId FROM Object WHERE "
            "ObjectName='%s' AND UserId=%d",
            object->ObjectName, object->UserId);
    /* if object name is existed, failed*/
    if(mysql_query(&mdb->mysql, mdb->cmd)){
        printf("query failed: (%d) %s \n", mysql_errno(&mdb->mysql), mysql_error(&mdb->mysql));
        return false;
    }
    mdb->result = mysql_store_result(&mdb->mysql);
    mdb->numRows = mysql_num_rows(mdb->result);
    if (mdb->numRows > 0) {
           printf("%s,%d:Object record '%s' for UserId=%u already exists\n", __FILE__, __LINE__, object->ObjectName, object->UserId);
           mysql_free_result(mdb->result);
           return false;
    }
    mysql_free_result(mdb->result);

    /* insert it */
    sprintf(mdb->cmd,
            "INSERT  INTO Object (ObjectName, UserId, FullCount, FileSet) VALUES ('%s', %d, 1, '%s')",
            object->ObjectName,
            object->UserId,
            object->fileset
    );

    if (mysql_query(&mdb->mysql, mdb->cmd)) {
        printf("%s,%d:Create DB Object record %s failed: ERR=%s\n", __FILE__, __LINE__, mdb->cmd, mysql_error(mdb->db));
        object->ObjectId = 0;
        stat = false;
    } else {
        object->ObjectId = mysql_insert_id(mdb->db);
        stat = true;
       // printf("%s,%d:insertion success! new ObjectId = %llu\n", __FILE__,__LINE__,object->ObjectId);
    }
    mysql_commit(mdb->db);
    return stat;
}

/* insert a job into job table */
bool insert_record_into_job(DedupDb *mdb, JobRecord* job)
{
    bool stat;
    struct tm tm;

    ASSERT(job->UserId);
    ASSERT(job->ObjectId);
	
    //(void)localtime_r(&job->StartTime, &tm);
    //strftime(job->StartTime, sizeof(job->StartTime), "%Y-%m-%d %H:%M:%S", &tm);

    //if (job->FinishTime == 0) {
    //    job->FinishTime = time(NULL);
    //}
    // (void)localtime_r(&job->FinishTime, &tm);
    //strftime(job->FinishTime, sizeof(job->FinishTime), "%Y-%m-%d %H:%M:%S", &tm);
    
    /* insert into job table */
    sprintf(mdb->cmd,
            "INSERT  INTO Job (JobType, FileCount, DataSize,"
            "UserId, ObjectId) VALUES ('%c', %d, %llu,%u,%u)",
            (char)(job->JobType), 
            job->FileCount, job->DataSize,
            job->UserId, job->ObjectId);

   if (mysql_query(&mdb->mysql, mdb->cmd)) {
        job->JobId = 0;
	 printf("%s,%d:Create DB Job record %s failed: ERR=%s . \n", __FILE__,__LINE__,mdb->cmd, mysql_error(mdb->db));
        stat = false;
    } else {
        stat = true;
        job->JobId = mysql_insert_id(mdb->db);
        stat = true;
       // printf("%s,%d:insertion success! new JobId = %d\n", __FILE__,__LINE__,job->JobId);
    }
    mysql_commit(mdb->db);
    return stat;
}


/* insert a file into file table */
bool insert_record_into_file(DedupDb *mdb, FileRecord *file)
{
    int stat;
    struct tm tm;
    MYSQL_STMT *insert_stmt;
    MYSQL_BIND param[5];

//	printf("%s,%d\n",__FILE__, __LINE__);
	
    ASSERT(file->JobId);
   
    //(void)localtime_r(&file->ModifiedTime, &tm);
    //strftime(file->ModifiedTime, sizeof(file->ModifiedTime), "%Y-%m-%d %H:%M:%S", &tm);
    char insert_sql[] = "INSERT INTO File (JobId, VersionCount, FilePath, HashCode, Size) VALUES (?, ?, ?, ?, ?);";

    insert_stmt = mysql_stmt_init(mdb->db);
	
    mysql_stmt_prepare(insert_stmt, insert_sql, strlen(insert_sql));

    unsigned long hashlen = sizeof(Fingerprint);
    unsigned long pathlen = strlen(file->FilePath);
	
    memset(param, 0, sizeof(param));
    param[0].buffer_type = MYSQL_TYPE_LONGLONG;
    param[0].buffer = &file->JobId;
    param[1].buffer_type = MYSQL_TYPE_LONG;
    param[1].buffer = &file->VersionCount;
    param[2].buffer_type = MYSQL_TYPE_BLOB;
    param[2].buffer = file->FilePath;
    param[2].buffer_length = pathlen;
    param[2].length = &pathlen;
    param[3].buffer_type = MYSQL_TYPE_BLOB;
    param[3].buffer = file->HashCode;
    param[3].buffer_length = hashlen;
    param[3].length = &hashlen;
    param[4].buffer_type = MYSQL_TYPE_LONGLONG;
    param[4].buffer = &file->Size;
	
    /* insert a record into file */
    if (mysql_stmt_bind_param(insert_stmt, param)) {
		printf("%s, %d: failed to insert file record! %s\n", __FILE__, __LINE__, mysql_stmt_error(insert_stmt));
    }

    if (mysql_stmt_execute(insert_stmt) != 0) {
		printf("%s, %d: failed to insert file record! %s\n", __FILE__, __LINE__, mysql_stmt_error(insert_stmt));
		printf("Create db File record %s failed. ERR=%s\n", mdb->cmd, mysql_error(mdb->db));
              stat = false;
    }else {
              file->FileId = mysql_insert_id(mdb->db);
              stat = true;
            //  printf("%s,%d:insertion success! new FileId = %d\n", __FILE__,__LINE__,file->FileId);
    }
    mysql_stmt_close(insert_stmt);

	//printf("%s,%d\n",__FILE__, __LINE__);
    return stat;

}


/* insert a versionfile into versionfile table */
bool insert_record_into_versionfile(DedupDb *mdb, VersionFileRecord *versionfile)
{
    int stat;
    struct tm tm;

    ASSERT(versionfile->FileId);
    ASSERT(versionfile->JobId);
 
    
    sprintf(mdb->cmd,
        "INSERT  INTO VersionFile(FileId, JobId, FilePath) "
        "VALUES (%llu,%llu,'%s')",
        versionfile->FileId, versionfile->JobId, versionfile->FilePath
    );
    if (mysql_query(&mdb->mysql, mdb->cmd)) {
	    printf("Create db VersionFile record %s failed. ERR=%s\n", mdb->cmd, mysql_error(mdb->db));
	    stat = false;
    } else {
	    stat = true;
	    printf("%s,%d:insertion versionfile success!\n", __FILE__,__LINE__);
    }
    mysql_commit(mdb->db);
    return stat;
}


/*
bool insert_record_into_segment(DedupDb *mdb)
{
	ASSERT(mdb != NULL);
	ASSERT(jcr != NULL);
	
	memset(mdb->cmd, 0, strlen(mdb->cmd));
	sprintf(mdb->cmd, "INSERT  INTO Segment (SegmentName,Hash,Size)"
				" VALUES ('%s','%s',%d)",);
	
	if(mysql_query(&mdb->mysql, mdb->cmd)){
		  printf("query failed: (%d) %s \n", mysql_errno(&mdb->mysql), mysql_error(&mdb->mysql));
		  return false;
	}
	return true;
}*/
