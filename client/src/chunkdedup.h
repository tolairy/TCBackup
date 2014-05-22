#ifndef CHUNKDEDUP_H_
#define CHUNKDEDUP_H_


#define CHUNKDEDUPERROR -1
#define CHUNKDEDUPSUCCESS 0

#define NEW_CHUNK 0
#define DEDUP_CHUNK 1

int chunk_dedup(Client *c, FileInfo *fileinfo);

bool calculate_rep_finger(FileInfo *fileinfo);

bool mark_deplicate_chunk(FileInfo *fileinfo, Bin *bin);

bool write_chunk_to_cloud(int fd, int64_t off_set, FingerChunk *fc);

bool update_file_recipe(Client *c, FileInfo *fileinfo);


#endif