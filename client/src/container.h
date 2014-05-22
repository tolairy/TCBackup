#ifndef _CONTAINER_H_
#define _CONTAINER_H_

#define DEFAULT_CONTAINER_SIZE (4*1024*1024UL)

typedef char container_name[CHUNK_ADDRESS_LENGTH];
typedef struct chunk_address{
    int32_t offset;
    int32_t length;
    Fingerprint hash;
    hlink next;
}ChunkAddr;

typedef struct chunk_tag{
    int32_t length;
    Fingerprint hash; 
	Chunkaddress address;
    char *data;
}Chunk;

typedef struct container_tag{
    //descriptor
    int32_t data_size;
    int32_t chunk_num;
	container_name name;

    htable *meta_table;
    unsigned char *data_buffer;
}Container;

typedef struct cache_node{
	Container *container;
	struct cache_node *next;
}CacheNode;


Container *InitContainer();
int WriteContainer(Container *container);

void FreeContainer(Container *container);

bool AppendChunkToContainer(Container *container, Chunk *chunk);

void ReadChunkFromContainer(Container *container, Chunk *chunk);

int ReadContainer(char *name, Container *container);

CacheNode *init_cache();

int search_cache(CacheNode **restore_cache, char *name);

int free_cache(CacheNode *restore_cache);

int get_container_size(char * name);

#endif
