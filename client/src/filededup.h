#ifndef FILEDEDUP_H_
#define FILEDEDUP_H_

#define FILEDEDUPERROR -1
#define FILEDEDUPSUCCESS 0

#define NEW_FILE 0
#define DEDUP_FILE 1


int file_dudup(Client *c, char *path);

int is_new_file(Client *c, FileInfo *fileinfo);

int recv_deplicate_chunk(Client *c, FileInfo *fileinfo);


#endif /* RECIPE_H_ */

