#include "../global.h"

int64_t buffer_capacity;
int max_refer_count;
static int buffer_length;
Queue * chunk_buffer;
htable *segment_buffer;

bool capping_init()
{
	buffer_capacity = buffer_capacity*1024*1024;
	buffer_length = 0;
	chunk_buffer = queue_new();
	RefNode rn;
	segment_buffer = new htable((char*)&rn.next - (char*)&rn, sizeof(Chunkaddress), 50);
	
}
bool capping_destory()
{

	queue_free(chunk_buffer);
	segment_buffer->destroy();

}

bool sort_segments()
{
	RefNode *rn;
	RefNode *head;
	RefNode *p;
	//printf("%s, %d\n", __FILE__, __LINE__);
	rn = (RefNode *)segment_buffer->first();
	head = rn;
	rn->link = NULL;
	rn = (RefNode *)segment_buffer->next();
	
	while (rn) {	
		if(rn->ref_length > head->ref_length) {
			rn->link = head;
			head = rn;
	//	printf("%s, %d\n", __FILE__, __LINE__);		
		}else {
			p = head;
			while (p) {
			//	printf("%s, %d\n", __FILE__, __LINE__);
				if(p->link != NULL) {
					if (rn->ref_length > p->link->ref_length) {
						rn->link = p->link;
						p->link = rn;
						break;
					}else {
						p = p->link;
					}
					
				}else {
					p->link = rn;
					rn->link = NULL;
					break;
				}

			}

		}

		rn = (RefNode *)segment_buffer->next();

	}
		
	int i = 1;
	p = head;
	RefNode *tmp;
	while (p) {
		//printf("%s, %d\n", __FILE__, __LINE__);
		if (i > max_refer_count) {
			tmp = p->link;
			free(segment_buffer->remove((unsigned char *)p->address));
			p = tmp;
		}else {
			p = p->link;
		}
		i++;
	}
	if(segment_buffer->size() != max_refer_count) {
		err_msg1("sort segments error!");
		return false;
	}
	printf("%s, %d\n", __FILE__, __LINE__);
	return true;

}
bool rewrite(Bin *bin){

	FingerChunk *fc;
	ChunkList *cl;
	ChunkMeta *cmeta;
	int err;
	//printf("%s, %d\n", __FILE__, __LINE__);
	if (segment_buffer->size() > max_refer_count) {
		if(!sort_segments()){
			return false;

		}
	}
	while (cl = (ChunkList *)queue_pop(chunk_buffer)) {
		int fd;
		if (!SIMULATE) {
			fd = open(cl->path, O_RDWR);
		}
		
		//printf("%s, %d, file %s \n", __FILE__, __LINE__, cl->path);
		for(fc = cl->first; fc != NULL; fc = fc->next) {
			if (fc->is_new == NEW_CHUNK) {
				cmeta = (ChunkMeta *)bin->fingers->lookup((unsigned char *)fc->chunk_hash);
				if (cmeta != NULL){
					fc->is_new = DEDUP_CHUNK;
					memcpy(fc->chunkaddress, cmeta->address, CHUNK_ADDRESS_LENGTH);
				}else {
					err = write_chunk_to_cloud(fd, fc->offset, fc);
					if(err == false) {
						err_msg1("write new chunk to cloud error");
						return false;
					}
					cmeta = (ChunkMeta*)malloc(sizeof(ChunkMeta));
					memset(cmeta, 0, sizeof(ChunkMeta));
					memcpy(cmeta->finger, fc->chunk_hash, FINGER_LENGTH);
					memcpy(cmeta->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
					bin->fingers->insert((unsigned char*)cmeta->finger, cmeta); 
				}
			
			}else if (fc->is_new == DEDUP_CHUNK) {
				if(!(segment_buffer->lookup((unsigned char *)fc->chunkaddress))) {
					err = write_chunk_to_cloud(fd, fc->offset, fc);
					if(err == false) {
						err_msg1("write new chunk to cloud error");
						return false;
					}
					fc->is_new = NEW_CHUNK;
					cmeta = (ChunkMeta *)bin->fingers->lookup((unsigned char *)(fc->chunk_hash));
					memcpy(cmeta->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);

				}


			}
			pthread_mutex_unlock(&(fc->mutex));
			if (fc == cl->last) {
				break;
			}	
			
		}
		
		free(cl);
		if (!SIMULATE)
			close(fd);
	}

	segment_buffer->destroy();
	RefNode rn;
	segment_buffer = new htable((char*)&rn.next - (char*)&rn, sizeof(Chunkaddress), 50);
	return true;
	//printf("%s, %d\n", __FILE__, __LINE__);
}
bool capping_process(FileInfo *fileinfo, Bin *bin){

	FingerChunk *fc;
	ChunkMeta *cmeta;
	RefNode *rn;
	ChunkList *cl = NULL;
	
	if(fileinfo->chunknum == STREAM_END) {
		//if(cl != NULL) {
		//	queue_push(chunk_buffer, (void *)cl);
		//}
		memset(fileinfo->file_hash, 0, FINGER_LENGTH);
		Bin *reload_bin = LoadBin(fileinfo);
		if(!rewrite(reload_bin)) {
			return false;
		}
		return true;
	}
	//printf("%s, %d, file %s\n", __FILE__, __LINE__, fileinfo->file_path);
	cl = (ChunkList *)malloc(sizeof(ChunkList));
	memset(cl, 0, sizeof(ChunkList));
	strcpy(cl->path, fileinfo->file_path);
	cl->first = fileinfo->first;
	cl->last = fileinfo->first;
	//if(fileinfo->file_size + buffer_length <= buffer_capacity) {
		
		//cl->last = fileinfo->last;
		//queue_push(chunk_buffer, (void *)cl);
		
	//}else {}
	//printf("%s, %d\n", __FILE__, __LINE__);
	for (fc = fileinfo->first; fc != NULL; fc = fc->next) {
	//	printf("%s, %d\n", __FILE__, __LINE__);
		if (fc->chunklen + buffer_length > buffer_capacity) {
			if(fc != fileinfo->first){
				queue_push(chunk_buffer, (void *)cl);
				if(!rewrite(bin)) {
					return false;
				}
				buffer_length = 0;
				cl = (ChunkList *)malloc(sizeof(ChunkList));
				memset(cl, 0, sizeof(ChunkList));
				strcpy(cl->path, fileinfo->file_path);
				cl->first = fc;
				cl->last = fc;
			}else {
				if(!rewrite(bin)) {
					return false;
				}
				buffer_length = 0;
			}
		}
		//printf("%s, %d\n", __FILE__, __LINE__);
		cl->last = fc;
		buffer_length += fc->chunklen;
		cmeta = (ChunkMeta *)bin->fingers->lookup((unsigned char *)fc->chunk_hash);
		if (cmeta == NULL) {
			fc->is_new = NEW_CHUNK;
			
		}else if (cmeta != NULL) {
			fc->is_new = DEDUP_CHUNK;
			memcpy(fc->chunkaddress, cmeta->address, CHUNK_ADDRESS_LENGTH);
			rn = (RefNode *)segment_buffer->lookup((unsigned char *)fc->chunkaddress);
			
			if (rn == NULL) {
				rn = (RefNode *)malloc(sizeof(RefNode));
				rn->link = NULL;
				memcpy(rn->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
				rn->ref_length = fc->chunklen;
				segment_buffer->insert((unsigned char *)rn->address, rn);
				
			}else {
				rn->ref_length += fc->chunklen;

			}
		}
		//printf("%s, %d\n", __FILE__, __LINE__);
		pthread_mutex_lock(&fc->mutex);
		
	}
	queue_push(chunk_buffer, (void *)cl);
	//printf("%s, %d\n", __FILE__, __LINE__);
	return true;
}
