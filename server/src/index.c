#include "../global.h"


static MemIndex *mem_index;

bool index_open()
{ 
	char path[FILE_NAME_LEN] = {0};
	int fd;
	int32_t count = 0;
	int i;
	IndexItem *index_item = NULL;
	strcpy(path, store_path);
	strcat(path, "global_index");
	fd = open(path, O_CREAT|O_RDWR, S_IRWXU);
	if(fd < 0){
		err_msg1("open global index error!");
		return false;
	}
	lseek(fd, 0, SEEK_SET);
	if(read(fd, &count, 4) == 4) {
		for(i = 0; i < count; i++) {
			index_item = (IndexItem *)malloc(sizeof(IndexItem));
			read(fd, index_item->hash, sizeof(Fingerprint));
			read(fd, &index_item->offset, 8);
			htable_insert(mem_index->index_table, index_item->hash, index_item);
		}
		printf("%s,%d,total index items:%u\n",__FILE__, __LINE__, mem_index->index_table->num_items);
			
	}
	close(fd);
	return true;
}


bool index_close()
{
	char path[FILE_NAME_LEN] = {0};
	int fd;
	IndexItem *index_item = NULL;

	strcpy(path,store_path);
	strcat(path,"global_index");
	fd = open(path, O_RDWR);
	if(fd < 0){
		err_msg1("open global index error!");
		return false;
	}

	lseek(fd, 0, SEEK_SET);
	write(fd, &(mem_index->index_table->num_items), sizeof(int32_t));
	foreach_htable(index_item, mem_index->index_table) {
	    write(fd, index_item->hash, sizeof(Fingerprint));
		write(fd, &index_item->offset, sizeof(int64_t));
	}
	close(fd);
	return false;
}

void index_init()
{
    IndexItem index_item;

	mem_index = (MemIndex *)malloc(sizeof(MemIndex));
	
	mem_index->index_table = htable_init((char*)&(index_item.next) - (char*)&index_item, sizeof(Fingerprint), HASH_TBALE_SIZE);
	//index->index_new = htable_init((char*)&(cdr.next) - (char*)&cdr, sizeof(Fingerprint), 10*1024);
	//index->index_cache = cache_init(30*1024*1024);
	
	pthread_mutex_init(&mem_index->mutex, 0);
	index_open();
	return;
}

/* insert the items of container ct  into MemDex */
bool index_insert(IndexItem *item)
{
	P(mem_index->mutex);
	IndexItem *index_item = NULL;

	index_item = (IndexItem *)htable_lookup(mem_index->index_table, item->hash);
    
    if(index_item == NULL) {
    	htable_add(mem_index->index_table, item->hash, item);
    }	
	
	V(mem_index->mutex);
	return true;
}


IndexItem* index_lookup(unsigned char *key)
{ 
	P(mem_index->mutex);
	IndexItem *index_item = NULL;
	
	// First Step: search it from cache in memory firstly 

	// Secondly, search it  in Bloom Filter which is not completed now
	
	// Thirdly, search it from global indexs in disk (Actually, they are stored in memory, but we assum they are stored in disk)

	index_item = (IndexItem *)htable_lookup(mem_index->index_table, key);
	V(mem_index->mutex);
    return index_item;
}


void index_destroy()
{
	index_close();
	htable_destroy(mem_index->index_table);
	free(mem_index);
	return;
}

