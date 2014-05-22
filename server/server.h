
#ifndef SERVER_H_
#define SERVER_H_


/* define default buf size */
#define BUF_DEFAULT_SIZE 64*1024

/* server port */
#define SERVER_PORT 8888
#define SERVER_OTH_PORT 8889

/* define max clients */
#define MAX_CLIENT 1000


/* define finger length */
#define FINGER_LENGTH 20

/* define the length of chunk address */
#define CHUNK_ADDRESS_LENGTH 64

/* define the length of filename */
#define FILE_NAME_LEN 256


#define SUCCESS 1
#define FAILURE 0
#define ERROR -100

#define FILE_HASH 1
#define FILE_RECIPE 2

#define HASH_END -1
#define RECIPE_END -2

#define STREAM_END -6


/* define the type of fingerprint */
typedef unsigned char Fingerprint[FINGER_LENGTH];

/* define the type of chunk address */
typedef unsigned char Chunkaddress[CHUNK_ADDRESS_LENGTH]; 

typedef struct CCmds{
	const char *cmd;
	int (*func)(void *data, void *msg);
}Command;

/* define the struct of server */
typedef struct dedupServer{
	pthread_t main_thread;
	int port;
	list *clients;
	unsigned int max_clients;
	int stat_num_connections;
	
	pthread_mutex_t server_mutex;
}DedupServer;

extern DedupServer *server;


/* functions */
void *wait_conncet(void *arg);
void* test_data(void *arg);
void usage();
bool init_server();
void make_dir();
void *handle_client_request(void *data);
int  bnet_thread_server(workq_t *client_wq, int max_client_count, int port, void *handle_client_request(void * data));
void TerminateClose(int sig);

#endif 
/* SERVER_H_ */
