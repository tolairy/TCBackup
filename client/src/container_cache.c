#include "../global.h"

gint container_simulator_cmp_asc(gconstpointer a, gconstpointer b) {
	 return memcmp((char *)a, (char *)b, CHUNK_ADDRESS_LENGTH);
}

LRUCache *container_cache_simulator_new(int cache_size)
{
	LRUCache *cache = lru_cache_new(cache_size, container_simulator_cmp_asc);
	return cache;
}

bool container_cache_simulator_look(LRUCache *cache, Chunkaddress address)
{
	//char *new_address = (char *)malloc(CHUNK_ADDRESS_LENGTH*sizeof(char));
	//memset(new_address, 0, CHUNK_ADDRESS_LENGTH);
	//strcpy(new_address, address);
	void *data = lru_cache_lookup(cache, (void *)address);
	if (data) {
		return true;
	}else {
		return false;
	}

}

bool container_cache_simulator_insert(LRUCache *cache, Chunkaddress address)
{
	char *containerid = (char *)malloc(CHUNK_ADDRESS_LENGTH * sizeof(char));
	//memset(containerid, 0, CHUNK_ADDRESS_LENGTH);
	//strcpy(containerid, address);
	memcpy(containerid, address, CHUNK_ADDRESS_LENGTH);
	void *kickout = lru_cache_insert(cache,(void *)containerid);
	if (kickout) {
		free(kickout);
	}
	return true;
	
}

bool container_cache_simulator_free(LRUCache *cache)
{
	lru_cache_free(cache,(void (*)(void *))free);
	return true;

}
