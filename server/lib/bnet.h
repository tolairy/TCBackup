#ifndef BNET_H_
#define BNET_H_

typedef struct bnet_sock {
	int sockfd;
	int msglen;
	char msg[SOCKET_BUF_SIZE];
} DSOCK;


int bnet_connect(const char *ip, unsigned short port);

int bnet_open(unsigned short port);
int bnet_accept(int socket_fd,struct sockaddr_in *clientaddr,socklen_t * addrlen);

int readn(int fd, char *vptr, int n);
int writen(int fd, char *vptr, int n);

int bnet_send(int fd, char* msg, int len);
int bnet_recv(int fd, char* msg, int *len);
int bnet_signal(int fd, int sig);
void set_recvbuf_size(int fd, int size);
void set_sendbuf_size(int fd, int size);
void get_socket_default_bufsize(int fd);

#endif /* BNET_H_ */
