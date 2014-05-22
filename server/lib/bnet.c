#include "../global.h"

/* server listen number */
#define LISTEN_NUM 20

int bnet_connect(const char *ip, unsigned short port)
{
    struct sockaddr_in serveraddr;
    int socket_fd = 0;
    int ret = 0;
    fd_set wtfds;

    if (SIG_ERR == (signal(SIGPIPE, SIG_IGN)))
    {
        printf("signal SIGPIPE");
        return -1;
    }

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket error");
        close(socket_fd);
        return -1;
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_port = htons(port);
    serveraddr.sin_family = AF_INET;

    if (!inet_aton(ip, (struct in_addr *) &serveraddr.sin_addr.s_addr))
    {
        printf("bad IP address format");
        close(socket_fd);
        return -1;
    }

    ret = connect(socket_fd, (struct sockaddr *) &serveraddr,sizeof(serveraddr));

    if (-1 == ret)
    {
        if (EINTR == errno || EALREADY == errno)
        {

            FD_ZERO(&wtfds);
            FD_SET(socket_fd, &wtfds);

            ret = select(socket_fd + 1, NULL, &wtfds, NULL, NULL);
            if (-1 == ret)
            {
                if (EINTR == errno)
                {
                    FD_ZERO(&wtfds);
                    FD_SET(socket_fd, &wtfds);
                }
                else
                {
                    printf("select error");
                    close(socket_fd);
                    return -1;
                }

            }
        }
        else
        {
            printf("connect error");
            return -1;
        }
    }

    printf("已于服务器(%s:%d)连上\n", inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port));

    return socket_fd;
}

int bnet_open(unsigned short port)
{
    struct sockaddr_in a;
    int s;
    int yes;

    if (SIG_ERR == (signal(SIGPIPE, SIG_IGN)))
    {
        printf("signal SIGPIPE");
        return -1;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket error");
        return -1;
    }
    yes = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,(char *)&yes, sizeof(yes)) < 0)
    {
        printf("setsockopt error");
        close(s);
        return -1;
    }

    memset(&a, 0, sizeof(a));
    a.sin_port = htons(port);
    a.sin_family = AF_INET;
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) < 0)
    {
        printf("bind error");
        close(s);
        return -1;
    }

    printf("accepting connections on port %d\n", port);
    listen(s, LISTEN_NUM);
    return s;
}

int bnet_accept(int socket_fd,struct sockaddr_in *clientaddr,socklen_t * addrlen)
{
    int client_fd = 0;
    while ((-1 == (client_fd = accept(socket_fd, (struct sockaddr *) clientaddr, addrlen))) && (EINTR == errno));
    if (-1 == client_fd)
    {
        printf("accept error");
        return -1;
    }

    return client_fd;
}

int readn(int fd, char *vptr, int n)
{
   int    nleft;
    int   nread;
    char    *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) 
	{
        if ( (nread = read(fd, ptr, nleft)) < 0) 
		{
            if (errno == EINTR)
                nread = 0;
            else
                return(ERROR); 
        } else if (nread == 0)
            break;  
        nleft -= nread;
        ptr += nread;
    }
    return(n - nleft); 
}

int writen(int fd, char *vptr, int n)
{
    int        nleft;
    int       nwritten;
    const char    *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) 
	{                                        
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) 
		{
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0; 
            else
                return(ERROR);
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return(n);
}
 
int bnet_send(int fd, char* msg, int len) 
{
	int net_order_size = htonl(len);
	int wn=0;
	if (writen(fd, (char *)&net_order_size, sizeof(int)) != sizeof(int)) {
		printf("%s,%d failed to send msg length!\n",__FILE__,__LINE__);
	    return ERROR;
	}
	wn=writen(fd,msg,len);
	if(wn!=len)
		printf("%s,%d writen is wrong(%d !=%d )\n",__FILE__,__LINE__,wn,len);
	return wn;
}
 
 /* you must ensure the msg has enough space to contain the data bytes */
int bnet_recv(int fd,char *msg, int *len) {
	int host_order_size;
	int rn=0;
	if (readn(fd,  (char *)&host_order_size,sizeof(int)) != sizeof(int)) {
		printf("%s,%d failed to recv msg length or shutdown by other side!\n",__FILE__,__LINE__);
		return ERROR;
	}
	host_order_size=ntohl(host_order_size);
	if (host_order_size <=0) {
		*len =host_order_size;
		//printf("recv an end signal!\n");
		return host_order_size;
	}
	*len=host_order_size;
	rn=readn(fd,msg,*len);
	if(rn==ERROR)
		return ERROR;
	if(rn !=*len)
		printf("%s,%d readn is wrong\n",__FILE__,__LINE__);
	msg[rn]=0;
	return rn;
}


int bnet_signal(int fd, int sig) {
	int net_order_size = htonl(sig);
    if (writen(fd,  (char *)&net_order_size, sizeof(int)) != sizeof(int)) {
        printf("failed to send signal!\n");
        return ERROR;
    }
    return 0;
}

void set_recvbuf_size(int fd, int size)
{
	
	if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size)) < 0)
		err_msg1("set recv buffer wrong");
}

void set_sendbuf_size(int fd, int size)
{
	
	if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size)) < 0)
		err_msg1("set send buffer wrong");
}

void get_socket_default_bufsize(int fd)
{
	int snd_size = 0;
	int rcv_size = 0;
	socklen_t optlen;
	int err=0;
	optlen = sizeof(snd_size); 
    err = getsockopt(fd, SOL_SOCKET, SO_SNDBUF,(char *)&snd_size, &optlen); 
	if(err<0){ 
		err_msg1("get system socket buffer wrong"); 
	}
	 
	optlen = sizeof(rcv_size); 
	err = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_size, &optlen); 
	if(err<0){ 
		err_msg1("get system socket buffer wrong"); 
	} 
  	printf(" default socket send buffer: %d \n",snd_size); 
	printf(" default socket recv buffer: %d \n",rcv_size); 
}


