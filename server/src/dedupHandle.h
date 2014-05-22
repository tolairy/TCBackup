#ifndef DEDUPHANDLE_H
#define DEDUPHANDLE_H

#define DEDUPHANDLETRUE 0
#define DEDUPHANDLEFAIL -1

int register_user(void *data, void *msg);
int user_login(void *data, void *msg);
int logout(void *data, void *msg);
int new_backup(void *data, void *msg);
int restore(void *data, void *msg);
int delete_job(void *data, void *msg);
int list_job(void *data, void *msg);

#endif /*DEDUPHANDLE_H*/

