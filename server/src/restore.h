#ifndef RESTORE_H_
#define RESTORE_H_

int send_job_info(DedupClient *client, Queue *filelist);
int get_file_info(DedupClient *client, Recipe *recipe);


#endif
