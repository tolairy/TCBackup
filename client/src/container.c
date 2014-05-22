#include "../global.h"


static const int32_t cvol_magic_key = 0x7856ffee;
static const int32_t container_valid = 0xf5f5e4e4;
int randomabc = 1;
/*
 * Read a container from container volume.
 * return NULL if failure.
 */


int ReadContainer(char *name, Container *container){
	char container_path[128];
	int fd;
	int32_t valid;
	char *p;

	char *buffer = (char*)malloc(DEFAULT_CONTAINER_SIZE);

	strcpy(container_path, TmpPath);
	strcat(container_path, name);
	
	memset(container->name, 0, CHUNK_ADDRESS_LENGTH);
	strcpy(container->name, name);
	
	//printf("%s, %d: read container, name is %s\n",__FILE__,__LINE__,container_path);
	if ((fd = open(container_path, O_RDONLY)) < 0) {
		err_msg1("open container error!");
		return FAILURE;
	}

	read(fd, buffer, DEFAULT_CONTAINER_SIZE);
	p = buffer;
	
	memcpy(&valid, p, sizeof(valid));
	p += sizeof(valid);
    if(valid!=container_valid){
        err_msg1("invalid container!");
        free(buffer);
		close(fd);
        return FAILURE;
    }
	memcpy(&(container->chunk_num), p, sizeof(int32_t));
	p += sizeof(int32_t);
	memcpy(&(container->data_size), p, sizeof(int32_t));
	p += sizeof(int32_t);

    int i;
    for(i=0; i<container->chunk_num; ++i){
        ChunkAddr *addr = (ChunkAddr*)malloc(sizeof(ChunkAddr));
		
		memcpy(&(addr->offset), p, sizeof(int32_t));
		p += sizeof(int32_t);
     	memcpy(&(addr->length), p, sizeof(int32_t));
		p += sizeof(int32_t);
       	memcpy(&(addr->hash), p, FINGER_LENGTH);
		p += FINGER_LENGTH;
     
        container->meta_table->insert((unsigned char*)&addr->hash, addr);
    }
    memcpy(container->data_buffer, p, container->data_size);

    free(buffer);
	close(fd);
    return SUCCESS;
}

void ReadChunkFromContainer(Container *container, Chunk *chunk)
{
	
    ChunkAddr *addr = (ChunkAddr*)container->meta_table->lookup((unsigned char *)(chunk->hash));
    if(!addr){
        err_msg1("%s, %d: Such chunk does not exist in this container!\n");
        chunk->data = NULL;
		return;
    }
	
    if(addr->length<0||addr->offset > container->data_size || addr->offset < 0){
        err_msg1(" Invalid chunk length or offset");
		chunk->data = NULL;
        return;
    }
 
    chunk->length = addr->length;
    
    if(*(container->data_buffer+addr->offset+chunk->length) != '\t'){
        err_msg1(" Container has been corrupted");
		chunk->data = NULL;
		return;
    }
		chunk->data = (char*)malloc(chunk->length);
    memcpy(chunk->data, container->data_buffer+addr->offset, chunk->length);
    /*IntChunk(chunk);*/

}


bool AppendChunkToContainer(Container *container, Chunk *chunk){
    if((32+container->meta_table->size()*28+container->data_size)>DEFAULT_CONTAINER_SIZE-28-chunk->length-1){
      //  printf("%s, %d: Container is full\n",__FILE__,__LINE__);
        return false;
    }
	
   	memcpy(container->data_buffer+container->data_size, chunk->data, chunk->length);
	
    ChunkAddr *new_addr = (ChunkAddr*)malloc(sizeof(ChunkAddr));
    new_addr->length = chunk->length;
    new_addr->offset = container->data_size;
    memcpy(&new_addr->hash, &chunk->hash, sizeof(Fingerprint));
    container->meta_table->insert((unsigned char*)&new_addr->hash, new_addr);
    container->data_size += chunk->length;
    memset(container->data_buffer+container->data_size, '\t', 1);
    container->data_size++;
    container->chunk_num++;
    return true;
}
void GetContainerName(char *name){
	randomabc += 1;	
	sprintf(name, "%011o%d", (int)time(NULL),randomabc);

}
Container *InitContainer(){
    Container *container = (Container *)malloc(sizeof(Container));
    container->data_size = 0;
	memset(container->name, 0, CHUNK_ADDRESS_LENGTH);
    GetContainerName(container->name);
	container->chunk_num = 0;
    container->data_buffer = (unsigned char*)malloc(DEFAULT_CONTAINER_SIZE);
	memset(container->data_buffer, 0, DEFAULT_CONTAINER_SIZE);
    ChunkAddr tmp;
    container->meta_table = new htable((char*)&tmp.next - (char*)&tmp, sizeof(Fingerprint), 512);

    return container;
}

void FreeContainer(Container *container){
    if(!container)
        return;
    delete container->meta_table;
    if(container->data_buffer)
        free(container->data_buffer);
    free(container);
}


int container_save(char *name, char *data, int len){

	int fd;
	char path[300] = {0};
	strcpy(path, TmpPath);
	strcat(path, name);
	
	printf("%s\n", path);
	fd = open(path, O_RDWR | O_CREAT, 0777);
	if (fd == -1) {
		err_msg1("open error!");
		return FAILURE;
	}
	if (write(fd, data, len) == -1) {
		err_msg1("write error!");
		return FAILURE;
	}
	close(fd);
	return SUCCESS;


}
int WriteContainer(Container *container)
{
	char buf[DEFAULT_CONTAINER_SIZE];
	char *p;
	int len_in,len_out;
	int seg_fd;
	unsigned char * out_buf = (unsigned char *)malloc(DEFAULT_CONTAINER_SIZE);

	p = buf;
	memcpy(p, &container_valid, sizeof(int32_t));
	p += sizeof(int32_t);
	memcpy(p, &(container->chunk_num), sizeof(int32_t));
	p += sizeof(int32_t);
	memcpy(p, &(container->data_size), sizeof(int32_t));
	p += sizeof(int32_t);

	ChunkAddr *addr = (ChunkAddr*)container->meta_table->first();
    while(addr){
		memcpy(p, &(addr->offset), sizeof(int32_t));
		p += sizeof(int32_t);
     	memcpy(p, &(addr->length), sizeof(int32_t));
		p += sizeof(int32_t);
       	memcpy(p, &(addr->hash), FINGER_LENGTH);
		p += FINGER_LENGTH;
        addr = (ChunkAddr*)container->meta_table->next();
    }
	if (!SIMULATE) {
		memcpy(p, container->data_buffer, container->data_size);
		p += container->data_size;
	}
	
	len_in = p - buf;

//	zlib_compress_block((unsigned char *)buf, len_in, out_buf, &len_out);


	container_save(container->name,(char *)buf, len_in);
	
	free(out_buf);
	
	if (CLOUD) {
		//printf("%s, %d, enter write container and upload container %s\n", __FILE__, __LINE__, container->name);
		cloud_upload(container->name);
	}
	//int log_fd;
	//int tmplen = 0;
	//char tmp[256];
	//log_fd = open("test/container_write_log", O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
	//tmplen = sprintf(tmp, "container name %s, data length %d\n", container->name, container->data_size);
	//write(log_fd, tmp, tmplen);
	//close(log_fd);
	
	return SUCCESS;

}

CacheNode *init_cache()
{
	int i;
	
	CacheNode *cache, *tmp, *pretmp;
	for (i = 0; i < restore_cache_size; i++) {
		tmp = (CacheNode *)zmalloc(sizeof(CacheNode));
		tmp->container = InitContainer();
		tmp->next = NULL;
		if (i == 0) {
			cache = tmp;
			pretmp = tmp;
		}else {
			pretmp->next = tmp;
			pretmp = tmp;
		}	

	}
	return cache;

}

int search_cache(CacheNode **restore_cache, char * name)
{
	CacheNode *p,*prep;
	prep = p = *restore_cache;
	
	while (p) {
		if (!strcmp(p->container->name, name)) {
			if (p == *restore_cache) {	
				return SUCCESS;
			}else {
				prep->next = p->next;
				p->next = *restore_cache;
				*restore_cache = p;
				return SUCCESS;
			}
		}else {
			if (p->next != NULL) {
				prep = p;
			}
			
			p = p->next;

		}

	}

	if(prep->next != NULL) {
		p = prep->next;
		prep->next = NULL;
		p->next = *restore_cache;
		*restore_cache = p;
	}
	
	FreeContainer((*restore_cache)->container);
	(*restore_cache)->container = InitContainer();
	return FAILURE;
}

int free_cache(CacheNode * restore_cache)
{

	int i;
	CacheNode *p;
	while (restore_cache) {
		p = restore_cache;
		restore_cache = restore_cache->next;
		FreeContainer(p->container);
		zfree(p);
	}
	return 1;

}

int get_container_size(char *name){

	char container_path[MAX_PATH];
	int fd;
	int32_t valid;
	int32_t chunk_num;
	int32_t data_size;
	int total_length = 0;


	char *buffer = (char*)malloc(DEFAULT_CONTAINER_SIZE);

	strcpy(container_path, TmpPath);
	strcat(container_path, name);
	
	//printf("%s, %d: read container, name is %s\n",__FILE__,__LINE__,container_path);
	if ((fd = open(container_path, O_RDONLY)) < 0) {
		err_msg1("open container error!");
		return FAILURE;
	}

	read(fd, &valid, sizeof(valid));
	
    if(valid!=container_valid){
        err_msg1("invalid container!");
        free(buffer);
		close(fd);
        return -1;
    }
	read(fd, &chunk_num, sizeof(chunk_num));
	read(fd, &data_size, sizeof(data_size));
	close(fd);
	
	struct stat buf;
	char tmp[MAX_PATH];
	sprintf(tmp, "%s%s", TmpPath, name);
	stat(tmp, &buf);
	total_length = buf.st_size + data_size;
	
    return total_length;


}