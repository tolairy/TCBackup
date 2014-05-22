#include "../global.h"

/*send the filehashs of to the client
 *send file number 
 *then send the filehashs*/
int send_job_info(DedupClient *client, Queue *filelist)
{
	int err,i;
	char *p;
	char *fileinfo;
	int32_t fileinfo_len, filenum;
	
	char *buf = (char *)malloc((FINGER_LENGTH + FILE_NAME_LEN+sizeof(int32_t)) * queue_size(filelist) + 1);
	//char buf[1000];
	filenum = (int32_t)queue_size(filelist);
	//memset(buf, 0, sizeof(buf));
	printf("%s,%d:%s%d\n", __FILE__,__LINE__,"enter send job files,file num is ", filenum);
	
	memcpy(buf, &filenum, sizeof(int32_t));
	err = bnet_send(client->client_fd, buf, sizeof(int32_t));
    if(err == false) {
    	err_msg1("bnet_send failed!");
		return FAILURE;
    }
	
//	memset(buf, 0, sizeof(buf));
	p=buf;
	
	for(i = queue_size(filelist); i>0; i--) {
		fileinfo = (char *)queue_pop(filelist);
		memcpy(&fileinfo_len, fileinfo, sizeof(int32_t));
		memcpy(p, fileinfo, fileinfo_len+sizeof(int32_t));
		p += fileinfo_len+sizeof(int32_t);
		queue_push(filelist,fileinfo);
	}
	
	err = bnet_send(client->client_fd, buf, p-buf);
 	if (err == false) {
    	err_msg1("bnet_send failed!");
		return FAILURE;
  	}
	printf("%s,%d:%s\n", __FILE__,__LINE__,"exit send job files");
	free(buf);
	return SUCCESS;
}

/*read chunk fingerprints of the file*/
int get_file_info(DedupClient *c, Recipe *rp)
{
    int err;
    char buf[256] = {0};
    IndexItem *index_item = NULL;
	
	printf("%s,%d:%s\n", __FILE__,__LINE__,"get file info");
    index_item = index_lookup(rp->file_hash);
    if(index_item == NULL) {
		err_msg1("index lookup error!");
		return FAILURE;
    }

    err = read_recipe_from_vol(index_item->offset, rp);
    if(err == false) {
        err_msg1("read file recipe error!");
        return FAILURE;
    }
    return SUCCESS;
}

