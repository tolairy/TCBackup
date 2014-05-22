#include "../global.h"

SyncQueue *file_pool;

int create_file(char *path)
{

	int fd=0;
    char *p;
    int i=0,len=0;
    char filepath[MAX_PATH];
    char ss[MAX_PATH];
    memset(ss,'\0',sizeof(ss));
    memset(filepath,'\0',sizeof(filepath));
    memcpy(filepath, path, strlen(path));
	//printf("%s\n",filepath);
    len = strlen(filepath);
    printf("%d\n",len);
	p=strtok(filepath,"/");
    while(p)
    {
       //printf("%s\n",p);
	   memcpy(ss+i,"/",strlen("/"));
       i=i+strlen("/");
       //printf("%d\n", strlen(p)+i);
       if((i+strlen(p))==len)
       {
           break;
       }
       memcpy(ss+i,p,strlen(p));
       i=i+strlen(p);
       if(access(ss,F_OK)!=0)
       {
           mkdir(ss,0777);
       }
       p=strtok(NULL,"/");
    }
    //printf("%s,%d:%s\n", __FILE__,__LINE__,path);
    fd = open(path, O_CREAT|O_RDWR, S_IROTH|S_IWOTH);
    if (fd < 0)
    {
        return 0; 
    }
	close(fd);
	return 1;
}

/*receive file list of the job after sending job id*/
int recv_file_list(Client *client, DList *filelist)
{
	char buf[256];
	char *fileinfo_buf;
	int err,len,i;
	int32_t file_num,fileinfo_len;
	char *p;
	
	memset(buf, 0, sizeof(buf));
    err = bnet_recv(client->fd, buf, &len);
    if(err <= 0) {
        err_msg1("bnet_recv failed!");
        return FAILURE;
    }
	
	memcpy(&file_num, buf, sizeof(int32_t));
	fileinfo_buf =(char *) malloc(file_num *( FINGER_LENGTH+FILE_NAME_LEN+sizeof(int32_t)));
	
	printf("%s,%d:%s%d\n", __FILE__,__LINE__,"enter recv_file_list,file num is", file_num);
	
	err = bnet_recv(client->fd, fileinfo_buf, &len);
	 if(err <= 0) {
        err_msg1("bnet_recv failed!");
        return FAILURE;
    }
	 
	p = fileinfo_buf;
	for (i = 0; i < file_num; i++) {
		FileInfo *fileinfo = file_new();
		memcpy(&fileinfo_len, p, sizeof(int32_t));
		p += sizeof(int32_t);
		memcpy(fileinfo->file_hash, p, FINGER_LENGTH);
		p += FINGER_LENGTH;
		memcpy(fileinfo->file_path, p, fileinfo_len - FINGER_LENGTH);
		p += fileinfo_len - FINGER_LENGTH;
		dlist_insert(filelist, (void *)fileinfo);
	}

	free(fileinfo_buf);
	return SUCCESS;

}

DList *get_job_info(Client *client)
{
	int err;
	FileInfo *fileinfo;
	char buf[256] = {0};
	
	printf("%s,%d, in get job info ,jobid is %d\n", __FILE__,__LINE__, client->jcr->id);

	sprintf(buf, restore_cmd, client->jcr->id);
	err = bnet_send(client->fd, buf, strlen(buf));
	if (err == FAILURE) {
		err_msg1("send job id error!");
		return NULL;
	}
	
	DList *filelist = dlist_init();
	err = recv_file_list(client, filelist);
	if (err == FAILURE) {
		err_msg1("receive file list from dir error! ");
		return NULL;
	}

	return filelist;

}


void *construct_file(void *arg)
{
	int i,err,fd;
	FingerChunk *p;
	char full_path[FILE_NAME_LEN];
	Bin *bin;
	
	Client *client = (Client *)arg;
	FileInfo *fileinfo= (FileInfo *)sync_queue_pop(file_pool);
	Chunk *chunk = (Chunk *)malloc(sizeof(Chunk));
	
	restore_init();
	
	while (fileinfo->file_size != -1) {
		
		if (REWRITE != NO_REWRITE) {
			/*如果重写过，则需要重bin里获取块的地址*/

			/*int log_fd;
			int tmplen;
			char tmp[256];
			log_fd = open("test/rewrite_restore", O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
			*/
			
			calculate_rep_finger(fileinfo);
			bin = LoadBin(fileinfo);
			//tmplen = sprintf(tmp, "bin size is %lld\n", bin->fingers->size());
			//write(log_fd, tmp, tmplen);
			printf("%s, %d, bin size is %d\n", __FILE__, __LINE__, bin->fingers->size());
			FingerChunk *fc;
			ChunkMeta *cmeta;
			for (fc = fileinfo->first; fc != NULL; fc = fc->next) {
				cmeta = (ChunkMeta *)bin->fingers->lookup((unsigned char *)fc->chunk_hash);
				if(cmeta == NULL) {
					err_msg1("restore file error!\n");
					return NULL;
				}
				memcpy(fc->chunkaddress, cmeta->address, CHUNK_ADDRESS_LENGTH);
			}	
			
			//close(log_fd);
		}
		
		memset(full_path, 0, FILE_NAME_LEN);
		strcpy(full_path, client->jcr->restore_path);
		strcat(full_path, fileinfo->file_path);
		printf("%s,%d,full path is %s\n",__FILE__,__LINE__,full_path);
		if(!create_file(full_path)) {
			err_msg1("create file error");
			return NULL;
		}
		fd = open(full_path, O_RDWR | O_CREAT, 0777);
		if (fd < 0) {
			err_msg1("open file error!");
			return NULL;
		}
		
		p = fileinfo->first;	
		while (p != NULL) {
			printf("%s,%d, address %s\n", __FILE__,__LINE__, p->chunkaddress);
			memset(chunk->address, 0, CHUNK_ADDRESS_LENGTH);
			//strcpy(chunk->address, p->chunkaddress);
			memcpy(chunk->address, p->chunkaddress, CHUNK_ADDRESS_LENGTH);
			memcpy(chunk->hash, p->chunk_hash, FINGER_LENGTH);
			err = restore_chunk(chunk);
			if	(err == FAILURE) {
				err_msg1("restore chunk error!");
				return NULL;
			}
			if (write(fd, chunk->data, chunk->length) != chunk->length) {
				err_msg1("write chunk date error!");
				free(chunk->data);
				return NULL;
			}
			free(chunk->data);
			p=p->next;
		}
		close(fd);
		file_free(fileinfo);
		fileinfo = (FileInfo *)sync_queue_pop(file_pool);
	}
	restore_destory();
	file_free(fileinfo);
	
	return NULL;
}


int get_files(Client *client, DList *filelist)
{
		pthread_t download;
		int err;
		FileInfo *fileinfo;
		file_pool = sync_queue_new();
		
		err = pthread_create(&download, NULL, construct_file, (void *)client);
		if (err !=0) {
			err_msg1("create pthread error!");
			return FAILURE;
		}

		while (dlist_size(filelist)) {
			fileinfo = (FileInfo *)dlist_delete_tail(filelist);
			recv_deplicate_chunk(client, fileinfo);
			sync_queue_push(file_pool, fileinfo);	
		}

		fileinfo = file_new();
		fileinfo->file_size = -1;
		sync_queue_push(file_pool, fileinfo);	
		
		pthread_join(download, NULL);
		sync_queue_free(file_pool);
		return SUCCESS;

}


