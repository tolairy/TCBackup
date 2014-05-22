#include "../global.h"



bool file_dedup_mark(DedupClient *c, Recipe *rp)
{
    int err;
    char buf[256] = {0};
    IndexItem *index_item = NULL;

    index_item = index_lookup(rp->file_hash);
    if(index_item == NULL) {
    	rp->is_new = NEW_FILE;
        sprintf(buf, "%c%c", SUCCESS, NEW_FILE);
    }else {
    	rp->is_new = DEDUP_FILE;
    	sprintf(buf, "%c%c", SUCCESS, DEDUP_FILE);
	err = read_recipe_from_vol(index_item->offset, rp);
       if(err == false) {
            err_msg1("read file recipe error!");
            return false;
        }
    }
	
	if (rp->is_new == NEW_FILE)
		printf("%s, %d, send file dedup result, file is new\n", __FILE__,__LINE__);
	else 
		printf("%s, %d, send file dedup result, file is dedup\n", __FILE__,__LINE__);
	
    err = bnet_send(c->client_fd, buf, strlen(buf));
    if(err == false) {
        err_msg1("bnet_send the result of dedupcation");
        return false;
    }
    return true;
}


bool send_chunk_info(DedupClient *c, Recipe *rp)
{
    int len;
    int err;
    FingerChunk *fc = NULL;
    char *buf = NULL;

	printf("%s,%d:%s\n", __FILE__,__LINE__,"send_chunk_info");
	
    buf = (char *)malloc(FINGER_LENGTH + CHUNK_ADDRESS_LENGTH+ 1);
    memset(buf, 0, FINGER_LENGTH + CHUNK_ADDRESS_LENGTH+ 1);
    memcpy(buf, &rp->chunknum, sizeof(rp->chunknum));
   // printf("%s,%d:chunknum:%d\n",__FILE__,__LINE__,rp->chunknum);
    err = bnet_send(c->client_fd, buf, sizeof(rp->chunknum));
    if(err == false) {
		free(buf);
        err_msg1("bnet_send chunknum failed");
        return false;
    }

    fc = rp->first;
    for(fc; fc != NULL; fc = fc->next) {
        memset(buf, 0, FINGER_LENGTH + CHUNK_ADDRESS_LENGTH+ 1);
        memcpy(buf, fc->chunk_hash, FINGER_LENGTH);
        memcpy(buf + FINGER_LENGTH, fc->chunk_address, CHUNK_ADDRESS_LENGTH);
        err = bnet_send(c->client_fd, buf, FINGER_LENGTH + CHUNK_ADDRESS_LENGTH);
        if(err == false) {
			free(buf);
            err_msg1("bnet_send updateattr msg failed");
            return false;
        }
    }
	free(buf);
    return true;
}

/* recive duplicate File chunk information */
bool recv_deplicate_chunk(DedupClient *c, Recipe *rp, int32_t chunknum)
{
	int len;
	int i;
	int err;
    FingerChunk *fc = NULL;
    char *buf = NULL;
    Fingerprint chunk_hash;
    Chunkaddress chunk_address;

    printf("%s,%d:recv_deplicate_chunk\n",__FILE__,__LINE__);
    buf = (char *)malloc(FINGER_LENGTH + CHUNK_ADDRESS_LENGTH + 1);

    for(i = 0; i < chunknum; i++) {
		memset(buf, 0, FINGER_LENGTH + CHUNK_ADDRESS_LENGTH + 1);
		err = bnet_recv(c->client_fd, buf ,&len);
		if(err == false){
		    err_msg1("net_recv failed!");
		    return false;
	    }
	    memcpy(chunk_hash, buf, FINGER_LENGTH);
           memcpy(chunk_address, buf + FINGER_LENGTH, CHUNK_ADDRESS_LENGTH);
	    fc = fingerchunk_new(chunk_hash, chunk_address);
	    recipe_append_fingerchunk(rp, fc);
    }
    free(buf);
    return true;
}
