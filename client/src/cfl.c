#include "../global.h"

htable *RDC;
float CFL,LWM;
TmpContainer *tmp_container;
Chunkaddress last_container = {0};
float default_container_reuse = 0.3;
int total_container_num;
unsigned long int dataset_size;
unsigned long int size_tmp = 0;

bool cfl_init()
{
	CFL = 1;
	total_container_num = 0;
	dataset_size = 0;
	RdcMember rdc_member;
	RDC = new htable((char*)&rdc_member.next - (char*)&rdc_member, sizeof(Chunkaddress), 3200);
	tmp_container = (TmpContainer *)malloc(sizeof(TmpContainer));
	tmp_container->chunks = queue_new();

	return true;
}

bool selective_dedup(FileInfo *fileinfo, FingerChunk *fc, Bin *bin)
{
	int err;
	ChunkMeta *cmeta;
	ChunkNode *cn = NULL;
	printf("%s, %d, chunk address: %s\n", __FILE__, __LINE__, fc->chunkaddress);
	if (queue_size(tmp_container->chunks)!= 0 && strcmp(fc->chunkaddress, tmp_container->address) != 0) {
		/*the chunkaddress is equal to the container id  in the tmp comtainer, process the chunks in tmp container first*/
		printf("%s, %d\n", __FILE__, __LINE__);
		
			printf("%s, %d\n", __FILE__, __LINE__);
			if(((float)(tmp_container->length))/DEFAULT_CONTAINER_SIZE < default_container_reuse){
						/*rewrite the chunks in the tmp container*/
				printf("%s, %d\n", __FILE__, __LINE__);
				while (cn = (ChunkNode *)queue_pop(tmp_container->chunks)){
					int fd;
					fd= open(cn->fileinfo->file_path, O_RDWR);
					err = write_chunk_to_cloud(fd, cn->fc->offset, cn->fc);
           	 		if(err == false) {
            			err_msg1("write new chunk to cloud error");
						return false;
            		 }
					close(fd);
				
					cn->fc->is_new = NEW_CHUNK;
					cmeta = (ChunkMeta *)bin->fingers->lookup((unsigned char *)(cn->fc->chunk_hash));
					memcpy(cmeta->address, cn->fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
					
					if(!(RDC->lookup((unsigned char *)&cn->fc->chunkaddress))) {
						RdcMember *rdc_member;
						rdc_member = (RdcMember *)malloc(sizeof(RdcMember));
						memset(rdc_member->container_id, 0, CHUNK_ADDRESS_LENGTH);
						strcpy(rdc_member->container_id, cn->fc->chunkaddress);
						RDC->insert((unsigned char *)rdc_member->container_id, rdc_member);
						total_container_num ++;			
					}
					pthread_mutex_unlock(&(cn->fc->mutex));
					free(cn);
					
				}	
				tmp_container->length = 0;
			}
			else{
				/*the chunks of the tmp container need not rewrite*/
					if(!(RDC->lookup((unsigned char *)&tmp_container->address))) {
						RdcMember *rdc_member;
						rdc_member = (RdcMember *)malloc(sizeof(RdcMember));
						memset(rdc_member->container_id, 0, CHUNK_ADDRESS_LENGTH);
						strcpy(rdc_member->container_id, tmp_container->address);
						RDC->insert((unsigned char *)rdc_member->container_id, rdc_member);
						total_container_num ++;			
					}	
					while (cn = (ChunkNode *)queue_pop(tmp_container->chunks)){
						pthread_mutex_unlock(&cn->fc->mutex);
						free(cn);

					}
					tmp_container->length = 0;			
			}
							
		printf("%s, %d\n", __FILE__, __LINE__);
		
		
		
		
	}
	
	//printf("%s, %d\n", __FILE__, __LINE__);
	
		/* insert the chunk into tmp container*/
	memset(tmp_container->address, 0 ,CHUNK_ADDRESS_LENGTH);
	memcpy(tmp_container->address, fc->chunkaddress, CHUNK_ADDRESS_LENGTH);
	pthread_mutex_lock(&fc->mutex);
	
	cn = (ChunkNode *)malloc(sizeof(ChunkNode));
	cn->fc= fc;
	cn->fileinfo = fileinfo;
	queue_push(tmp_container->chunks, (void *)cn);
	
	tmp_container->length += fc->chunklen;
	printf("%s, %d, tmp container chunknum :%d\n", __FILE__, __LINE__, queue_size(tmp_container->chunks));
	return true;

}

bool typical_dedup(int fd, FingerChunk *fc, Bin *bin)
{
	int err;
	ChunkMeta *cmeta;
	
	
	if(fc->is_new == NEW_CHUNK){
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
	
	if(!(RDC->lookup((unsigned char *)&fc->chunkaddress))) {
			RdcMember *rdc_member;
			rdc_member = (RdcMember *)malloc(sizeof(RdcMember));
			memset(rdc_member->container_id, 0, CHUNK_ADDRESS_LENGTH);
			strcpy(rdc_member->container_id, fc->chunkaddress);
			RDC->insert((unsigned char *)&rdc_member->container_id, rdc_member);
			total_container_num ++;			
	}	
	
	return true;

}

bool cfl_process(FileInfo *fileinfo, Bin *bin)
{
	int err;
	FingerChunk *fc;
	ChunkMeta *cmeta;
	int chunk_num;
	int fd;
	if (!SIMULATE)
		fd= open(fileinfo->file_path, O_RDWR);
	printf("%s, %d, %s\n", __FILE__, __LINE__, fileinfo->file_path);
	//size_tmp += fileinfo->file_size;
	for(chunk_num = 0, fc = fileinfo->first; chunk_num < fileinfo->chunknum; fc = fc->next){
		printf("%s, %d\n", __FILE__, __LINE__);
		chunk_num++;
		dataset_size += fc->chunklen;
		cmeta = (ChunkMeta *)bin->fingers->lookup((unsigned char *)fc->chunk_hash);
		
		if(cmeta != NULL){
			fc->is_new = DEDUP_CHUNK;
			strcpy(fc->chunkaddress, cmeta->address);
	
			if (CFL < LWM) {
				/* selective deduplication*/
				printf("%s, %d,CFL = %f, total chunknum is %d, dedup chunk(%d) enter selective dedup\n", __FILE__, __LINE__, CFL, fileinfo->chunknum, chunk_num);
				if (!selective_dedup(fileinfo, fc, bin)) {
					err_msg1("selective dedup error!");
					return false;
				} 
					
			}
			else {
				
				/*CFL >= LWM this chunk need not rewirte*/
				printf("%s, %d, total chunknum is %d, dedup chunk(%d) enter typical dedup\n", __FILE__, __LINE__, fileinfo->chunknum, chunk_num);
				if (!typical_dedup(fd, fc, bin)) {
					err_msg1("typical dedup error");
					return false;	
				}
			}

		}
		else{
			/*new chunk*/
			fc->is_new = NEW_CHUNK;
			
			printf("%s, %d, total chunknum is %d, new chunk(%d) enter typical dedup\n", __FILE__, __LINE__, fileinfo->chunknum, chunk_num);
			if (!typical_dedup(fd, fc, bin)) {
				err_msg1("typical dedup error");
				return false;
			}
		}
		CFL = ((float)((dataset_size/DEFAULT_CONTAINER_SIZE) + 1))/total_container_num;
		printf("%s, %d\n", __FILE__, __LINE__);
	}
	printf("%s, %d\n", __FILE__, __LINE__);

	if (!SIMULATE)
		close(fd);
	
	if(fileinfo->chunknum == STREAM_END && queue_size(tmp_container->chunks)!= 0) {
		Bin *bin;
		ChunkNode *cn = NULL;
		bin = LoadBin(fileinfo);
		
						/*the address is not in the RDC*/
			if(((float)(tmp_container->length))/DEFAULT_CONTAINER_SIZE < default_container_reuse){
						/*rewrite the chunks in the tmp container*/
				while (cn = (ChunkNode *)queue_pop(tmp_container->chunks)){
					int fd;
					if (!SIMULATE)
						fd= open(cn->fileinfo->file_path, O_RDWR);
					err = write_chunk_to_cloud(fd, cn->fc->offset, cn->fc);
           	 		if(err == false) {
            			err_msg1("write new chunk to cloud error");
						return false;
            		 }
					if (!SIMULATE)
						close(fd);
					
					cn->fc->is_new = NEW_CHUNK;
					cmeta = (ChunkMeta *)bin->fingers->lookup((unsigned char *)(cn->fc->chunk_hash));
					memcpy(cmeta->address, cn->fc->chunkaddress, CHUNK_ADDRESS_LENGTH);

					pthread_mutex_unlock(&(cn->fc->mutex));
					free(cn);
				}	
			}	
			else {
				while (cn = (ChunkNode *)queue_pop(tmp_container->chunks)){
					pthread_mutex_unlock(&cn->fc->mutex);
					free(cn);
				}

			}
		
		
	}

	printf("%s, %d\n", __FILE__, __LINE__);
	return true;
}


bool cfl_destory()
{

	queue_free(tmp_container->chunks);
	free(tmp_container);
	RDC->destroy();
	//printf("%s, %d, datalength %lld, %lld\n", __FILE__, __LINE__, dataset_size, size_tmp);
	return true;

}
