#ifndef INDEX_H_
#define INDEX_H_


typedef struct index_item{
	Fingerprint hash; 
	int64_t offset;
	hlink next;
}IndexItem;


typedef struct memindex{
	htable *index_table;            /* include all ContainerAddr items in disk*/
	//htable *index_new;            /* new item (only include fingerprints */
	//Cache *index_cache;           /* part of ChunkAddr items in memory */
	pthread_mutex_t mutex;
}MemIndex;

bool index_open();

bool index_close();
	
void index_init();

bool index_insert(IndexItem *item);

IndexItem* index_lookup(unsigned char *key);

void index_destroy();


#endif
