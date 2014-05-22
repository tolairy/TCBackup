#include "../global.h"

typedef struct restore_seg{
	Chunkaddress segment_id;
	hlink next;
}RestoreSeg;


SyncQueue *overhead_chunk_dedup_queue = NULL;


extern float segment_usage_threshold;

extern double read_time;

htable *restore_segments_overhead;
double rewrite_time = 0;

double dedup_time = 0;
double overhead_read_time;
double overhead_chunk_time;
double overhead_hash_time;

double norewrite_store_chunk_time = 0;
double perfect_stastics_time = 0;
extern double rewrite_write_time;
extern double upload_time_first;
extern double upload_time_second;
extern double open_time;
extern double reference_time;
extern double lookup_time;
extern double loadbin_time;
extern double unlock_time;

LRUCache *overhead_traditional_container_cache = NULL;
bool get_restore_info_init_overhead(){
	RestoreSeg rs;
	restore_segments_overhead = new htable((char*)&rs.next - (char*)&rs, sizeof(Chunkaddress), 100);

}
bool get_restore_info_overhead(FingerChunk *fc){
	RestoreSeg *rs;
	rs = (RestoreSeg *)restore_segments_overhead->lookup((unsigned char *)fc->chunkaddress);
			
	if (rs == NULL) {
		rs = (RestoreSeg *)malloc(sizeof(RestoreSeg));
		memcpy(rs->segment_id, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
		restore_segments_overhead->insert((unsigned char *)rs->segment_id, rs);
				
	}	

}
bool get_restore_info_final_overhead(char *path){

	int64_t size = 0;
	int count = 0;
	RestoreSeg *rs;
	
	rs = (RestoreSeg *)restore_segments_overhead->first();
	printf("%s, %d, %d\n", __FILE__, __LINE__, restore_segments_overhead->size());
	while (rs) {
		struct stat buf;
		char tmp[MAX_PATH];
		sprintf(tmp, "%s%s", TmpPath, rs->segment_id);
		stat(tmp, &buf);
		size += buf.st_size;
		count += 1;
		//printf("%s, %d, %s, %llu\n", __FILE__, __LINE__, rs->segment_id, container_size);
		rs = (RestoreSeg *)restore_segments_overhead->next();
	}
	restore_segments_overhead->destroy();

	if (OUTPUT_RESULT) {
		int dedup_result, tmplen;
		dedup_result = open(path, O_RDWR|O_CREAT|O_APPEND, 0777);
		char tmp[256]; 
		tmplen = sprintf(tmp, "download count is %d\n", count);
		write(dedup_result, tmp, tmplen);
		tmplen = sprintf(tmp, "download size is %llu B\n\n\n\n", size);
		write(dedup_result, tmp, tmplen);
		close(dedup_result);
	}


}
bool store_chunk_overhead(FingerChunk *fc, unsigned char *data, Bin *bin){
	ChunkMeta *cmeta;
	int err;
	
	Chunk *chunk = (Chunk *)malloc(sizeof(Chunk));
	memcpy(chunk->hash, fc->chunk_hash, FINGER_LENGTH);
	chunk->length = fc->chunklen;
    chunk->data = (char *)malloc(fc->chunklen + 1);
	memcpy(chunk->data, data, fc->chunklen);
	
	if((cmeta =  (ChunkMeta*)bin->fingers->lookup((unsigned char *)&fc->chunk_hash)) != NULL) {
		fc->is_new = DEDUP_CHUNK;
		memcpy(fc->chunkaddress, cmeta->address, sizeof(cmeta->address));
	} else {
		fc->is_new = NEW_CHUNK;
		err = store_chunk(chunk);
		if(err == false) {
			err_msg1("write new chunk to cloud error");
		}
		memcpy(fc->chunkaddress, chunk->address, CHUNK_ADDRESS_LENGTH);
		
		cmeta = (ChunkMeta*)malloc(sizeof(ChunkMeta));
		memset(cmeta, 0, sizeof(cmeta));
		memcpy(cmeta->finger, fc->chunk_hash, FINGER_LENGTH);
		memcpy(cmeta->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
		bin->fingers->insert((unsigned char*)&cmeta->finger, cmeta);
	}
						 
	free(chunk->data);
	free(chunk);
	return true;


}
int chunk_overhead(char *path){

	int subFile;
    int32_t srclen=0, left_bytes = 0;
    int32_t size=0,len=0; 
    int32_t n = MAX_CHUNK_SIZE;
	int64_t offset = 0;
	FileInfo *fileinfo;
	int err;
	Bin *bin;
	
	unsigned char *p;
	FingerChunk *fc;
    unsigned char *src = (unsigned char *)malloc(MAX_CHUNK_SIZE*2);	
	//SHA1Context ctx;
	
	//SHA1Init(&ctx);
    chunk_alg_init();
	if(src == NULL) {
        err_msg1("Memory allocation failed");
		return FAILURE;
    }

	if ((subFile = open(path, O_RDONLY)) < 0) {
	    printf("%s,%d: open file %s error\n",__FILE__, __LINE__, fileinfo->file_path);
		free((char*)src);
		return FAILURE;
	}

	while(1) 
	{
		TIMER_DECLARE(start1,end1);
		TIMER_START(start1);
	 	if((srclen = readn(subFile, (char *)(src+left_bytes), MAX_CHUNK_SIZE)) <= 0)
		    break;
		TIMER_END(end1);
		TIMER_DIFF(overhead_read_time,start1,end1);
		if (left_bytes > 0){ 
			srclen += left_bytes;
			left_bytes = 0;
		} 

		if(srclen < MIN_CHUNK_SIZE)
		 	break;
		
		p = src;
		len = 0;
		while (len < srclen) 
		{
          	n = srclen -len;
			
			TIMER_DECLARE(start2,end2);
			TIMER_START(start2);
			size = chunk_data(p, n);
			TIMER_END(end2);
			TIMER_DIFF(overhead_chunk_time,start2,end2);
			
			if(n==size && n < MAX_CHUNK_SIZE)
			{ 	
          		memmove(src, src+len, n );
          		left_bytes = n;
                break;
			}  
      		fileinfo = file_new();
			strcpy(fileinfo->file_path, path);
			fileinfo->is_new = NEW_FILE;	
			fileinfo->file_size = size;
			
			fc = fingerchunk_new();
			
			TIMER_DECLARE(start3,end3);
			TIMER_START(start3);
			chunk_finger(p, size, fc->chunk_hash);
			TIMER_END(end3);
			TIMER_DIFF(overhead_hash_time,start3,end3);
			
			//SHA1Update(&ctx, p, size);
			fc->chunklen = size;
			fc->offset = offset;
			file_append_fingerchunk(fileinfo, fc);
			bin = LoadBin(fileinfo);

			if (REWRITE == PERFECT_REWRITE) {
				TIMER_DECLARE(start,end);
				TIMER_START(start);
				err = perfect_rewrite_process(fileinfo, bin);
    				if(err == false) {
        			err_msg1("perfect rewrite error!");
    			}
				TIMER_END(end);
				TIMER_DIFF(perfect_stastics_time,start,end);
				
			}else if(REWRITE == CAPPING_REWRITE) {
				TIMER_DECLARE(start,end);
				TIMER_START(start);
				err = capping_process(fileinfo, bin);
    				if(err == false) {
        			err_msg1("capping rewrite error!");
    			}
				TIMER_END(end);
				TIMER_DIFF(perfect_stastics_time,start,end);

			}else if(REWRITE == CFL_REWRITE) {
				TIMER_DECLARE(start,end);
				TIMER_START(start);
				err = cfl_process(fileinfo, bin);
						if(err == false) {
						err_msg1("capping rewrite error!");
				}
				TIMER_END(end);
				TIMER_DIFF(perfect_stastics_time,start,end);

			}else if (REWRITE == NO_REWRITE){
				TIMER_DECLARE(start,end);
				TIMER_START(start);
				store_chunk_overhead(fc, p, bin);
				TIMER_END(end);
				TIMER_DIFF(norewrite_store_chunk_time,start,end);
			}
			sync_queue_push(overhead_chunk_dedup_queue, fileinfo);
			
			offset += size;
			p = p + size;
			len += size;
		}
    }
	
	/******more******/
	len = 0;
	if(srclen > 0)
	    len=srclen;
	else if(left_bytes > 0)
	 	len=left_bytes;
	if(len > 0){
	 	p= src;
		
		fileinfo = file_new();
		strcpy(fileinfo->file_path, path);
		fileinfo->is_new = NEW_FILE;
		fileinfo->file_size = len;
		
	 	fc = fingerchunk_new();
		
		TIMER_DECLARE(start3,end3);
		TIMER_START(start3);
		chunk_finger(p, size, fc->chunk_hash);
		TIMER_END(end3);
		TIMER_DIFF(overhead_hash_time,start3,end3);
			
		//SHA1Update(&ctx, p, len);
		fc->chunklen = len;
		fc->offset = offset;
		file_append_fingerchunk(fileinfo, fc);
		
		calculate_rep_finger(fileinfo);
    	bin = LoadBin(fileinfo);
    	if (REWRITE == PERFECT_REWRITE) {
			TIMER_DECLARE(start,end);
			TIMER_START(start);
			err = perfect_rewrite_process(fileinfo, bin);
    			if(err == false) {
        		err_msg1("perfect rewrite error!");
    		}
			TIMER_END(end);
			TIMER_DIFF(perfect_stastics_time,start,end);
				
		}else if (REWRITE == CAPPING_REWRITE) {
			TIMER_DECLARE(start,end);
			TIMER_START(start);
			err = capping_process(fileinfo, bin);
    			if(err == false) {
        		err_msg1("capping rewrite error!");
    		}
			TIMER_END(end);
			TIMER_DIFF(perfect_stastics_time,start,end);

		}else if (REWRITE == CFL_REWRITE) {

			TIMER_DECLARE(start,end);
			TIMER_START(start);
			err = cfl_process(fileinfo, bin);
					if(err == false) {
					err_msg1("capping rewrite error!");
			}
			TIMER_END(end);
			TIMER_DIFF(perfect_stastics_time,start,end);

		}else if (REWRITE == NO_REWRITE){
			TIMER_DECLARE(start,end);
			TIMER_START(start);
			store_chunk_overhead(fc, p, bin);
			TIMER_END(end);
			TIMER_DIFF(norewrite_store_chunk_time,start,end);
		}
		sync_queue_push(overhead_chunk_dedup_queue, fileinfo);
		
	 }	
	//SHA1Final(&ctx, fileinfo->file_hash);
	 close(subFile);
   	 free(src);
   	 return SUCCESS;


}
int  walk_dir_overhead(char *path) 
{
	struct stat state;
	if (stat(path, &state) != 0) {
		err_msg1("file does not exist! ignored!");
		return 0;
	}
	if (S_ISDIR(state.st_mode)) {
		DIR *dir = opendir(path);
		struct dirent *entry;
		char newpath[512];
		memset(newpath,0,512);
		if (strcmp(path + strlen(path) - 1, "/")) {
			strcat(path, "/");
		}

		while ((entry = readdir(dir)) != 0) {
			/*ignore . and ..*/
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))//(entry->d_name[0]=='.')
				continue;
			strcpy(newpath, path);
			strcat(newpath, entry->d_name);
			if (walk_dir_overhead(newpath) != 0) {
				return -1;
			}
		}
		//printf("*** out %s direcotry ***\n", path);
		closedir(dir);
	} 
	else if (S_ISREG(state.st_mode)) {
		chunk_overhead(path);

	} else {
		err_msg1("illegal file type! ignored!");
		return 0;
	}
	return 0;
}

void * chunk_dedup_overhead(void *arg)
{

	int len;
	Client *c = (Client *)arg;
	char path[FILE_NAME_LEN];
	struct stat state;
	FileInfo *fileinfo;
	
	if (stat(c->jcr->backup_path, &state) != 0) {
		puts("backup path does not exist!");
		return FAILURE;
	}

	memcpy(path, c->jcr->backup_path, strlen(c->jcr->backup_path));

	//printf("%s,%d\n",__FILE__, __LINE__);
	TIMER_DECLARE(start,end);
	TIMER_START(start);
	
	if(S_ISREG(state.st_mode)) {
        char *p = c->jcr->backup_path + strlen(c->jcr->backup_path) - 1;
        while (*p != '/')
            --p;
        *(p + 1) = 0;
        chunk_overhead(path);
    }else {
        len = strlen(c->jcr->backup_path);
        if(path[len-1] != '/')
            path[len] = '/';
        walk_dir_overhead(path);
		
    }
	TIMER_END(end);
	TIMER_DIFF(dedup_time,start,end);

	TIMER_DECLARE(start1,end1);
	TIMER_START(start1);
	
	fileinfo = file_new();
	fileinfo->chunknum = STREAM_END;
	if(REWRITE == CFL_REWRITE)
		cfl_process(fileinfo, NULL);
	else if (REWRITE == PERFECT_REWRITE)
		perfect_rewrite_process(fileinfo,NULL);
	else if (REWRITE == CAPPING_REWRITE)
		capping_process(fileinfo, NULL);

	TIMER_END(end1);
	TIMER_DIFF(rewrite_time,start1,end1);
	
	sync_queue_push(overhead_chunk_dedup_queue, fileinfo);
	return NULL;
	
}



void *get_dedup_result_module_overhead(void *arg)
{
	int err, last_chunknum = -1;
	Client *c = (Client *)arg;
	FileInfo *fileinfo;
	FingerChunk *fc;
	
	while (fileinfo = (FileInfo *)sync_queue_pop(overhead_chunk_dedup_queue)) {
		if (fileinfo->chunknum == STREAM_END) {
			file_free(fileinfo);
			break;
		}
			

		
	//	printf("%s,%d\n",__FILE__, __LINE__);
		
		c->jcr->old_size += fileinfo->file_size;
		for (fc = fileinfo->first; fc != NULL; fc = fc->next) {
		//	printf("%s, %d\n", __FILE__, __LINE__);
			pthread_mutex_lock(&fc->mutex);
		//	printf("%s, %d\n", __FILE__, __LINE__);
			if (fc->is_new == DEDUP_CHUNK) {
				c->jcr->total_dedup_size += fc->chunklen;
			}
			get_restore_info_overhead(fc);
			
			if (!container_cache_simulator_look(overhead_traditional_container_cache, fc->chunkaddress)) {
				container_cache_simulator_insert(overhead_traditional_container_cache, fc->chunkaddress);
			}
			pthread_mutex_unlock(&fc->mutex);
		}    
		file_free(fileinfo);
	}
	
	return NULL;

}

int backup_overhead(Client *c, char *path, char *output_path) {

	char buf[256]={0};
	int err;
	int len=0;
	
	pthread_t chunk_dedup_thread, get_dedup_result_thread;
	 
	c->jcr = jcr_new();
	strcpy(c->jcr->backup_path, path);
	//printf("jcr path is %s\n", c->jcr->backup_path);

	if (access(c->jcr->backup_path, F_OK) != 0) {
		puts("This path does not exist or can not be read!");
		return FAILURE;
	}

	overhead_traditional_container_cache = container_cache_simulator_new(16);
	get_restore_info_init_overhead();
	ExtremeBinningInit();
	store_init();

	FileInfo *fileinfo = file_new();
	Bin *bin = LoadBin(fileinfo);
	
	if(REWRITE == CFL_REWRITE)
		cfl_init();
	else if (REWRITE == PERFECT_REWRITE)
		perfect_rewrite_init();
	else if(REWRITE == CAPPING_REWRITE)
		capping_init();
	

	if (!(overhead_chunk_dedup_queue = sync_queue_new())) {
		err_msg1("chunk dedup queue init error!");
		return false;
	}
	
	
	TIMER_DECLARE(start,end);
	TIMER_START(start);


    pthread_create(&chunk_dedup_thread, NULL,  chunk_dedup_overhead, c);
    pthread_join(chunk_dedup_thread, NULL);

	
	if(REWRITE == CFL_REWRITE)
		cfl_destory();
	else if (REWRITE == PERFECT_REWRITE)
		perfect_rewrite_destory();
	else if (REWRITE == CAPPING_REWRITE)
		capping_destory();
	
    /* write all bin to disk and update the primary index */ 
   
	
	store_destory();
	
	TIMER_END(end);
	TIMER_DIFF(c->jcr->total_time,start,end);

	pthread_create(&get_dedup_result_thread, NULL, get_dedup_result_module_overhead, c);	
	pthread_join(get_dedup_result_thread, NULL);

	sync_queue_free(overhead_chunk_dedup_queue);
	
	free_cache_bin();
	ExtremeBinningDestroy();

    /* write primary index and free bin */ 
    
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
			tmplen = sprintf(tmp, "dedup time is %.3fs\n", dedup_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "read time is %.3fs\n", overhead_read_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "chunk time is %.3fs\n", overhead_chunk_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "hash time is %.3fs\n", overhead_hash_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "norewrite store chunk time is %.3fs\n", norewrite_store_chunk_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "defragment time is %.3fs\n", perfect_stastics_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "rewrite time is %.3fs\n", rewrite_time);
			write(dedup_result, tmp, tmplen);
			/*tmplen = sprintf(tmp, "loadbin time is %.3fs\n", loadbin_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "open time is %.3fs\n", open_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "lookup time is %.3fs\n", lookup_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "calculate reference time is %.3fs\n", reference_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "rewrite read and write time is %.3fs\n", rewrite_write_time);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "unlock time is %.3fs\n", unlock_time);
			write(dedup_result, tmp, tmplen);*/
			tmplen = sprintf(tmp, "upload time is %.3fs\n", upload_time_first+upload_time_second);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "total time is %.3fs\n\n", c->jcr->total_time+upload_time_second);
			write(dedup_result, tmp, tmplen);
			tmplen = sprintf(tmp, "in traditional backup system, the number of containers be read is %f\n\n", overhead_traditional_container_cache->miss_count);
			write(dedup_result, tmp, tmplen);
			close(dedup_result);
			
			
	}
	container_cache_simulator_free(overhead_traditional_container_cache);
	get_restore_info_final_overhead(output_path); 
	jcr_free(c->jcr);
	return true;

}

