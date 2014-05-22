#include "../global.h"

#ifdef STORE_TEST_
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>

#include "store.h"
#endif

Container *container = NULL;
htable *got_containers;
extern int64_t download_size;
extern int64_t download_count;
CacheNode *restore_cache;


void store_init()
{
	
	cloud_upload_init();
	container = InitContainer();

}

int store_chunk(Chunk *chunk)
{
	if (AppendChunkToContainer(container, chunk) == false) {
		
		WriteContainer(container);
		FreeContainer(container);
		container = InitContainer();
		AppendChunkToContainer(container, chunk);
		memcpy(chunk->address, container->name, CHUNK_ADDRESS_LENGTH);
		return SUCCESS;
	}
	
	memcpy(chunk->address, container->name, CHUNK_ADDRESS_LENGTH);
	return SUCCESS;

}
void store_destory()
{
	if(container->chunk_num > 0){
		WriteContainer(container);
		FreeContainer(container);
	}
	cloud_upload_destory();
}

void restore_init()
{
	GotContainer downloaded_container;
	got_containers = new htable((char *)&(downloaded_container.next) - (char *)&downloaded_container, CHUNK_ADDRESS_LENGTH, 3200);
	restore_cache = init_cache();
	
}
int restore_chunk(Chunk *chunk)
{
	GotContainer *downloaded_container;
	
	if (!(got_containers->lookup((unsigned char *)chunk->address))) {

		/*int log_fd;
		int tmplen;
		char tmp[256];
		log_fd = open("test/get_container_log", O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
		tmplen = sprintf(tmp, "container=%s is not exist\n", chunk->address);
		write(log_fd, tmp, tmplen);
		int i;
		for (i = 0; i< CHUNK_ADDRESS_LENGTH; i++) {
			tmplen = sprintf(tmp, "%d/", (chunk->address)[i]);
			write(log_fd, tmp, tmplen);
		}
		tmplen = sprintf(tmp, "\n");
		write(log_fd, tmp, tmplen);
		close(log_fd);*/
		
		printf("%s, %d, container=%s is not exist\n",__FILE__,__LINE__,chunk->address);	
		
		if (CLOUD) {
			char cloud_path[FILE_NAME_LEN];
			char local_path[FILE_NAME_LEN];
			strcpy(cloud_path, cloud_dir);
			strcat(cloud_path, chunk->address);
			strcpy(local_path, TmpPath);
			strcat(local_path, chunk->address);
			init_bcs();
			if (!get_from_cloud(local_path, cloud_path)) {
				err_msg1("download container error!");
				free_bcs();
				return FAILURE;
			
			}
			free_bcs();
		}
		else{
			struct stat buf;
			char tmp[MAX_PATH];
			sprintf(tmp, "%s%s", TmpPath, chunk->address);
			stat(tmp, &buf);
			download_size += buf.st_size;
			download_count += 1;
			
		}
		downloaded_container = (GotContainer *)malloc(sizeof(GotContainer));
		memset(downloaded_container, 0, sizeof(GotContainer));
		memcpy(downloaded_container->address, chunk->address, CHUNK_ADDRESS_LENGTH);
		got_containers->insert((unsigned char *)downloaded_container->address, (void *)downloaded_container);
		
	}
	else{
		printf("%s, %d, container=%s is already exist\n",__FILE__,__LINE__,chunk->address);
		
	}
	
	
	if (search_cache(&restore_cache, chunk->address) == FAILURE) {
		
		printf("%s, %d, container %s is not in cache\n", __FILE__, __LINE__, chunk->address);
		if (ReadContainer(chunk->address, restore_cache->container) == FAILURE) {
			err_msg1("read container error!");
			return FAILURE;
		}
	}
	
	ReadChunkFromContainer(restore_cache->container, chunk);
	if(chunk->data == NULL)
	{
		err_msg1("read chunk err!");
		return FAILURE;
	}
	
	return SUCCESS;
}

void restore_destory()
{
	printf("%s, %d, got containers num is %d", __FILE__, __LINE__, got_containers->size());
	got_containers->destroy();
	//delete got_containers;
	free_cache(restore_cache);
}

#ifdef STORE_TEST_
void main()
{
		int len;
    int fd =  open("store.c", O_RDWR);
		char buf[1024];
		chunk_address address;
		Ref reference;
		while(len = read(fd,buf,1024)){
			if(store_chunk(buf, len, address)){
			 printf("%s\n", address);
			
			 string_to_ref(&reference, address);
			 printf("%s,%s\n", reference.segment, reference.chunk_id);
			}
		}
		//store_chunk(test, strlen(test));
    //store_chunk(test, strlen(test));

		//store_chunk(test, strlen(test)); 
		stop_store();
}
#endif 
