#include "../global.h"


/* user register */
int register_user(void *data, void *msg)
{
    bool err;
    bool ok;
    DedupClient *c;
    UsrRecord user;
    char *ibuf = (char *)msg;
    char obuf[256] = {0};

    c = (DedupClient *)data;
    if(sscanf(ibuf, register_cmd, user.UserName, user.Password) != 2) {
        return DEDUPHANDLEFAIL;
    }
    c->db = db_init_database();
    if(c->db == NULL){
        err_msg1("db_init_failed!");
        return DEDUPHANDLEFAIL;
    }
    if(db_open_database(c->db) != 1) {
        err_msg1("db_open_failed!");
        return DEDUPHANDLEFAIL;
    }
    ok = find_username(c->db, user.UserName);
    if(ok == false) {
        memcpy(obuf, register_fail, strlen(register_fail));
        err_msg1("2001 register fail:username has existed!");
        goto REGISTERFAIL;
    }
    ok = insert_record_into_user(c->db, &user);
    if(ok == false){
        err_msg1("insert_record_into_usr failed\n");
        goto REGISTERFAIL;
    }
    
    memcpy(obuf, register_ok, strlen(register_ok));
    err = bnet_send(c->client_fd, obuf, strlen(obuf));
    if(err == false){
        err_msg1("net_send failed!");
        return DEDUPHANDLEFAIL;
    }
    printf("%s,%d:register_user success!\n", __FILE__, __LINE__);
    db_close_database(c->db);
    return DEDUPHANDLETRUE;
REGISTERFAIL:
    err = bnet_send(c->client_fd, obuf, strlen(obuf));
    if(err == false) {
        err_msg1("net_send failed");
    }
    db_close_database(c->db);
    return DEDUPHANDLEFAIL;
}

/* user login */
int user_login(void *data, void *msg)
{
    int err;
    bool ok;
    DedupClient *c;
    char *ibuf = (char *)msg;
    char obuf[256] = {0};
    c = (DedupClient *)data;

    if(sscanf(ibuf, login_cmd, c->username, c->password) != 2) {
        memcpy(obuf, command_err, strlen(command_err));
        err_msg1("wrong login command!");
        goto LOGINERR;
    }
    if(listSearchKey(server->clients, c->username)) {
        memcpy(obuf, login_fail3, strlen(login_fail3));
	    err_msg1("1003 fail login:username has loged on!");
	    goto LOGINERR;
    }
    c->db = db_init_database();
    if(c->db == NULL){
        memcpy(obuf, database_err, strlen(database_err));
        err_msg1("db_init_failed!");
        goto LOGINERR;
    }
    if(db_open_database(c->db) != 1) {
        memcpy(obuf, database_err, strlen(database_err));
        err_msg1("db_open_failed!");
        goto LOGINERR;
    }
    ok = find_username(c->db, c->username);
    if(ok == false) {
        memcpy(obuf, login_fail1, strlen(login_fail1));
        err_msg1("1001 fail login:username has not existed!");
        goto LOGINERR;
    }
    ok = get_pass_by_username(c->db, c->username, c->password);
    if(ok == false) {
        memcpy(obuf, login_fail2, strlen(login_fail2));
        err_msg1("1002 fail login:password does not match with the username!");
        goto LOGINERR;
    }
    memcpy(obuf, login_ok, strlen(login_ok));
    err = bnet_send(c->client_fd, obuf, strlen(obuf));
    if(err == false){
        err_msg1("net_send failed!");
        return DEDUPHANDLEFAIL;
    }

    printf("%s,%d:%s login success\n", __FILE__, __LINE__, c->username);
    pthread_mutex_lock(&server->server_mutex);
    listAddNodeTail(server->clients, c);
    pthread_mutex_unlock(&server->server_mutex);
    return DEDUPHANDLETRUE;
LOGINERR:
    err = bnet_send(c->client_fd, obuf, strlen(obuf));
    if(err == false) {
	    err_msg1("net_send failed\n");
    }
    return DEDUPHANDLEFAIL;
}


int logout(void *data, void *msg)
{
    int err;
    bool ok;
    listNode *ln;
    DedupClient *c;
    char *ibuf = (char *)msg;
    char obuf[256] = {0};
    c = (DedupClient *)data;

    if(sscanf(ibuf, logout_cmd, c->username) != 1) {
        err_msg1("wrong logout command!");
        return false;
    }
    ok = find_username(c->db, c->username);
    if(ok == false) {
        memcpy(obuf, logout_fail2, strlen(logout_fail2));
        err_msg1("3002 logout fail:username has not existed!");
        goto LOGOUTERR;
    }
    if((ln = listSearchKey(server->clients, c->username)) == NULL) {
        memcpy(obuf, logout_fail1, strlen(logout_fail1));
        err_msg1("3001 logout fail:username has quited!");
        goto LOGOUTERR;
    }

    pthread_mutex_lock(&server->server_mutex);
    listDelNode(server->clients, ln);
    pthread_mutex_unlock(&server->server_mutex);

    memcpy(obuf, logout_ok, strlen(logout_ok));
    err = bnet_send(c->client_fd, obuf, strlen(obuf));
    if(err == false) {
        err_msg1("bnet_send failed!");
        return DEDUPHANDLEFAIL;
    }
    printf("%s,%d:%s logout success\n", __FILE__, __LINE__, c->username);
    return DEDUPHANDLETRUE;
LOGOUTERR:
    err = bnet_send(c->client_fd, obuf, strlen(obuf));
    if(err == false) {
        err_msg1("net_send failed\n");
    }
    return DEDUPHANDLEFAIL;
}

/* job backup */
int new_backup(void *data, void *msg)
{
    int err;
    int len;
    bool ok;
    int type;
    time_t current_time;
    struct tm *current_tm;
    int32_t chunknum;
    DedupClient *c;
    bool result = true; 
    IndexItem *index_item = NULL;
    char *ibuf = (char *)msg;
    char *buf=(char *)malloc(SOCKET_BUF_SIZE);
    char s_time[128] = {0};
    JCR *jcr = NULL;
    Recipe *rp = NULL;
    ObjectRecord *object;
    JobRecord *job;
    FileRecord *file;
    VersionFileRecord *versionfile;
    c = (DedupClient *)data;

    jcr = jcr_new(); 
    
    if(sscanf(ibuf, backup_cmd, jcr->backup_path) != 1) {
        err_msg1("wrong backup command!");
        return false;
    }

    time(&current_time);
    current_tm = localtime(&current_time);
    strftime(s_time, sizeof(s_time), "%Y%m%d_%H_%M_%S", current_tm);
	
    printf("%s,%d:%s\n", __FILE__,__LINE__,jcr->backup_path);
    object = (ObjectRecord *)malloc(sizeof(ObjectRecord));
    memset(object, 0, sizeof(object));
    object->UserId = c->user_id;
    sprintf(object->ObjectName, "%c%s%s", 'f', c->username, s_time);
	strcpy(object->fileset, jcr->backup_path);
    ok = insert_record_into_object(c->db, object);
    if(ok == false){
        err_msg1("insert_record_into_object failed\n");
        result = false;
    }else {
         jcr->object_id = object->ObjectId;
    }
    job = (JobRecord *)malloc(sizeof(JobRecord));
    memset(job, 0, sizeof(job));
    job->UserId = c->user_id;
    job->ObjectId = object->ObjectId;
    job->JobType = 'f';
    job->FileCount = 0;
    job->DataSize = 0;
    ok = insert_record_into_job(c->db, job);
    if(ok == false){
        err_msg1("insert_record_into_job failed\n");
        result = false;
    }else {
        jcr->job_id = job->JobId;
    }
    file = (FileRecord *)malloc(sizeof(FileRecord));
    versionfile = (VersionFileRecord *)malloc(sizeof(VersionFileRecord));
    memset(buf, 0, SOCKET_BUF_SIZE);
    while(bnet_recv(c->client_fd, buf, &len) != ERROR) {
        if(len == STREAM_END) {
            printf("%s %d backup is over\n",__FILE__, __LINE__);
            break;
        }
        if(sscanf(buf,"%d", &type) != 1)
            goto FAIL;
        switch(type) {
            case FILE_HASH:
		  		rp = recipe_new();
		  		jcr->file_count++;
                memset(buf, 0, SOCKET_BUF_SIZE);
		  		bnet_recv(c->client_fd, buf, &len);
		  		memcpy(rp->file_hash, buf, len);
                /* check the file is new or old */
                file_dedup_mark(c, rp);
                /* if the file is old, send the chunk info to client */
                if(rp->is_new == DEDUP_FILE) {
                    send_chunk_info(c, rp);
                }
                break;
            case FILE_RECIPE:
                /* recive file chunk information if the file is not dedup */
				memset(buf, 0, SOCKET_BUF_SIZE);
                bnet_recv(c->client_fd, buf, &len);
                if(sscanf(buf, update_attr, &chunknum, rp->filename) != 2) {
                    err_msg1("wrong updateattr command!");
                    result = false;
                }
				printf("%s,%d:current backup file, %s\n",__FILE__,__LINE__,rp->filename);
                /* if the file is new, recive the recipe and write file recipe to file */
                if(rp->is_new == NEW_FILE) {
					recv_deplicate_chunk(c, rp, chunknum);
                    index_item = (IndexItem *)malloc(sizeof(IndexItem));
                    memset(index_item, 0, sizeof(index_item));
                    memcpy(index_item->hash, rp->file_hash, sizeof(Fingerprint));
                    err = write_recipe_to_vol(&index_item->offset, rp);
                    if(err == false) {
                        err_msg1("read file recipe error!");
                        result = false;
                    }
                    index_insert(index_item);
                }
				
				
		 	
			   
                /* update database */
		 		memset(file, 0, sizeof(file));
		 		file->JobId = jcr->job_id;
		 		memset(file->FilePath, 0, sizeof(file->FilePath));
		 		memcpy(file->FilePath, rp->filename, strlen(rp->filename));
		 		memcpy(file->HashCode, rp->file_hash, FINGER_LENGTH);
				file->Size = 0;
		 		file->VersionCount = 1;
		 		ok = insert_record_into_file(c->db, file);
		 		if(ok == false){
		       		err_msg1("insert_record_into_file failed\n");
		       		result = false;
				}
				
		 		memset(versionfile, 0, sizeof(versionfile));
		 		versionfile->JobId = jcr->job_id;
		 		versionfile->FileId = file->FileId;
		 		memset(versionfile->FilePath, 0, sizeof(versionfile->FilePath));
		 		memcpy(versionfile->FilePath, rp->filename, strlen(rp->filename));
		 		ok = insert_record_into_versionfile(c->db, versionfile);
		 		if(ok == false){
		       		err_msg1("insert_record_into_versionfile failed\n");
		       		result = false;
               	}
				
		 		if(result == false) {
					memset(buf, 0, SOCKET_BUF_SIZE);
					memcpy(buf, updateattr_fail, strlen(updateattr_fail));
					printf("%s,%d:updateattr_fail\n",__FILE__,__LINE__);
		 		}else {
					memset(buf, 0, SOCKET_BUF_SIZE);
					memcpy(buf, updateattr_ok, strlen(updateattr_ok));
					printf("%s,%d:updateattr_ok\n",__FILE__,__LINE__);
		 		}
		 		err = bnet_send(c->client_fd, buf, strlen(buf));
				if(err == false) {
                      err_msg1("bnet_send failed!");
				}
				
	        	recipe_free(rp);
				break;
           default:
                printf("%s %d wrong\n",__FILE__,__LINE__);
                break;
        }
    }
    memset(buf, 0, SOCKET_BUF_SIZE);
    memcpy(buf, backup_ok, strlen(backup_ok));
    err = bnet_send(c->client_fd, buf, strlen(buf));
    if(err == false) {
        err_msg1("bnet_send failed!");
        return DEDUPHANDLEFAIL;
    }
	
    printf("%s,%d:=============backup success==============\n", __FILE__, __LINE__);
    free(object);
    free(job);
    free(file);
    free(versionfile);
    jcr_free(jcr);
    free(buf);
    return DEDUPHANDLETRUE;
FAIL:
    memset(buf, 0, SOCKET_BUF_SIZE);
    memcpy(buf, backup_fail, strlen(backup_fail));
    err = bnet_send(c->client_fd, buf, strlen(buf));
    if(err == false) {
        err_msg1("bnet_send failed!");
    }
	
    printf("%s,%d:================backup error==============\n", __FILE__, __LINE__);
    free(object);
    free(job);
    free(file);
    free(versionfile);
    jcr_free(jcr);
    free(buf);
    return DEDUPHANDLEFAIL;
}

int restore(void *data, void *msg)
{
	int err,len,filenum;
	char *ibuf = (char *)msg;
	DedupClient *client;
	int job_id;
	char *fileinfo_buf;

	client = (DedupClient *)data;

	
    if(sscanf(ibuf, restore_cmd, &(job_id)) != 1) {
        err_msg1("wrong restore command!");
        return FAILURE;
    }
	
	Queue *filelist = queue_new();
	
	if (!db_get_job_files(client->db, job_id, filelist)) {
		err_msg1("get job infomation error");
		return FAILURE;
	}
	
	err = send_job_info(client, filelist);
	if (err == FAILURE) {
		err_msg1("sent job info error!");
		return FAILURE;
	}

	Recipe *recipe;

    for(filenum = queue_size(filelist); filenum>0; filenum--) {
        fileinfo_buf = (char *)queue_pop(filelist);
		recipe = recipe_new();
		memcpy(recipe->file_hash, fileinfo_buf+sizeof(int32_t), FINGER_LENGTH);
		
		printf("%s, %d, file %s \n", __FILE__, __LINE__, fileinfo_buf+sizeof(int32_t)+FINGER_LENGTH);
		
		get_file_info(client,recipe);
		send_chunk_info(client, recipe);
		recipe_free(recipe);
		free(fileinfo_buf);
    }
	
	return SUCCESS;
}


int delete_job(void *data, void *msg)
{
    int err;
    bool ok;
    DedupClient *c;
    char *ibuf = (char *)msg;
    char obuf[256] = {0};

    c = (DedupClient *)data;
    c->jcr = jcr_new();

    if(sscanf(ibuf, delete_cmd, &(c->jcr->job_id)) != 1) {
        err_msg1("wrong delete command!");
        return FAILURE;
    }
   if (!db_delete_job(c->db, c->jcr->job_id)) {
		memcpy(obuf, delete_fail, strlen(delete_fail));
		goto LOGOUTERR;
   }

    memcpy(obuf, delete_ok, strlen(delete_ok));
    err = bnet_send(c->client_fd, obuf, strlen(obuf));
    if(err == false) {
        err_msg1("bnet_send failed!");
        return FAILURE;
    }
    printf("%s,%d:delete success\n", __FILE__, __LINE__);
    return SUCCESS;
	
LOGOUTERR:
    err = bnet_send(c->client_fd, obuf, strlen(obuf));
    if(err == false) {
        err_msg1("net_send failed\n");
    }
    return FAILURE;

}

int list_job(void *data, void *msg)
{
	int err;
	DedupClient *client;

	client = (DedupClient *)data;

	Queue *filelist = queue_new();
	if (!db_get_job_list(client->db,filelist)) {
		err_msg1("get job list error!");
		return FAILURE;
	}
	
	if (!queue_size(filelist)) {
		char *buf = (char *)malloc(strlen(list_none));
		strcpy(buf, list_none);
		
		err = bnet_send(client->client_fd, buf, strlen(list_none));
 		if (err == false) {
    		err_msg1("bnet_send failed!");
			return FAILURE;
  		}
		free(buf);
		
	}
	
	else{
		err = send_job_info(client, filelist);
		if (err == FAILURE) {
			err_msg1("sent job list error!");
			return FAILURE;
		}

	}
	
	return SUCCESS;
	
}


