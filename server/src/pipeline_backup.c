#include  "../global.h"



SyncQueue *file_dedup_queue;



void *file_dedup_module(void *arg)
{
	char buf[256] = {0};
	int len;
	Recipe *rp;
	DedupClient *c = (DedupClient *)arg;
	
	//printf("%s,%d\n",__FILE__, __LINE__);

	while (bnet_recv(c->client_fd, buf, &len) != ERROR) {

		rp = recipe_new();
		
		if (len == STREAM_END) {
		//	printf("%s,%d\n",__FILE__, __LINE__);
			break;
		}
		
		c->jcr->file_count++;
		memcpy(rp->file_hash, buf, FINGER_LENGTH);
                /* check the file is new or old */
        file_dedup_mark(c, rp);
                /* if the file is old, send the chunk info to client */
        if (rp->is_new == DEDUP_FILE) {
			send_chunk_info(c, rp);
 		}
		
		//printf("%s,%d\n",__FILE__, __LINE__);
		sync_queue_push(file_dedup_queue, rp);

	}
	//printf("%s,%d\n",__FILE__, __LINE__);
	sync_queue_push(file_dedup_queue, rp);
	return NULL;
}

void *update_recipe_module(void *arg)
{
	char msg[FILE_NAME_LEN + 128] = {0};
	char *buf = NULL, *p;
	int32_t last_chunknum = -1, chunknum;
	Recipe *rp;
	int err , len, result;
	DedupClient *c = (DedupClient *)arg;
	FingerChunk *fc;
	Fingerprint chunk_hash;
	Chunkaddress chunk_address;
	FileRecord *file;
	VersionFileRecord *versionfile;
	IndexItem *index_item;

	file = (FileRecord *)zmalloc(sizeof(FileRecord));
	versionfile = (VersionFileRecord *)zmalloc(sizeof(VersionFileRecord));

	double write_recipe_time = 0, write_filetable_time = 0, write_versionfiletable_time = 0;
	TIMER_DECLARE(start,end);
	TIMER_DECLARE(start1,end1);
	TIMER_DECLARE(start2,end2);
	
	while (rp = (Recipe *)sync_queue_pop(file_dedup_queue)) {

		if (rp->is_new == -1) {
			//printf("%s,%d\n",__FILE__, __LINE__);
			recipe_free(rp);
			break;
		}

		//printf("%s,%d\n",__FILE__, __LINE__);
		err = true;
		result = true;
		
		memset(msg, 0, sizeof(msg));
        bnet_recv(recipe_fd, msg, &len);
        if(sscanf(msg, update_attr, &chunknum, rp->filename) != 2) {
				err_msg1("wrong updateattr command!");
				result = false;
		}
		
		if (rp->is_new == NEW_FILE) {
			
			  /* if the file is new, recive the recipe and write file recipe to file */
			
			

			//printf("%s,%d,%s\n",__FILE__, __LINE__, msg);
			
              
            if (chunknum > last_chunknum) { 
			
				buf = (char *)zrealloc(buf, FINGER_LENGTH + chunknum * (FINGER_LENGTH + CHUNK_ADDRESS_LENGTH)+1);
				last_chunknum = chunknum;
				
            }

			
			
			err = bnet_recv(recipe_fd, buf ,&len);
			if(err == false){
		   		err_msg1("net_recv failed!");
				result = false;
	    	}
			
			p = buf;
			if (memcmp(p, rp->file_hash, FINGER_LENGTH) != 0) {
				err_msg1("wrong file hash!");
				result = false;
			}
			p += FINGER_LENGTH;

			int i;
			
			for (i = 0; i < chunknum; i++) {
				//printf("%s,%d,%d\n",__FILE__, __LINE__, i);
				memcpy(chunk_hash, p, FINGER_LENGTH);
				p += FINGER_LENGTH;
				
        		memcpy(chunk_address, p, CHUNK_ADDRESS_LENGTH);
				p += CHUNK_ADDRESS_LENGTH;
				fc = fingerchunk_new(chunk_hash, chunk_address);
				recipe_append_fingerchunk(rp, fc);

			}
	   
        	index_item = (IndexItem *)malloc(sizeof(IndexItem));
			//printf("%s,%d,%d\n",__FILE__, __LINE__, sizeof(index_item));
        	memset(index_item, 0, sizeof(IndexItem));
       		memcpy(index_item->hash, rp->file_hash, FINGER_LENGTH);
			TIMER_START(start);
        	err = write_recipe_to_vol(&index_item->offset, rp);
			TIMER_END(end);
			TIMER_DIFF(write_recipe_time,start,end);
        	if(err == false) {
             	err_msg1("read file recipe error!");
				result = false;
	             
	        }
        	index_insert(index_item);
		}

		//printf("%s,%d\n",__FILE__, __LINE__);
                /* update database */
		TIMER_START(start1);
		memset(file, 0, sizeof(FileRecord));
		file->JobId = c->jcr->job_id;
		memcpy(file->FilePath, rp->filename, FILE_NAME_LEN);
		memcpy(file->HashCode, rp->file_hash, FINGER_LENGTH);
		file->Size = 0;
		file->VersionCount = 1;
		err = insert_record_into_file(c->db, file);
		if(err == false){
			err_msg1("insert_record_into_file failed\n");
			result = false;
		       	
		}
		TIMER_END(end1);
		TIMER_DIFF(write_filetable_time,start1,end1);

		//printf("%s,%d\n",__FILE__, __LINE__);

		TIMER_START(start2);
				
		memset(versionfile, 0, sizeof(VersionFileRecord));
		versionfile->JobId = c->jcr->job_id;
		versionfile->FileId = file->FileId;
		memcpy(versionfile->FilePath, rp->filename, strlen(rp->filename));
		err = insert_record_into_versionfile(c->db, versionfile);
		if(err == false){
			err_msg1("insert_record_into_versionfile failed\n");
			result = false;
		      
		}
		
		TIMER_END(end2);
		TIMER_DIFF(write_versionfiletable_time,start2,end2);
	
		
			//printf("%s,%d\n",__FILE__, __LINE__);
		if(result == false) {
			memset(msg, 0, sizeof(msg));
			memcpy(msg, updateattr_fail, strlen(updateattr_fail));
			printf("%s,%d,%s:updateattr_fail\n",__FILE__,__LINE__, rp->filename);
		}else {
			memset(msg, 0, sizeof(msg));
			memcpy(msg, updateattr_ok, strlen(updateattr_ok));
			printf("%s,%d:%s,updateattr_ok\n",__FILE__,__LINE__, rp->filename);
		}
		err = bnet_send(recipe_fd, msg, strlen(msg));
		if(err == false) {
			err_msg1("bnet_send failed!");
		}
		
		//printf("%s,%d\n",__FILE__, __LINE__);
		recipe_free(rp);


	}
	
	//printf("%s,%d\n",__FILE__, __LINE__);
	printf("%s, %d, write recipe time is %.3fs, filetable %.3fs, versionfiletable %.3fs\n", __FILE__, __LINE__, write_recipe_time, write_filetable_time, write_versionfiletable_time);
	zfree(versionfile);
	zfree(file);
	zfree(buf);
	
	return NULL;
}

int pipeline_backup(DedupClient *c, char *msg)
{
	JCR *jcr;
	ObjectRecord *object;
	JobRecord *job;
	time_t current_time;
    struct tm *current_tm;
	char s_time[128] = {0};
	int err, result;
	
	pthread_t file_dedup_thread, update_recipe_thread;
	
	
	while (recipe_fd < 0) 
		sleep(1);

	
	jcr = jcr_new(); 
	c->jcr = jcr;
    
    if (sscanf(msg, backup_cmd, jcr->backup_path) != 1) {
        err_msg1("wrong backup command!");
        return false;
    }

    time(&current_time);
    current_tm = localtime(&current_time);
    strftime(s_time, sizeof(s_time), "%Y%m%d_%H_%M_%S", current_tm);
	
   // printf("%s,%d:%s\n", __FILE__,__LINE__,jcr->backup_path);
    object = (ObjectRecord *)zmalloc(sizeof(ObjectRecord));
    memset(object, 0, sizeof(object));
    object->UserId = c->user_id;
    sprintf(object->ObjectName, "%c%s%s", 'f', c->username, s_time);
	strcpy(object->fileset, jcr->backup_path);
    err = insert_record_into_object(c->db, object);
    if(err == false){
        err_msg1("insert_record_into_object failed\n");
        result = false;
    }else {
         jcr->object_id = object->ObjectId;
    }
	
    job = (JobRecord *)zmalloc(sizeof(JobRecord));
    memset(job, 0, sizeof(job));
    job->UserId = c->user_id;
    job->ObjectId = object->ObjectId;
    job->JobType = 'f';
    job->FileCount = 0;
    job->DataSize = 0;
    err = insert_record_into_job(c->db, job);
    if(err == false){
        err_msg1("insert_record_into_job failed\n");
        result = false;
    }else {
        jcr->job_id = job->JobId;
    }

	file_dedup_queue = sync_queue_new();
	
	pthread_create(&file_dedup_thread, NULL, file_dedup_module, c);
	pthread_create(&update_recipe_thread, NULL, update_recipe_module, c);



	pthread_join(file_dedup_thread, NULL);
	pthread_join(update_recipe_thread, NULL);

	sync_queue_free(file_dedup_queue);
	
	printf("===============job %d complete==========\n", jcr->job_id);
	jcr_free(jcr);
	zfree(job);
	zfree(object);

}

