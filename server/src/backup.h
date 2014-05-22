#ifndef BACKUP_H_
#define BACKUP_H_

bool file_dedup_mark(DedupClient *c, Recipe *rp);
bool send_chunk_info(DedupClient *c, Recipe *rp);
bool recv_deplicate_chunk(DedupClient *c, Recipe *rp, int32_t chunknum);



#endif /*BCAKUP_H_*/

