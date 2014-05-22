#ifndef CONTAINER_CACHE_H_
#define CONTAINER_CACHE_H_


LRUCache * container_cache_simulator_new(int cache_size);
bool container_cache_simulator_look(LRUCache *cache, Chunkaddress address);
bool container_cache_simulator_insert(LRUCache *cache, Chunkaddress address);
bool container_cache_simulator_free(LRUCache *cache);


#endif
