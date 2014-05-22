#ifndef RESTORE_H_
#define RESTORE_H_

/*get file list of the job*/
DList *get_job_info(Client *client);

/*restore files*/
int get_files(Client *client, DList *filelist);


#endif