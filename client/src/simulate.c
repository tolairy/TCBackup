#include "../global.h"

typedef struct restore_seg{
	Chunkaddress segment_id;
	hlink next;
}RestoreSeg;


SyncQueue *fileinfo_queue = NULL;
SyncQueue *simulate_chunk_dedup_queue = NULL;


extern float segment_usage_threshold;

extern double read_time;

htable *restore_segments;
LRUCache *simulator_traditional_restore_cache = NULL;


bool get_restore_info_init(){
	RestoreSeg rs;
	restore_segments = new htable((char*)&rs.next - (char*)&rs, sizeof(Chunkaddress), 100);

}

bool get_restore_info(FingerChunk *fc){
	RestoreSeg *rs;
	rs = (RestoreSeg *)restore_segments->lookup((unsigned char *)fc->chunkaddress);
			
	if (rs == NULL) {
		rs = (RestoreSeg *)malloc(sizeof(RestoreSeg));
		memcpy(rs->segment_id, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
		restore_segments->insert((unsigned char *)rs->segment_id, rs);
				
	}	

}

bool output_seg_statistics(int chunknum){
    char *buf = NULL;
    char *tmp = NULL;
	int segment_length = 0;
    RestoreSeg *rs = NULL;
    
    int buf_size = (CHUNK_ADDRESS_LENGTH + 1) * chunknum;

    buf = (char *)malloc(buf_size + 1);
    memset(buf, 0, buf_size + 1);
    tmp = buf;

    rs = (RestoreSeg *)restore_segments->first();
    while(rs) {
		segment_length = get_container_size(rs->segment_id);
       // memcpy(tmp, rs->segment_id, strlen(rs->segment_id));
       // tmp += strlen(rs->segment_id);
       // memcpy(tmp, "/", 1);
		int str_len = sprintf(tmp, "%s,%d/", rs->segment_id, segment_length);
        tmp += str_len;
        rs = (RestoreSeg *)restore_segments->next();
    }
    tmp -= 1;
    memcpy(tmp, "\n", strlen("\n"));
    if (SEG_STATISTICS) {
        int output_fd;
        output_fd = open(static_path, O_RDWR|O_CREAT|O_APPEND, 0777);
        if(output_fd == -1)
            return false;
        write(output_fd, buf, strlen(buf));
        close(output_fd);
    }
    free(buf);
    return true;
}

bool get_restore_info_final(char *path){

	int64_t size = 0;
	int count = 0;
	RestoreSeg *rs;
	
	rs = (RestoreSeg *)restore_segments->first();
	printf("%s, %d, %d\n", __FILE__, __LINE__, restore_segments->size());
	while (rs) {
		int container_size = get_container_size(rs->segment_id);
		size += container_size;
		count += 1;
		printf("%s, %d, %s, %llu\n", __FILE__, __LINE__, rs->segment_id, container_size);
		rs = (RestoreSeg *)restore_segments->next();
	}
	if(SEG_STATISTICS) {
        output_seg_statistics(count);
    }
    
    restore_segments->destroy();

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


void * read_trace_module(void *arg) {
	Client *c = (Client *)arg;
	read_trace(c->jcr->backup_path, fileinfo_queue);
	

}


void * simulate_chunk_dedup_module(void *arg)
{

	char buf[256];
	int err, len;
	char success, newfile;
	Client *c = (Client *)arg;
	FileInfo *fileinfo;
	Bin *bin;
	
	while (fileinfo = (FileInfo *)sync_queue_pop(fileinfo_queue)) {

		if (fileinfo->chunknum == STREAM_END) {
			printf("%s, %d\n", __FILE__, __LINE__);
			/*terminate the cfl process*/
			if(REWRITE == CFL_REWRITE)
				cfl_process(fileinfo, NULL);
			else if (REWRITE == PERFECT_REWRITE)
				perfect_rewrite_process(fileinfo,NULL);
			else if (REWRITE == CAPPING_REWRITE)
				capping_process(fileinfo, NULL);
			
			sync_queue_push(simulate_chunk_dedup_queue, fileinfo);
			break;
		}
			

		printf("%s, %d, simulate file: %s\n", __FILE__, __LINE__, fileinfo->file_path);
		FingerChunk *fchunk = fileinfo->first;
    	
		calculate_rep_finger(fileinfo);
    	bin = LoadBin(fileinfo);
    	err = mark_deplicate_chunk(fileinfo, bin);
    	if(err == false) {
        	err_msg1("mark the duplicate chunk error!");
        	return NULL;
    	} 
		printf("%s, %d\n", __FILE__, __LINE__);
	
		sync_queue_push(simulate_chunk_dedup_queue, fileinfo);
		printf("%s, %d\n", __FILE__, __LINE__);
	}
	
	return NULL;
}



void *get_dedup_result_module(void *arg)
{
	int err, last_chunknum = -1;
	Client *c = (Client *)arg;
	FileInfo *fileinfo;
	FingerChunk *fc;
	
	while (fileinfo = (FileInfo *)sync_queue_pop(simulate_chunk_dedup_queue)) {
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
			get_restore_info(fc);
			
			if (!container_cache_simulator_look(simulator_traditional_restore_cache, fc->chunkaddress)) {
				container_cache_simulator_insert(simulator_traditional_restore_cache, fc->chunkaddress);
			}
			
			pthread_mutex_unlock(&fc->mutex);
		}    
		file_free(fileinfo);
	}

	return NULL;

}

int simulata_backup(Client *c, char *path, char *output_path) {

	char buf[256]={0};
	int err;
	int len=0;
	
	pthread_t  read_trace_thread, chunk_dedup_thread, get_dedup_result_thread;
	 
	c->jcr = jcr_new();
	strcpy(c->jcr->backup_path, path);
	//printf("jcr path is %s\n", c->jcr->backup_path);

	if (access(c->jcr->backup_path, F_OK) != 0) {
		puts("This path does not exist or can not be read!");
		return FAILURE;
	}
	get_restore_info_init();
	ExtremeBinningInit();
	store_init();
	simulator_traditional_restore_cache = container_cache_simulator_new(4);
	
	if(REWRITE == CFL_REWRITE)
		cfl_init();
	else if (REWRITE == PERFECT_REWRITE)
		perfect_rewrite_init();
	else if(REWRITE == CAPPING_REWRITE)
		capping_init();

	if (!(fileinfo_queue = sync_queue_new())) {
			err_msg1("fileinfo queue init error!");
			return false;
		}

	

	if (!(simulate_chunk_dedup_queue = sync_queue_new())) {
		err_msg1("chunk dedup queue init error!");
		return false;
	}
	
	
	TIMER_DECLARE(start,end);
	TIMER_START(start);


	pthread_create(&read_trace_thread, NULL, read_trace_module, c);
    pthread_create(&chunk_dedup_thread, NULL,  simulate_chunk_dedup_module, c);
    pthread_create(&get_dedup_result_thread, NULL, get_dedup_result_module, c);


	 
 	
	//get_socket_default_bufsize(jcr->finger_socket);
	//set_sendbuf_size(jcr->finger_socket,0);
	//set_recvbuf_size(jcr->finger_socket,0);
	
	pthread_join(read_trace_thread, NULL);
	pthread_join(chunk_dedup_thread, NULL);
	pthread_join(get_dedup_result_thread, NULL);


	sync_queue_free(fileinfo_queue);
	sync_queue_free(simulate_chunk_dedup_queue);
	

 
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
		tmplen = sprintf(tmp, "in traditional backup system, the number of containers be read is %f\n\n", simulator_traditional_restore_cache->miss_count);
		write(dedup_result, tmp, tmplen);
		close(dedup_result);

		
		
	}


	
	
	if(REWRITE == CFL_REWRITE)
		cfl_destory();
	else if (REWRITE == PERFECT_REWRITE)
		perfect_rewrite_destory();
	else if (REWRITE == CAPPING_REWRITE)
		capping_destory();
	
    /* write all bin to disk and update the primary index */ 
    free_cache_bin();
	ExtremeBinningDestroy();
	
	store_destory();
	get_restore_info_final(output_path);
	container_cache_simulator_free(simulator_traditional_restore_cache);
    /* write primary index and free bin */ 
    
	double size;
	size = ((double)c->jcr->old_size)/(1024*1024);
	printf("%s, %d, backup time is %.3fs\n", __FILE__,__LINE__, c->jcr->total_time);
	printf("%s, %d, size is %f MB, time is %.3fs, read_speed is %f MB/s\n", __FILE__,__LINE__, size, read_time, size/read_time);
	
	jcr_free(c->jcr);
	return true;

}

