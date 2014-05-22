#include "../global.h"

SyncQueue *file_dedup_queue = NULL;
SyncQueue *chunk_dedup_queue = NULL;
SyncQueue *update_recipe_queue = NULL;

extern float segment_usage_threshold;

extern double read_time;
//LRUCache *pipeline_traditional_restore_cache;

void * file_dedup_module(void *arg)
{
	int len;
	Client *c = (Client *)arg;
	char path[FILE_NAME_LEN];
	struct stat state;
	
	if (stat(c->jcr->backup_path, &state) != 0) {
		puts("backup path does not exist!");
		return FAILURE;
	}

	memcpy(path, c->jcr->backup_path, strlen(c->jcr->backup_path));

	//printf("%s,%d\n",__FILE__, __LINE__);
	
	if(S_ISREG(state.st_mode)) {
        char *p = c->jcr->backup_path + strlen(c->jcr->backup_path) - 1;
        while (*p != '/')
            --p;
        *(p + 1) = 0;
        pipeline_file_dedup(c, path);
    }else {
        len = strlen(c->jcr->backup_path);
        if(path[len-1] != '/')
            path[len] = '/';
        walk_dir(c, path);
		
    }
	
	bnet_signal(c->fd, STREAM_END);
	pipeline_file_dedup(NULL, NULL);
	return NULL;
	

}


int pipeline_file_dedup(Client *c, char *path)
{

	int err;
	int is_new;
    FileInfo *fileinfo = NULL;
	char buf[256] = {0};
	struct stat file_stat;
	
	fileinfo = file_new();
	if (c == NULL) {
		fileinfo->chunknum = STREAM_END;
		sync_queue_push(file_dedup_queue, fileinfo);
		return SUCCESS;
	}

	//int file_order_log = open("test/file_order_log", O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
	//char tmp[256];
	//int tmplen;
	//tmplen = sprintf(tmp, "%s\n", path);
	//write(file_order_log, tmp, tmplen);
	//close(file_order_log);
	
	stat(path, &file_stat);
	fileinfo->file_size = file_stat.st_size;
	c->jcr->old_size += file_stat.st_size;
	c->jcr->file_count += 1;
	
    memcpy(fileinfo->file_path, path, strlen(path));
    err = SHA1File(path, fileinfo->file_hash);
	if(err != shaSuccess) {
        err_msg1("calculate file hash failed!");
        return FILEDEDUPERROR;
	}

	memcpy(buf, (char *)fileinfo->file_hash, FINGER_LENGTH);
	err = bnet_send(c->fd, buf, FINGER_LENGTH);
	if(err == false){
		err_msg1("net_send failed!");
		return FAILURE;
	}
	
	//printf("%s,%d\n",__FILE__, __LINE__);
	sync_queue_push(file_dedup_queue, fileinfo);

}


void * chunk_dedup_module(void *arg)
{

	char buf[256];
	int err, len;
	char success, newfile;
	Client *c = (Client *)arg;
	FileInfo *fileinfo;
	
	while (fileinfo = (FileInfo *)sync_queue_pop(file_dedup_queue)) {

		if (fileinfo->chunknum == STREAM_END) {
			printf("%s, %d\n", __FILE__, __LINE__);
			/*terminate the cfl process*/
			if(REWRITE == CFL_REWRITE)
				cfl_process(fileinfo, NULL);
			else if (REWRITE == PERFECT_REWRITE)
				perfect_rewrite_process(fileinfo,NULL);
			else if (REWRITE == CAPPING_REWRITE)
				capping_process(fileinfo, NULL);
			
			sync_queue_push(chunk_dedup_queue, fileinfo);
			break;
		}

		memset(buf, 0, sizeof(buf));
		err = bnet_recv(c->fd, buf, &len);
		if(err == false){
		err_msg1("net_recv failed!");
		return FAILURE;
		}
		
	//	printf("%s,%d\n",__FILE__, __LINE__);
		
		success = *((char *)buf);
		if(success == FAILURE) {
			err_msg1("other err happen in dir");
			continue;
		}
		newfile = *((char *)(buf + 1));

		if(newfile == NEW_FILE) {
			printf("%s, %d\n", __FILE__, __LINE__);
			fileinfo->is_new = NEW_FILE;
			err = chunk_dedup(c, fileinfo); 
			if(err != CHUNKDEDUPSUCCESS) {
				err_msg1("chunk_dedup error\n");
				continue;
			}

		}else if(newfile == DEDUP_FILE){
				
			fileinfo->is_new = DEDUP_FILE;
			
			err = recv_deplicate_chunk(c, fileinfo); 
			if(err == FAILURE) {
				err_msg1("recv_deplicate_chunk error\n");
				continue;
			}

			if (REWRITE != NO_REWRITE) {
				/*如果是重复文件，而且进行重写，则仍需要进行块级重删*/
				printf("%s, %d\n", __FILE__, __LINE__);
				FingerChunk *fchunk = fileinfo->first;
    			while(fchunk){
        			fileinfo->first = fchunk->next;
        			free(fchunk);
       				fchunk = fileinfo->first;    
    			} 
				fileinfo->chunknum = 0;
				fileinfo->first = NULL;
				fileinfo->last = NULL;

				err = chunk_dedup(c,fileinfo);
				if (err != CHUNKDEDUPSUCCESS) {
					err_msg1("chunk_dedup error\n");
					continue;
				}
				printf("%s, %d\n", __FILE__, __LINE__);
			}
			
		}

		
		sync_queue_push(chunk_dedup_queue, fileinfo);
		printf("%s, %d\n", __FILE__, __LINE__);
	}
	
	return NULL;
}



void *update_recipe_module(void *arg)
{
	int err, last_chunknum = -1;
	char *buf = NULL, *p;
	char msg[FILE_NAME_LEN + 128] = {0};
	Client *c = (Client *)arg;
	FileInfo *fileinfo;
	FingerChunk *fc;
	
	while (fileinfo = (FileInfo *)sync_queue_pop(chunk_dedup_queue)) {
		if (fileinfo->chunknum == STREAM_END)
			break;

		printf("%s, %d\n", __FILE__, __LINE__);
		sprintf(msg, update_attr, fileinfo->chunknum, fileinfo->file_path);
   	
		err = bnet_send(c->recipe_fd, msg, strlen(msg));
		if(err == false) {
			err_msg1("bnet_send updateattr msg failed");
			file_free(fileinfo);
			continue;
		}
	//	printf("%s,%d\n",__FILE__, __LINE__);
		
		if (fileinfo->is_new == DEDUP_FILE) {
			/*在重写模式下，重复文件也要进行重写，故要先锁住等待*/
			if (REWRITE != NO_REWRITE) 
				for (fc = fileinfo->first; fc != NULL; fc = fc->next) {
					pthread_mutex_lock(&fc->mutex);
					if (fc->is_new == DEDUP_CHUNK) {
						c->jcr->total_dedup_size += fc->chunklen;
					}
					c->jcr->total_chunk_size += fc->chunklen;
					pthread_mutex_unlock(&fc->mutex);
				}

			else if(REWRITE == NO_REWRITE) {
				c->jcr->total_dedup_size += fileinfo->file_size;
			}
				
        	sync_queue_push(update_recipe_queue, fileinfo);
          
    	}else if(fileinfo->is_new == NEW_FILE) {
			
		//	printf("%s,%d,%s\n",__FILE__, __LINE__, msg);
			
     		if(fileinfo->chunknum > last_chunknum){
        		buf = (char *)zrealloc(buf, FINGER_LENGTH + 
        			fileinfo->chunknum*(FINGER_LENGTH + CHUNK_ADDRESS_LENGTH)+1);
				last_chunknum = fileinfo->chunknum;
     		}
			
			memset(buf, 0, sizeof(buf));
			p = buf;
			memcpy(p, fileinfo->file_hash, FINGER_LENGTH);
			p += FINGER_LENGTH;

			for (fc = fileinfo->first; fc != NULL;  fc = fc->next) {
				printf("%s, %d\n", __FILE__, __LINE__);
            	pthread_mutex_lock(&(fc->mutex));
				printf("%s, %d\n", __FILE__, __LINE__);
				if (fc->is_new == DEDUP_CHUNK) {
					c->jcr->total_dedup_size += fc->chunklen;
				}
				c->jcr->total_chunk_size += fc->chunklen;

				memcpy(p, fc->chunk_hash, FINGER_LENGTH);
				p += FINGER_LENGTH;
            	memcpy(p, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
				p += CHUNK_ADDRESS_LENGTH;
				
				pthread_mutex_unlock(&(fc->mutex));
				 	
        	}

		//	printf("%s,%d,%d\n",__FILE__, __LINE__, p-buf);
			
			err = bnet_send(c->recipe_fd, buf, p-buf);
            if(err == false) {
                err_msg1("bnet_send updateattr msg failed");
				file_free(fileinfo);
                continue;
            }
			
		//	for (fc = fileinfo->first; fc != NULL; fc = fc->next) {
				//if (!container_cache_simulator_look(pipeline_traditional_restore_cache, fc->chunkaddress)) {
				//	container_cache_simulator_insert(pipeline_traditional_restore_cache, fc->chunkaddress);
			//	}
			//}
			
			sync_queue_push(update_recipe_queue, fileinfo);	
		}
		
	}

	zfree(buf);
	sync_queue_push(update_recipe_queue, fileinfo);
	return NULL;

}



void *recive_result_module(void *arg)
{
	Client *c = (Client *)arg;
	FileInfo *fileinfo;
	char msg[256] = {0};
	int msg_len;
	int err;
	
	while (fileinfo = (FileInfo *)sync_queue_pop(update_recipe_queue)) {

		if (fileinfo->chunknum == STREAM_END) {
			file_free(fileinfo);
			break;
		}
	//	printf("%s,%d\n",__FILE__, __LINE__);
		

		memset(msg, 0, sizeof(msg));
   		err = bnet_recv(c->recipe_fd, msg, &msg_len);
    	if(err == false){
			err_msg1("bnet_recv failed");
			file_free(fileinfo);
        	continue;
    	}
   
		
   		if(strncmp(msg, updateattr_ok, strlen(updateattr_ok)) == 0) {
			printf("%s,%d:success update attr!\n", __FILE__,__LINE__);
		}else if(strncmp(msg, updateattr_fail, strlen(updateattr_fail)) == 0) {
           	err_msg1("fail updateattr");
		} 


		file_free(fileinfo);
		
	}
	
	return NULL;
}
int pipeline_backup(Client *c, char *path, char *output_path) {

	char buf[256]={0};
	int err;
	int len=0;
	
	pthread_t  file_dedup_thread, chunk_dedup_thread, update_recipe_thread, recive_result_thread;
	 
	c->jcr = jcr_new();
	strcpy(c->jcr->backup_path, path);
	//printf("jcr path is %s\n", c->jcr->backup_path);

	if (access(c->jcr->backup_path, F_OK) != 0) {
		puts("This path does not exist or can not be read!");
		return FAILURE;
	}
	
	//pipeline_traditional_restore_cache = container_cache_simulator_new(4);
	
	ExtremeBinningInit();
	store_init();
	if(REWRITE == CFL_REWRITE)
		cfl_init();
	else if (REWRITE == PERFECT_REWRITE)
		perfect_rewrite_init();
	else if(REWRITE == CAPPING_REWRITE)
		capping_init();

	if (!(file_dedup_queue = sync_queue_new())) {
		err_msg1("file dedup queue init error!");
		return false;
	}

	if (!(chunk_dedup_queue = sync_queue_new())) {
		err_msg1("chunk dedup queue init error!");
		return false;
	}
	
	if (!(update_recipe_queue = sync_queue_new())) {
		err_msg1("update dedup queue init error!");
		return false;
	}
	
	TIMER_DECLARE(start,end);
	TIMER_START(start);

	/*send backup request*/
	sprintf(buf, backup_cmd, c->jcr->backup_path);
    err = bnet_send(c->fd, buf, strlen(buf));
    if(err <= 0) {
        err_msg1("bnet_sent failed!");
        return FAILURE;
    }
	
    pthread_create(&file_dedup_thread, NULL, file_dedup_module, c);
    pthread_create(&chunk_dedup_thread, NULL, chunk_dedup_module, c);
    pthread_create(&update_recipe_thread, NULL, update_recipe_module, c);
	pthread_create(&recive_result_thread, NULL, recive_result_module, c);

	 
 	
	//get_socket_default_bufsize(jcr->finger_socket);
	//set_sendbuf_size(jcr->finger_socket,0);
	//set_recvbuf_size(jcr->finger_socket,0);
	
	
	pthread_join(file_dedup_thread, NULL);
	pthread_join(chunk_dedup_thread, NULL);
	pthread_join(update_recipe_thread, NULL);
	pthread_join(recive_result_thread, NULL);

	sync_queue_free(file_dedup_queue);
	sync_queue_free(chunk_dedup_queue);
	sync_queue_free(update_recipe_queue);

 
	TIMER_END(end);
	TIMER_DIFF(c->jcr->total_time,start,end);


	if (OUTPUT_RESULT) {
		char is_rewrite[128];
		if(REWRITE == CFL_REWRITE)
			strcpy(is_rewrite, "with cfl rewrite");
		else if(REWRITE == PERFECT_REWRITE)
			strcpy(is_rewrite, "with perfect rewrite");
		else if(REWRITE == CAPPING_REWRITE)
			strcpy(is_rewrite, "with capping rewrite");
		else if(REWRITE == NO_REWRITE)
			strcpy(is_rewrite, "without rewrite");
		
		int dedup_result, tmplen;
		dedup_result = open(output_path, O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
		char tmp[256];
		tmplen = sprintf(tmp, "======== %s %s ========\n", path, is_rewrite);
		write(dedup_result, tmp, tmplen);
		tmplen = sprintf(tmp, "total size is %llu B\n", c->jcr->old_size);
		write(dedup_result, tmp, tmplen);
		//tmplen = sprintf(tmp, "total chunk size is %lluB\n", c->jcr->total_chunk_size);
		//write(dedup_result, tmp, tmplen);	
		tmplen = sprintf(tmp, "dedup size is %llu B\n", c->jcr->total_dedup_size);
		write(dedup_result, tmp, tmplen);
		tmplen = sprintf(tmp, "file dedup size is %llu B\n", c->jcr->file_dedup_size);
		write(dedup_result, tmp, tmplen);
		tmplen = sprintf(tmp, "chunk dedup size is %llu B\n", c->jcr->chunk_dedup_size);
		write(dedup_result, tmp, tmplen);
		float total_size, dedup_size;
		total_size = c->jcr->old_size;
		dedup_size = c->jcr->total_dedup_size;
		float dedup_rate = dedup_size/total_size;
		tmplen = sprintf(tmp, "dedup rate is %f\n", dedup_rate);
		write(dedup_result, tmp, tmplen);
		tmplen = sprintf(tmp, "total time is %.3fs\n\n", c->jcr->total_time);
		write(dedup_result, tmp, tmplen);
		//tmplen = sprintf(tmp, "in traditional backup system, the number of containers be read is %f\n\n", pipeline_traditional_restore_cache->miss_count);
		//write(dedup_result, tmp, tmplen);
		
		close(dedup_result);
		
	}


	
	if(REWRITE == CFL_REWRITE)
		cfl_destory();
	else if (REWRITE == PERFECT_REWRITE)
		perfect_rewrite_destory();
	else if (REWRITE == CAPPING_REWRITE)
		capping_destory();
	
	//container_cache_simulator_free(pipeline_traditional_restore_cache);
    /* write all bin to disk and update the primary index */ 
    free_cache_bin();
	store_destory();

    /* write primary index and free bin */ 
    ExtremeBinningDestroy();
	double size;
	size = ((double)c->jcr->old_size)/(1024*1024);
	printf("%s, %d, backup time is %.3fs\n", __FILE__,__LINE__, c->jcr->total_time);
	printf("%s, %d, size is %f MB, time is %.3fs, read_speed is %f MB/s\n", __FILE__,__LINE__, size, read_time, size/read_time);
	
	jcr_free(c->jcr);
	return true;

}


