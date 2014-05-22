#ifndef STORE_H_
#define STORE_H_

typedef struct got_container{
	Chunkaddress address;
	hlink next;
}GotContainer;

void store_init();
int store_chunk(Chunk *chunk);
void store_destory();

void restore_init();
int restore_chunk(Chunk *chunk);
void restore_destory();
#endif
