#ifndef PIPELINE_BACKUP_H_
#define PIPELINE_BACKUP_H_

typedef struct buffer_ele{
	void *user_data;
	void *file_data;
}BUFELE;

int pipeline_backup(Client *c, char *path, char *output_path);
int pipeline_file_dedup(Client * c,char * path);



#endif
