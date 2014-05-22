	#include "../global.h"


/* local chunk lever dedup */
int chunk_dedup(Client *c, FileInfo *fileinfo)
{
	int32_t err;
    Bin *bin;
	
    /* chunk the current file */
    err = chunk_file(fileinfo);
    if(err == FAILURE) {
        err_msg1("chunk file failed");
        return CHUNKDEDUPERROR;
    }
    /* calculate the represent finger of current file */
    calculate_rep_finger(fileinfo);
	
    /* load bin and find the current represent finger in primary index */
    bin = LoadBin(fileinfo);
        
    /* mark the duplicate chunk and insert the new chunk finger into bin */
    err = mark_deplicate_chunk(fileinfo, bin);
    if(err == false) {
        err_msg1("mark the duplicate chunk error!");
        return CHUNKDEDUPERROR;
    } 

	return CHUNKDEDUPSUCCESS;
}


/* calculate the represent finger of a file */
bool calculate_rep_finger(FileInfo *fileinfo)
{
   /* Fingerprint rep_finger;
    FingerChunk *fc = NULL;
    printf("%s,%d:%s\n",__FILE__,__LINE__,"Calculation the represent fingerprint of each file");
    if(fileinfo->chunknum == 0 && fileinfo->first == NULL) {
        printf("%s,%d:%s\n",__FILE__,__LINE__,"The chunk number of this is zero!");
        return true;
    }else {
        memset(rep_finger, 0, FINGER_LENGTH);
        fc = fileinfo->first;
        memcpy(rep_finger, fc->chunk_hash, FINGER_LENGTH);
    }
    fc = fc->next;
    while(fc != NULL) {
        if(memcmp((const char *)rep_finger,(const char *)fc->chunk_hash, FINGER_LENGTH)) {
            memcpy(rep_finger, fc->chunk_hash, FINGER_LENGTH);
        }
        fc = fc->next;
    }
    memcpy(fileinfo->rep_finger, rep_finger, FINGER_LENGTH);*/
    memset(fileinfo->rep_finger, 0, FINGER_LENGTH);
    return true;
}


/* mark the deplicate chunk and update bin */
bool mark_deplicate_chunk(FileInfo *fileinfo, Bin *bin)
{
    FingerChunk *fc = NULL;
    int err;
	int fd;
    int64_t size = 0;
    ChunkMeta *cmeta = NULL;
    fc = fileinfo->first;
    
   // printf("%s,%d:%s\n",__FILE__,__LINE__,"mark_deplicate_chunk");

	if (REWRITE == CFL_REWRITE) { 
		if(!cfl_process(fileinfo, bin)){
			err_msg1("cfl process error! ");
			return false;
		}

	}else if(REWRITE == PERFECT_REWRITE) {
		if (!perfect_rewrite_process(fileinfo, bin)) {
			err_msg1("perfect rewrite error! ");
			return false;
		}
	}else if(REWRITE == CAPPING_REWRITE) {
		if (!capping_process(fileinfo, bin)) {
			err_msg1("capping rewrite error! ");
			return false;
		}
	}else if(REWRITE == NO_REWRITE) {

		if(!SIMULATE) {
			fd = open(fileinfo->file_path, O_RDWR);
			if(fd <= 0) {
				err_msg2("open file %s error", fileinfo->file_path);
				return false;
			}
		}
    	if(fileinfo->is_new == DEDUP_FILE) {
      	  /* duplicate file,update the second index */
      	  while(fc != NULL) {
         	   if(bin->fingers) {
         	       if((cmeta =  (ChunkMeta*)bin->fingers->lookup((unsigned char *)&fc->chunk_hash)) == NULL) {
						cmeta = (ChunkMeta*)malloc(sizeof(ChunkMeta));
						memset(cmeta, 0, sizeof(cmeta));
						memcpy(cmeta->finger, fc->chunk_hash, FINGER_LENGTH);
						memcpy(cmeta->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
						bin->fingers->insert((unsigned char*)&cmeta->finger, cmeta);
            	    }
         	   }
          	  fc = fc->next;
       	 }
    	} else if(fileinfo->is_new == NEW_FILE) {
       	 /* un-duplicate file, store the new chunk and update the second index*/
			while(fc != NULL) {
       	    	 if(bin->fingers) {
              		if((cmeta =  (ChunkMeta*)bin->fingers->lookup((unsigned char *)&fc->chunk_hash)) != NULL) {
                	    fc->is_new = DEDUP_CHUNK;
                 	   memcpy(fc->chunkaddress, cmeta->address, sizeof(cmeta->address));
               		 } else {
							fc->is_new = NEW_CHUNK;
                	  		err = write_chunk_to_cloud(fd, size, fc);
                  	  		if(err == false) {
                   	   		err_msg1("write new chunk to cloud error");
                  	  		}
                   			cmeta = (ChunkMeta*)malloc(sizeof(ChunkMeta));
                   	 		memset(cmeta, 0, sizeof(cmeta));
                   	 		memcpy(cmeta->finger, fc->chunk_hash, FINGER_LENGTH);
                    		memcpy(cmeta->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);

                   			bin->fingers->insert((unsigned char*)&cmeta->finger, cmeta);
                		}
					 
               		 size += fc->chunklen;
            		}
            		fc = fc->next;
        		}
    		
		}
		if (!SIMULATE)
			close(fd);
	}
	
	return true;
	
}


/* write the new chunk to cloud */
bool write_chunk_to_cloud(int fd, int64_t off_set, FingerChunk *fc)
{
    char *data = NULL;
    int err;
    int32_t readNums = 0;
    int32_t sumReadNums = 0;
	Chunk *chunk = (Chunk *)malloc(sizeof(Chunk));
	memcpy(chunk->hash, fc->chunk_hash, FINGER_LENGTH);
	chunk->length = fc->chunklen;
    data = (char *)malloc(fc->chunklen + 1);
	
	if (!SIMULATE) {
		lseek(fd, off_set, SEEK_SET);
			//printf("%s, %d, chunk len is %d, offset is %d\n", __FILE__, __LINE__, fc->chunklen, off_set);
			while(sumReadNums != fc->chunklen) {
				//printf("%s, %d, sumReadNums is %d\n", __FILE__, __LINE__, sumReadNums);
				readNums = read(fd, data + sumReadNums, fc->chunklen - sumReadNums);
				if(readNums < 0){
					err_msg1("read chunk data failed!");
					free(data);
					close(fd);
					return false;
				}
				sumReadNums += readNums;
			}

	}
	
	chunk->data = data;
	err = store_chunk(chunk);
  	memcpy(fc->chunkaddress, chunk->address, CHUNK_ADDRESS_LENGTH);
	
	free(data);
	free(chunk);
	
    return true;
}


/* update the file recipe to dird */
bool update_file_recipe(Client *c, FileInfo *fileinfo)
{
    int err;
    char msg[256] = {0};
    char stream[30] = {0};
    char *buf = NULL, *p = NULL;
    int len;
    FingerChunk *fc = NULL;
	int socket = c->fd;
	
	TIMER_DECLARE(start,end);
	TIMER_START(start);
	
    printf("%s,%d:%s\n",__FILE__,__LINE__,"update_file_recipe");
    snprintf(stream,30,"%d", FILE_RECIPE);
    bnet_send(socket, stream, strlen(stream));
	
    sprintf(msg, update_attr, fileinfo->chunknum, fileinfo->file_path);
    printf("%s,%d:%s\n", __FILE__,__LINE__,msg);

	err = bnet_send(socket, msg, strlen(msg));
	if(err == false) {
		err_msg1("bnet_send updateattr msg failed");
		return false;
	}
	
    if(fileinfo->is_new == NEW_FILE) {
		
        buf = (char *)malloc(FINGER_LENGTH + CHUNK_ADDRESS_LENGTH + 1);

        fc = fileinfo->first;
        for	(fc; fc != NULL;  fc = fc->next) {
            memset(buf, 0, FINGER_LENGTH + CHUNK_ADDRESS_LENGTH + 1);
            memcpy(buf, fc->chunk_hash, FINGER_LENGTH);
            memcpy(buf+FINGER_LENGTH, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
			printf("%s,%d, send recipe, chunkaddress is%s\n", __FILE__,__LINE__,fc->chunkaddress);
            err = bnet_send(socket, buf, FINGER_LENGTH + CHUNK_ADDRESS_LENGTH);
            if(err == false) {
                err_msg1("bnet_send updateattr msg failed");
                return false;
            }
        }
    }
    memset(msg, 0, sizeof(msg));
    err = bnet_recv(socket, msg, &len);
    if(err == false){
        err_msg1("bnet_recv failed");
        return false;
    }
   
		
    if(strncmp(msg, updateattr_ok, strlen(updateattr_ok)) == 0) {
			printf("%s,%d:success update attr!\n", __FILE__,__LINE__);
			free(buf);

			TIMER_END(end);
			TIMER_DIFF(c->jcr->send_recipe_time,start,end);
			
			return true;
    }else if(strncmp(msg, updateattr_fail, strlen(updateattr_fail)) == 0) {
           err_msg1("fail updateattr");
           free(buf);
           return false;
    } 
	
    free(buf);
    return false;
}
