#ifndef CAPPING_H_
#define CAPPING_H_

typedef struct ref_node{
	Chunkaddress address;
	int64_t ref_length; 
	struct ref_node *link;
	hlink next;
}RefNode;
typedef struct chunks_list{
	char path[FILE_NAME_LEN];
	FingerChunk *first;
	FingerChunk *last;
	
}ChunkList;
bool capping_init();
bool capping_process(FileInfo *fileinfo, Bin *bin);
bool capping_destory();
#endif
