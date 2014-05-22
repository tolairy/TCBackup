#ifndef CLIENT_H_
#define CLIENT_H_

/* define the length of username */
#define USERNAMELENGTH 50

/* define the length of password */
#define PASSWORDLENGTH 50

typedef struct dedupClient{
    uint64_t user_id;
    char username[USERNAMELENGTH];
    char password[PASSWORDLENGTH];
    DedupDb *db;
	int client_fd;
	JCR *jcr;
}DedupClient;


DedupClient *create_client(int fd);
void free_client(DedupClient *c);
int client_match(void *ptr, void *key);


#endif /*CLIENT_H_*/

