#include "../global.h"

int64_t download_size = 0;
int download_count = 0;

/* user register */
int register_user(Client *client)
{
    int err;
    int len;
    char buf[256] = {0};

    if(client->fd < 0){
	    return FAILURE;
    }

    sprintf(buf, register_cmd, client->username, client->password);

    err = bnet_send(client->fd, buf, strlen(buf));
    if(err <= 0) {
	    err_msg1("bnet_sent failed!");
	    return FAILURE;
    }

    printf("recieve register result from dird!");
    memset(buf, 0, sizeof(buf));
    err = bnet_recv(client->fd, buf, &len);
    if(err <= 0){
	    err_msg1("bnet_recv failed!");
	    return FAILURE;
    }

    if(strncmp(buf, register_ok, strlen(register_ok)) != 0) {
    	if(strncmp(buf, register_fail, strlen(register_fail)) == 0) {
            printf("%s,%d:fail register,username %s has existed!\n", __FILE__, __LINE__, client->username);
	        return FAILURE;
    	}
    }
	printf("%s,%d:success register,user %s regiser success!\n", __FILE__, __LINE__, client->username);
	return SUCCESS;
}


/* user login */
int login(Client *client)
{
    int err;
    int len;
    char buf[256] = {0};

    if(client->fd < 0){
        return FAILURE;
    }

    sprintf(buf, login_cmd, client->username, client->password);

    err = bnet_send(client->fd, buf, strlen(buf));
    if(err <= 0) {
        err_msg1("bnet_sent failed!");
        return FAILURE;
    }

    printf("recieve login result from dird!");
    memset(buf, 0, sizeof(buf));
    err = bnet_recv(client->fd, buf, &len);
    if(err <= 0){
        err_msg1("bnet_recv failed!");
        return FAILURE;
    }

    if(strncmp(buf, login_ok, strlen(login_ok)) != 0) {
        if(strncmp(buf, login_fail1, strlen(login_fail1)) == 0) {
            printf("%s,%d:fail login,username %s has not existed!\n", __FILE__, __LINE__, client->username);
            return FAILURE;
        }else if(strncmp(buf, login_fail2, strlen(login_fail2)) == 0){
            printf("%s,%d:fail login,password does not match username %s!\n", __FILE__, __LINE__, client->username);
            return FAILURE;
        }else if(strncmp(buf, login_fail3, strlen(login_fail3)) == 0){
            printf("%s,%d:fail login,username %s username has loged on!\n", __FILE__, __LINE__, client->username);
            return FAILURE;
        }else if(strncmp(buf, command_err, strlen(command_err)) == 0){
            printf("%s,%d:command error!\n", __FILE__, __LINE__);
            return FAILURE;
        }else if(strncmp(buf, database_err, strlen(database_err)) == 0){
            printf("%s,%d:Database error!\n", __FILE__, __LINE__);
            return FAILURE;
        }
    }
    printf("%s,%d:success login,user %s login success!\n", __FILE__, __LINE__, client->username);;
    return SUCCESS;
}

/* user logout */
int logout(Client *client)
{
    int err;
    int len;
    char buf[256] = {0};

    if(client->fd < 0) {
        return FAILURE;
    }

    if(client->jcr != NULL) {
        err_msg1("Job has not finished, cannot logout!");
        return FAILURE;
    }
    sprintf(buf, logout_cmd, client->username);

    err = bnet_send(client->fd, buf, strlen(buf));
    if(err <= 0) {
        err_msg1("bnet_sent failed!");
        return FAILURE;
    }

    printf("recieve logout result from dird!");
    memset(buf, 0, sizeof(buf));
    err = bnet_recv(client->fd, buf, &len);
    if(err <= 0) {
        err_msg1("bnet_recv failed!");
        return FAILURE;
    }

    if(strncmp(buf, logout_ok, strlen(logout_ok)) != 0) {
        if(strncmp(buf, logout_fail1, strlen(logout_fail1)) == 0) {
            printf("%s,%d:fail logout,username %s has quited!\n", __FILE__, __LINE__, client->username);
            return FAILURE;
        }else if(strncmp(buf, logout_fail2, strlen(logout_fail2)) == 0){
            printf("%s,%d:fail logout,username %s has not existed!\n", __FILE__, __LINE__, client->username);
            return FAILURE;
        }
    }
    printf("%s,%d:success logout,username %s logout success!\n", __FILE__, __LINE__, client->username);;
    free(client);
    return SUCCESS;
}

/* backup a job */
int backup_client(Client *c, char *path, char *output_path)
{
    char buf[256] = {0};
	int len = 0;
	int err;
	struct stat state;
    bool result;
	
	TIMER_DECLARE(start,end);
	TIMER_START(start);
	
    c->jcr = jcr_new();
    strcpy(c->jcr->backup_path, path);
	printf("jcr path is %s\n", c->jcr->backup_path);

    if (access(c->jcr->backup_path, F_OK) != 0) {
        err_msg1("This path does not exist or can not be read!");
        return FAILURE;
    }

    if (stat(c->jcr->backup_path, &state) != 0) {
        err_msg1("backup path does not exist!");
        return FAILURE;
    }
    
    sprintf(buf, backup_cmd, c->jcr->backup_path);
    err = bnet_send(c->fd, buf, strlen(buf));
    if(err <= 0) {
        err_msg1("bnet_sent failed!");
        return FAILURE;
    }
    printf("%s,%d:%s\n",__FILE__, __LINE__, c->jcr->backup_path);

    /* init Extremebinning and load the primary index*/
    ExtremeBinningInit();
	store_init();
	
	if(REWRITE == CFL_REWRITE)
		cfl_init();
	else if (REWRITE == PERFECT_REWRITE)
		perfect_rewrite_init();
	
    if(S_ISREG(state.st_mode)) {
        char *p = c->jcr->backup_path + strlen(c->jcr->backup_path) - 1;
        while (*p != '/')
            --p;
        *(p + 1) = 0;
        file_dudup(c, path);
    }else {
        len = strlen(c->jcr->backup_path);
        if(c->jcr->backup_path[len-1] != '/')
            c->jcr->backup_path[len] = '/';
        walk_dir(c, c->jcr->backup_path);
    }
	
    /* send backup end message */
    bnet_signal(c->fd, STREAM_END);

    memset(buf, 0, sizeof(buf));
    err = bnet_recv(c->fd, buf, &len);
    if(err <= 0) {
        err_msg1("bnet_recv failed!");
        return FAILURE;
    }
    if(strncmp(buf, backup_ok, strlen(backup_ok)) == 0) {
        printf("%s,%d:=============backup success!================\n", __FILE__, __LINE__);
        result = true;
    }else {
        printf("%s,%d:=============backup error!================\n", __FILE__, __LINE__);
        result = false;
    }

	TIMER_END(end);
	TIMER_DIFF(c->jcr->total_time, start, end);
	
	/*print the deduplication information*/
	if(OUTPUT_RESULT){
		int dedup_result, tmplen;
		dedup_result = open(output_path, O_RDWR|O_CREAT|O_APPEND, 0777);
		char tmp[256];
		tmplen = sprintf(tmp, "======== %s ========\n", path);
		write(dedup_result, tmp, tmplen);
		tmplen = sprintf(tmp, "total size is %llu\n", c->jcr->old_size);
		write(dedup_result, tmp, tmplen);
		tmplen = sprintf(tmp, "dedup size is %llu\n", c->jcr->total_dedup_size);
		write(dedup_result, tmp, tmplen);

		tmplen = sprintf(tmp, "total time is %.3fs\n", c->jcr->total_time);
		write(dedup_result, tmp, tmplen);
		
		tmplen = sprintf(tmp, "file dedup time is %.3fs\n", c->jcr->file_dedup_time);
		write(dedup_result, tmp, tmplen);

		tmplen = sprintf(tmp, "recv chunk info time is %.3fs\n", c->jcr->recv_chunk_info_time);
		write(dedup_result, tmp, tmplen);

		tmplen = sprintf(tmp, "send recipe time is %.3fs\n", c->jcr->send_recipe_time);
		write(dedup_result, tmp, tmplen);
		
		close(dedup_result);
		
	}

	jcr_free(c->jcr);
	
	if(REWRITE == CFL_REWRITE)
		cfl_destory();
	else if (REWRITE == PERFECT_REWRITE)
		perfect_rewrite_destory();
	
    /* write all bin to disk and update the primary index */ 
    free_cache_bin();

    /* write primary index and free bin */ 
    ExtremeBinningDestroy();
	store_destory();
	
	if(result == true) {
        return SUCCESS;
    }else {
        return FAILURE;
    }
}

/* restore */
int restore_client(Client *c, char *path, char *output_path)
{

	int err;
	JCR *jcr = jcr_new();

	if(REWRITE != NO_REWRITE)
		ExtremeBinningInit();
	
	download_size = 0;
	
	//printf("%s,%d, in restore clent, %s\n", __FILE__,__LINE__, path);
	memset(jcr->restore_path, 0, sizeof(jcr->restore_path));
	sscanf(path, "%d,%s", &(jcr->id), jcr->restore_path);
	if(*(jcr->restore_path + strlen(jcr->restore_path) - 1) == '/')
		*(jcr->restore_path + strlen(jcr->restore_path) - 1) = 0;
	printf("%s,%d, in restore clent, job id is %d\n", __FILE__,__LINE__, jcr->id);
	c->jcr = jcr;


	DList *filelist = get_job_info(c);
	if (filelist == NULL) {
		err_msg1("get job infomation error!");
		return FAILURE;
	}
	
	err = get_files(c, filelist);
	if (err == FAILURE) {
		err_msg1("get file infomation error!");
		return FAILURE;
	}

	if(OUTPUT_RESULT){
		int dedup_result, tmplen;
		dedup_result = open(output_path, O_RDWR|O_CREAT|O_APPEND, 0777);
		char tmp[256];
		tmplen = sprintf(tmp, "========job id %d========\n", jcr->id);
		write(dedup_result, tmp, tmplen);
		tmplen = sprintf(tmp, "download count is %d\n", download_count);
		write(dedup_result, tmp, tmplen);
		tmplen = sprintf(tmp, "download size is %llu B\n\n\n\n", download_size);
		write(dedup_result, tmp, tmplen);
	}

	printf("%s, %d, ======restore job %llu success======\n", __FILE__, __LINE__, jcr->id);

	if (REWRITE != NO_REWRITE) {
		free_cache_bin();
		ExtremeBinningDestroy();
		//free(exbin);
	}

	jcr_free(jcr);
	
	
	return SUCCESS;
}

/* delete according to job id*/
int delete_client(Client *c, int  job_id)
{
       int err;
       int len;
       char buf[256] = {0};

       if(c->fd < 0){
             return FAILURE;
       }

       sprintf(buf, delete_cmd, job_id);

       err = bnet_send(c->fd, buf, strlen(buf));
       if(err <= 0) {
           err_msg1("bnet_sent failed!");
           return FAILURE;
       }
	memset(buf, 0, sizeof(buf));
       err = bnet_recv(c->fd, buf, &len);
       if(err <= 0) {
            err_msg1("bnet_recv failed!");
            return FAILURE;
       }
       if(strncmp(buf, delete_ok, strlen(delete_ok)) == 0) {
            printf("%s,%d:=============delete success!================\n", __FILE__, __LINE__);
            return SUCCESS;
       }else {
            printf("%s,%d:=============delete error!================\n", __FILE__, __LINE__);
            return FAILURE;
       }
}

/* quit */
int quit_client(Client *c)
{
	bnet_signal(c->fd, SYSTEM_QUIT);
}

int list_client(Client *client)
{

	int err,len;
	char buf[256] = {0};
	char *joblist_buf, *p;
	int32_t job_num;

	if(client->fd <0)
		return FAILURE;
	
	strcpy(buf, list_cmd);
	err = bnet_send(client->fd, buf, strlen(buf));
	if (err <=0) {
		err_msg1("send list cmd error!");
		return FAILURE;
	}

	memset(buf, 0, sizeof(buf));
    err = bnet_recv(client->fd, buf, &len);
    if(err <= 0) {
        err_msg1("bnet_recv failed!");
        return FAILURE;
    }
	
	/*no job available*/
	if (strcmp(buf, list_none) == 0) {
		printf("No Job Available!\n");
		
	}
	else{

		memcpy(&job_num, buf, sizeof(int32_t));

		joblist_buf =(char *) malloc(job_num *(32 + FILE_NAME_LEN+sizeof(int32_t)));
	
	
		err = bnet_recv(client->fd, joblist_buf, &len);
	 	if(err <= 0) {
        	err_msg1("bnet_recv failed!");
        	return FAILURE;
    	}

		printf("The Available Jobs Are:\n");
		p = joblist_buf;
		int i;
		for (i = 0; i < job_num; i++) {
			int64_t jobid;
			int32_t jobinfo_len;
			char fileset[FILE_NAME_LEN+32] = {0};
			memcpy(&jobinfo_len, p, sizeof(int32_t));
			p += sizeof(int32_t);
			memcpy(fileset, p, jobinfo_len);
			p += jobinfo_len;
			printf("%s\n", fileset);		
		}

		free(joblist_buf);
	}
	return SUCCESS;

}
