#ifndef PERFECT_REWRITE_H_
#define PERFECT_REWRITE_H_


typedef struct segment_usage{
	Chunkaddress address;
	int32_t length;
	hlink next;
}SegUsage;
bool perfect_rewrite_init();

bool perfect_rewrite_process(FileInfo *fileinfo, Bin *bin);

bool perfect_rewrite_destory();

#endif
