#include "global.h"

DedupServer *server;

workq_t g_client_wq;
int g_client_count=20;

int G_TEST_DATA=0;
int G_Sanitization=0;


Command ClientCmds[] = {
	{"LOGIN", user_login},
	{"LOGOUT", logout},
	{"REGISTER", register_user},
	{"BACK", new_backup},
	{"REST", restore},
	{"DELE", delete_job},
	{"LIST", list_job},
	{NULL, NULL}
};


int main(int argc, char **argv)
{
	int opt;
	int len;
	pthread_t recipe_thread;
	
	init_server();
    while((opt=getopt(argc, argv, "s:S:tvhopc:C:f:T:P:RDd:"))!=-1) {
	    switch(opt) {
	     	/* set the path to store data */
			case 'P':
				len=strlen(optarg);
			 	memcpy(store_path, optarg, len+1);
			       if(store_path[len-1]!='/'){
					store_path[len]='/';
					store_path[len+1]=0;
				}
				printf("Path=%s\n", store_path);
				break;
			/* segment clean */
			case 'D':
			    G_Sanitization=1;
				break;
			/* test */
			case 't':
			    G_TEST_DATA=1;
			    break;
			/* use information */
			case 'h':
				usage();
				return 0;
			case 'p':
				G_PIPELINE = 1;
				break;
			default:
				printf("Your command is wrong \n");
			    usage();
			    return 0;
	    }
    }
    /* segment clean */     
/*	if(G_Sanitization) {
		sanitization();
		return 0;
	
	}
*/
	/* init index */
	index_init();
	/* init recipe */
	recipe_volume_init();

//       InitSignals(TerminateClose);

	if(G_TEST_DATA){
		bnet_thread_server(&g_client_wq, g_client_count, server->port, test_data);
		return 0;
	}
	make_dir();

	if(G_PIPELINE)
		pthread_create(&recipe_thread, NULL, wait_conncet, NULL);
	
	bnet_thread_server(&g_client_wq, g_client_count, server->port, handle_client_request);

    recipe_volume_flush();
    index_close();

	if (G_PIPELINE) 
		pthread_join(recipe_thread, NULL);
	
	return 0;
}

bool init_server()
{
       server = (DedupServer *)malloc(sizeof(DedupServer));
	server->main_thread = pthread_self();
	server->port = SERVER_PORT;
	server->max_clients = MAX_CLIENT;	
	server->stat_num_connections = 0;
	
	server->clients = listCreate();
	server->clients->match = client_match;
       pthread_mutex_init(&server->server_mutex, NULL);

	return true;
}

int bnet_thread_server(workq_t *client_wq, int max_client_count, int port, void *handle_client_request(void * data)) 
{	
	struct sockaddr_in s_addr;
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = INADDR_ANY;
	s_addr.sin_port = htons(port);

	int newsockfd, fd;
	socklen_t clilen;
	struct sockaddr cli_addr;
	int reuse = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	
	if (bind(fd, (struct sockaddr*) &s_addr, sizeof(struct sockaddr))!= 0) {
			printf("Can not bind address!\n");
			return -1;
	}
	if (listen(fd, 20) != 0) {
		printf("Can not listen the port!\n");
		return -1;
	}
       workq_init(client_wq, max_client_count, handle_client_request);
	for (;;) {
		unsigned int maxfd = 0;
		fd_set sockset;
		FD_ZERO(&sockset);

		FD_SET((unsigned) fd, &sockset);
		maxfd = maxfd > (unsigned) fd ? maxfd : fd;
		errno = 0;
		if (select(maxfd + 1, &sockset, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;
			close(fd); //error
			break;
		}
		if (FD_ISSET(fd, &sockset)) {
			/* Got a connection, now accept it. */
			do {
				clilen = sizeof(cli_addr);
				newsockfd = accept(fd, &cli_addr, &clilen);
			} while (newsockfd < 0 && errno == EINTR);
			if (newsockfd < 0) {
				continue;
			}
			printf("%s %d connectd fd:%d\n",__FILE__,__LINE__,newsockfd);
			workq_add(client_wq,(void *)&newsockfd,NULL,0);	
		}

	}
	workq_destroy(client_wq);
	return 0;
}

void *handle_client_request(void *data)
{
	int fd = *(int *)data;
	bool quit = false, found = false;
	char msg[255];
	int len = 0;
	int i;
	DedupClient *client = NULL;
	
    client = create_client(fd);
    if(client == NULL) {
    	close(fd);
    	return NULL;
    }

	for(quit = false; !quit;) {
		memset(msg, 0, sizeof(msg));
		if(bnet_recv(client->client_fd, msg, &len) == ERROR) {
			printf("%s,%d:client %s  force quit\n", __FILE__,__LINE__,client->username);
			break;
		}
		if(len <= 0)
			break;
		for (i = 0; ClientCmds[i].cmd; i++) {
			if (strncmp(ClientCmds[i].cmd, msg, strlen(ClientCmds[i].cmd)) == 0) {
				found = true;
				printf("%s,%d:Executing %s command.\n", __FILE__, __LINE__, ClientCmds[i].cmd);
				if(i == 3 && G_PIPELINE)
					pipeline_backup(client, msg);
				else
					ClientCmds[i].func(client, msg);
				quit = true;
				break;
			}
		}
		if(!found) {
            printf("%s,%d:the command is invalid\n", __FILE__, __LINE__);
            bnet_send(client->client_fd, command_err, strlen(command_err));
            continue;
		}
	}
	free_client(client);
	close(fd);
	return NULL;
}

void make_dir()
{
	char *p = NULL;
	char *q = store_path+1;
	while ((p = strchr(q, '/'))) {
		if (*p == *(p - 1)) {
			q++;
			continue;
		}
		*p = 0;
		if (access(store_path, 0) != 0) {
			mkdir(store_path, S_IRWXU | S_IRWXG | S_IRWXO);
		}
		*p = '/';
		q = p + 1;
	}
}

void usage()
{
	printf("\n===========usage============\n");
	printf("-p             # pipeline \n");
	printf("-s number      # recv fingerprints number per transmittion \n");
	printf("-v             #display filenames sent or recieved \n");
	printf("-t             # test network bandwidth \n");
	printf("-h             # give this help list \n");
}

void TerminateClose(int sig) 
{
      recipe_volume_flush();
      index_close();
      printf("%s%d: In terminate_stored() sig=%d\n", __FILE__, __LINE__, sig);
      exit(sig);
}

void *wait_conncet(void * arg){
	pthread_detach(pthread_self());
	struct sockaddr_in s_addr;
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = INADDR_ANY;
	s_addr.sin_port = htons(SERVER_OTH_PORT );

	int newsockfd=-1, fd=-1;
	socklen_t clilen;
	struct sockaddr cli_addr;/* ¿Í»§¶Ëaddress */
	//int reuse = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	//setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	if (bind(fd, (struct sockaddr*) &s_addr, sizeof(struct sockaddr))!= 0) {
			printf("Can not bind address!\n");
			return NULL;
	}
	if (listen(fd, 5) != 0) {
		printf("Can not listen the port!\n");
		return NULL;
	}
    	
	for (;;) {
		unsigned int maxfd = 0;
		fd_set sockset;
		FD_ZERO(&sockset);

		FD_SET((unsigned) fd, &sockset);
		maxfd = maxfd > (unsigned) fd ? maxfd : fd;
		errno = 0;
		if (select(maxfd + 1, &sockset, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;
			close(fd); //error
			break;
		}
		if (FD_ISSET(fd, &sockset)) {
			/* Got a connection, now accept it. */
			do {
				clilen = sizeof(cli_addr);
				newsockfd = accept(fd, &cli_addr, &clilen);
			} while (newsockfd < 0 && errno == EINTR);
			if (newsockfd < 0) {
				continue;
			}
			recipe_fd = newsockfd;
			printf("%s %d connectd fd:%d\n",__FILE__,__LINE__,recipe_fd);	
		}
	}
	//err_msg1("wrong socket  connect");
	return NULL;
}


void* test_data(void *arg)
{
	char  buf[BUF_DEFAULT_SIZE+1];
	int len=0;
	double total_time=0;
	double total_len=0;
	int  fd=-1;
	int socket=*(int *)arg;
	if((fd = open("./test_bandwidth", O_CREAT | O_TRUNC | O_WRONLY,00644))<0){
		 printf("%s,%d create  file error!\n",__FILE__,__LINE__);
		return NULL;
	 }
	TIMER_DECLARE(start, end);
	TIMER_START(start);
	while(bnet_recv(socket,buf, &len)>0){
		//writen(fd,buf,len);
		total_len+=len;
	}
	TIMER_END(end);
	TIMER_DIFF(total_time,start,end);
	close(fd);
	close(socket);
	printf("total time=%.4f  %.4fMB/s\n",total_time,total_len/total_time/1036288);
	return NULL;
}
