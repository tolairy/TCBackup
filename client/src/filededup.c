#include "../global.h"


/* global file lever dedup */
int file_dudup(Client *c, char *path)
{
	int err;
	int is_new;
    FileInfo *fileinfo = NULL;
	struct stat buf;

	stat(path, &buf);
	c->jcr->old_size += buf.st_size;
	c->jcr->file_count += 1;
	
	fileinfo = file_new();
    memcpy(fileinfo->file_path, path, strlen(path));
    err = SHA1File(path, fileinfo->file_hash);
	if(err != shaSuccess) {
        err_msg1("calculate file hash failed!");
		file_free(fileinfo);
        return FILEDEDUPERROR;
	}
    
	err = is_new_file(c, fileinfo);
	if(err == FAILURE) {
		err_msg1("is_new_file failed!\n");
		file_free(fileinfo);
		return FILEDEDUPERROR;
	}
	if(fileinfo->is_new == NEW_FILE) {
		printf("%s,%d:file %s is new file, now it will be local chunk level deduplication!\n", __FILE__, __LINE__, fileinfo->file_path);
		err = chunk_dedup(c, fileinfo); 
		if(err != 0) {
			err_msg1("chunk_dedup error\n");
			file_free(fileinfo);
			return FILEDEDUPERROR;
		}
			
	}else if(fileinfo->is_new == DEDUP_FILE) {
		printf("%s,%d:file %s can shared with others!\n", __FILE__, __LINE__, fileinfo->file_path);
		/* recvice deplicate chunk information */
		err = recv_deplicate_chunk(c, fileinfo); 
		if(err == FAILURE) {
			err_msg1("recv_deplicate_chunk error\n");
			file_free(fileinfo);
			return FILEDEDUPERROR;
		}
		
		c->jcr->total_dedup_size += buf.st_size;
		c->jcr->file_dedup_size += buf.st_size;
		c->jcr->dedup_file_count += 1;
	}
	
	err = update_file_recipe(c, fileinfo);
	if(err == false) {
		err_msg1("update_file_recipe error\n");
		file_free(fileinfo);
		return FILEDEDUPERROR;
	}
	
	file_free(fileinfo);
	return FILEDEDUPSUCCESS;

	
}


int is_new_file(Client *c, FileInfo *fileinfo)
{
    int err;
	char buf[256] = {0};
	int len;
	char stream[30] = {0};
	char success;
	char newfile;

	TIMER_DECLARE(start, end);
	TIMER_START(start);
	
    snprintf(stream,30,"%d", FILE_HASH);
	bnet_send(c->fd, stream, strlen(stream));

    memcpy(buf, (char *)fileinfo->file_hash, FINGER_LENGTH);
	err = bnet_send(c->fd, buf, FINGER_LENGTH);
	if(err == false){
		err_msg1("net_send failed!");
		return FAILURE;
	}

	memset(buf, 0, sizeof(buf));
	err = bnet_recv(c->fd, buf, &len);
	if(err == false){
		err_msg1("net_recv failed!");
		return FAILURE;
	}

	TIMER_END(end);
	TIMER_DIFF(c->jcr->file_dedup_time, start, end);
	
	success = *((char *)buf);
	if(success == FAILURE) {
		err_msg1("other err happen in dir");
		return FAILURE;
	}
	newfile = *((char *)(buf + 1));
	if(newfile == NEW_FILE) {
		fileinfo->is_new = NEW_FILE;
	}else{
		fileinfo->is_new = DEDUP_FILE;
	}
	
	return SUCCESS;
}


/* recive duplicate File chunk information */
int recv_deplicate_chunk(Client *c, FileInfo *fileinfo)
{
	int len;
	int i;
	int chunknum;
	int err;
    FingerChunk *fc = NULL;
    char *buf = NULL;
	//printf("%s,%d:%s\n", __FILE__,__LINE__,"recv_deplicate_chunk");

    buf = (char *)malloc(FINGER_LENGTH + CHUNK_ADDRESS_LENGTH + 1);
	
	TIMER_DECLARE(start,end);
	TIMER_START(start);
	
	err = bnet_recv(c->fd, (char *)&chunknum ,&len);
	if(err == false){
		err_msg1("net_recv failed!");
		return FAILURE; 
	}
	
 //  printf("%s,%d:chunknum:%d\n",__FILE__,__LINE__,chunknum);
    for(i = 0; i < chunknum; i++) {
		memset(buf, 0, FINGER_LENGTH + CHUNK_ADDRESS_LENGTH + 1);
		err = bnet_recv(c->fd, buf ,&len);
		if(err == false){
		    err_msg1("net_recv failed!");
		    return FAILURE;
	    }
	    fc = fingerchunk_new();
		memcpy(fc->chunk_hash, buf, FINGER_LENGTH);
        memcpy(fc->chunkaddress, buf + FINGER_LENGTH, CHUNK_ADDRESS_LENGTH);
		
		//printf("%s,%d, receive chunk,address is %s\n", __FILE__,__LINE__,fc->chunkaddress);
		
	    file_append_fingerchunk(fileinfo, fc);
    }

	TIMER_END(end);
	TIMER_DIFF(c->jcr->recv_chunk_info_time, start, end);
	
    free(buf);
    return SUCCESS;
}

