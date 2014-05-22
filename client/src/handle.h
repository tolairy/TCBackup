#ifndef HANDLE_H_
#define HANDLE_H_

int register_user(Client *client);

int login(Client *client);

int logout(Client *client);

int backup_client(Client *c, char *path, char *output_path);

int restore_client(Client *c, char *path, char *output_path);

int delete_client(Client *c, int job_id);

int quit_client(Client *c);

int list_client(Client *c);

#endif

