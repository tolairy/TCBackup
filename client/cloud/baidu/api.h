#ifndef API_H_
#define API_H_

void init_bcs();
void free_bcs();
bool put_to_cloud(char *dest, char *srouce);
bool get_from_cloud(char *dest, char *source);
/**/
bool list_files(char *filelist);
bool delete_file(char *name);

#endif
