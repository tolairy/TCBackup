#include "../global.h"

Queue *dedup_buf = NULL;
htable *segments;
float segment_usage_threshold;
double rewrite_write_time = 0;
double open_time = 0;
double reference_time = 0;
double lookup_time = 0;
double loadbin_time = 0;
double unlock_time = 0;
bool perfect_rewrite_init()
{
	dedup_buf = queue_new();
	SegUsage segment;
	segments = new htable((char *)&segment.next - (char *)&segment, CHUNK_ADDRESS_LENGTH, 200);
	return true;
}


bool perfect_rewrite_process(FileInfo *fileinfo, Bin *bin)
{
	FingerChunk *fc;
	ChunkMeta *cmeta = NULL;
	SegUsage *segment;

	//printf("%s, %d, file name %s, chunk num %d\n", __FILE__, __LINE__, fileinfo->file_path, fileinfo->chunknum);
	
	for (fc = fileinfo->first; fc != NULL; fc = fc->next) {
		int i;
		//printf("%s, %d, chunk %d\n", __FILE__, __LINE__, chunk_count);
		//for (i = 0; i < FINGER_LENGTH; i++) {
		//	printf("%s, %d, chunk_hash[%d] is %c\n", __FILE__, __LINE__, i, (char *)fc->chunk_hash + i);

		//}
		
		//printf("%s,%d \n", __FILE__, __LINE__);
		//cmeta = (ChunkMeta *)bin->fingers->lookup((unsigned char *)&fc->chunk_hash);
		cmeta = (ChunkMeta *)bin->fingers->lookup((unsigned char *)fc->chunk_hash);
		//printf("%s,%d \n", __FILE__, __LINE__);
		pthread_mutex_lock(&(fc->mutex));

		if (cmeta == NULL) {
			fc->is_new = NEW_CHUNK;
		}else {
			fc->is_new = DEDUP_CHUNK;
			memcpy(fc->chunkaddress, cmeta->address, CHUNK_ADDRESS_LENGTH);
			segment = (SegUsage *)segments->lookup((unsigned char *)fc->chunkaddress);
			if (segment != NULL) {
				segment->length += fc->chunklen;
			}else {
				SegUsage *tmp = (SegUsage *)malloc(sizeof(SegUsage));
				memcpy(tmp->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
				tmp->length = fc->chunklen;
				segments->insert((unsigned char *)tmp->address, (void *)tmp);

			}

		}
		if(fileinfo->is_new == DEDUP_FILE && fc->is_new == NEW_CHUNK) {
			int err_log;
			char tmp[512];
			int tmplen;
			
			err_log = open("test/err_log", O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
			//tmplen = sprintf(tmp, "file %s is dedup file, but chunk is new\n", fileinfo->file_path);
			write(err_log, tmp, tmplen);
			close(err_log);
			err_msg1("chunk dedup error");
			
			return false;

		}

	}

	if(fileinfo->chunknum != STREAM_END){
		queue_push(dedup_buf, (void *)fileinfo);

	}else {
		
		int32_t file_count;
		FileInfo *reload_fileinfo;
		FingerChunk *fc;
		Bin *reload_bin;
		int fd = -1;
		int err;
		
		while((reload_fileinfo = (FileInfo *)queue_pop(dedup_buf)) != NULL) {
			TIMER_DECLARE(start4,end4);
			TIMER_START(start4);
			reload_bin = LoadBin(reload_fileinfo);
			TIMER_END(end4);
			TIMER_DIFF(loadbin_time,start4,end4);
				
			//printf("%s, %d file name is %s, chunk num is %d\n", __FILE__, __LINE__, reload_fileinfo->file_path, reload_fileinfo->chunknum);
			TIMER_DECLARE(start1,end1);
			TIMER_START(start1);
			if (!SIMULATE) {
				fd = open(reload_fileinfo->file_path, O_RDWR);
					if (fd <= 0) {
						err_msg1("open file failed");
						return false;
					}
			}
			TIMER_END(end1);
			TIMER_DIFF(open_time,start1,end1);
			
			int i = 0;
			for (fc = reload_fileinfo->first; fc != NULL && i != reload_fileinfo->chunknum; fc = fc->next) {
				
				i++;
				if (fc->is_new == NEW_CHUNK) {
					//printf("%s, %d, new chunk\n", __FILE__, __LINE__);

					TIMER_DECLARE(start3,end3);
					TIMER_START(start3);
					cmeta = (ChunkMeta *)reload_bin->fingers->lookup((unsigned char *)fc->chunk_hash);
					TIMER_END(end3);
					TIMER_DIFF(lookup_time,start3,end3);
					
					if (cmeta != NULL){
						fc->is_new = DEDUP_CHUNK;
						memcpy(fc->chunkaddress, cmeta->address, CHUNK_ADDRESS_LENGTH);
					}else if (cmeta == NULL) {
						//printf("%s, %d, \n", __FILE__, __LINE__);

						TIMER_DECLARE(start,end);
						TIMER_START(start);
						
						err = write_chunk_to_cloud(fd, fc->offset, fc);
						if(err == false) {
							err_msg1("write new chunk to cloud error");
							return false;
						}
						TIMER_END(end);
						TIMER_DIFF(rewrite_write_time,start,end);
						
						cmeta = (ChunkMeta*)malloc(sizeof(ChunkMeta));
						memset(cmeta, 0, sizeof(ChunkMeta));
						memcpy(cmeta->finger, fc->chunk_hash, FINGER_LENGTH);
						memcpy(cmeta->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
						reload_bin->fingers->insert((unsigned char*)cmeta->finger, cmeta);	

					}

				}else if (fc->is_new == DEDUP_CHUNK){
				
					//printf("%s, %d new file\n", __FILE__, __LINE__);
					segment = (SegUsage *)segments->lookup((unsigned char *)fc->chunkaddress);
					if (segment == NULL) {
							err_msg1("search segment error!");
							return false;
					}
					
					TIMER_DECLARE(start2,end2);
					TIMER_START(start2);
					if(((float)segment->length)/DEFAULT_CONTAINER_SIZE < segment_usage_threshold) {
					TIMER_END(end2);
					TIMER_DIFF(reference_time,start2,end2);
					
							TIMER_DECLARE(start,end);
							TIMER_START(start);
							
							err = write_chunk_to_cloud(fd, fc->offset, fc);
							if(err == false) {
								err_msg1("write new chunk to cloud error");
								return false;
							}

							TIMER_END(end);
							TIMER_DIFF(rewrite_write_time,start,end);
							
							fc->is_new = NEW_CHUNK;
							cmeta = (ChunkMeta *)reload_bin->fingers->lookup((unsigned char *)fc->chunk_hash);
							if (cmeta == NULL) {
								err_msg1("lookup cmeta error");
								return false;
							}
							memcpy(cmeta->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
					}

				}

				TIMER_DECLARE(start5,end5);
				TIMER_START(start5);
				pthread_mutex_unlock(&(fc->mutex));
				TIMER_END(end5);
				TIMER_DIFF(unlock_time,start5,end5);

			}
			if (!SIMULATE)
				close(fd);
		}

	}
	return true;
}

bool perfect_rewrite_destory()
{
	queue_free(dedup_buf);
	
	if (0) {
		int log_fd;
		
		SegUsage *segment = NULL;
		char result[32];
		char tmp[256];
		char tmp_path[MAX_PATH];
		int tmplen;
		int64_t before_segment_length = 0;
		int64_t before_data_length = 0;
		int64_t ref_segment_length = 0;
		int64_t ref_length = 0;
		struct stat stats;
		
		log_fd = open("test/backup", O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
		
		//int segment_log_fd = open("test/segment_ref_log", O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
		//char segment_tmp[256];
		
		int segment_count = 0;
		int rewrite_segment_count = 0;
   		void *li = segments->first();
  		while(li){
			
			if (li != NULL) {
				segment_count++;
				segment = (SegUsage *)li;

				//tmplen = sprintf(segment_tmp, "%s, data length %d\n", segment->address, segment->length);
				//write(segment_log_fd, segment_tmp, tmplen);

			//	int i;
				//for (i = 0; i< CHUNK_ADDRESS_LENGTH; i++) {
				//	tmplen = sprintf(segment_tmp, "%d/", (segment->address)[i]);
			//		write(segment_log_fd, segment_tmp, tmplen);
			//	}
		
				//tmplen = sprintf(segment_tmp, "\n");
				//write(segment_log_fd, segment_tmp, tmplen);
			
				
				sprintf(tmp_path, "%s%s", TmpPath, segment->address);
				stat(tmp_path, &stats);
				before_segment_length += stats.st_size;
				before_data_length += segment->length;
				if(((float)segment->length)/DEFAULT_CONTAINER_SIZE < segment_usage_threshold) {
					rewrite_segment_count += 1;
					
					//tmplen = sprintf(segment_tmp, "rewrite\n");
					//write(segment_log_fd, segment_tmp, tmplen);

				}
				else {
					
					ref_length += segment->length;
					ref_segment_length += stats.st_size;
				}
							
				
			}
			li = segments->next();
		
   		}
		
		tmplen = sprintf(tmp, "before rewrite, refer segment count is %d\n", segment_count);
		write(log_fd, tmp, tmplen);
		tmplen = sprintf(tmp, "before rewrite, total refer segment length is %llu\n", before_segment_length);
		write(log_fd, tmp, tmplen);
		tmplen = sprintf(tmp, "before rewrite, total refer data length is %llu\n", before_data_length);
		write(log_fd, tmp, tmplen);
		tmplen = sprintf(tmp, "after rewrite, refer segment count is %d\n", segment_count - rewrite_segment_count);
		write(log_fd, tmp, tmplen);
		tmplen = sprintf(tmp, "after rewrite, total refer segment length is %llu\n", ref_segment_length);
		write(log_fd, tmp, tmplen);
		tmplen = sprintf(tmp, "after rewirte, total refer data total length is %llu\n\n\n\n", ref_length);
		write(log_fd, tmp, tmplen);
		
		close(log_fd);
		//close(segment_log_fd);
	}
	segments->destroy();
	return true;
}
