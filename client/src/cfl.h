#ifndef CFL_H_
#define CFL_H_

typedef struct cfl_rdc_member{
	Chunkaddress container_id;
	hlink next;
}RdcMember;
	
typedef struct cfl_tmp_container{
	Chunkaddress address;
	Queue *chunks;
	int length;
}TmpContainer;
typedef struct chunk_node{
	FingerChunk *fc;
	FileInfo *fileinfo;
}ChunkNode;

bool cfl_init();
bool cfl_process(FileInfo *fileinfo, Bin *bin);
bool cfl_destory();

#endif
