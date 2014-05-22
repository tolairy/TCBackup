
#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>
#include <signal.h>
#include <alloca.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>
#include <linux/sem.h>
#include <netinet/tcp.h> 
#include <openssl/sha.h>
#include <mysql/mysql.h>
#include <limits.h>
#include <mcheck.h>


extern char command_err[];
extern char database_err[];

extern char login_cmd[];
extern char login_ok[];
extern char login_fail1[];
extern char login_fail2[];
extern char login_fail3[];

extern char register_cmd[];
extern char register_ok[];
extern char register_fail[];

extern char logout_cmd[];
extern char logout_ok[];
extern char logout_fail1[];
extern char logout_fail2[];

extern char backup_cmd[];
extern char backup_ok[];
extern char backup_fail[];

extern char update_attr[];
extern char updateattr_ok[];
extern char updateattr_fail[];

extern char restore_cmd[];
extern char restore_msg[]; 

extern char delete_cmd[];
extern char delete_ok[];
extern char delete_fail[];

extern char list_cmd[];
extern char list_jobinfo[];
extern char list_none[];

extern char store_path[];

extern int G_PIPELINE;
extern int REWRITE;
extern int recipe_fd;

#define bool int
#define false 0
#define true 1

/* compute the total time and average time*/
#define TIMER_DECLARE(start,end) struct timeval start,end
#define TIMER_START(start) gettimeofday(&start, NULL)
#define TIMER_END(end) gettimeofday(&end, NULL)
#define TIMER_DIFF(diff,start,end) (diff)+=(end.tv_usec-start.tv_usec)*1.0/1000000+(end.tv_sec-start.tv_sec)
#define TIMER_AVG(avg,total,count) avg=(total)*1.0/count

#define P(mutex) pthread_mutex_lock(&mutex)
#define V(mutex) pthread_mutex_unlock(&mutex)

/* socket bucket size */
#define SOCKET_BUF_SIZE  64*1024



#include "lib/bnet.h"
#include "lib/zmalloc.h"
#include "lib/htable.h"
#include "lib/threadq.h"
#include "lib/queue.h"
#include "lib/adlist.h"
#include "lib/bloom.h"
#include "lib/htable.h"
#include "lib/signal.h"

#include "server.h"

#include "database/mysql.h"
#include "database/db.h"
#include "database/mysql_find.h"
#include "database/mysql_delete.h"

#include "src/jcr.h"
#include "src/client.h"
#include "src/recipe.h"
#include "src/backup.h"
#include "src/restore.h"
#include "src/pipeline_backup.h"
#include "src/dedupHandle.h"
#include "src/index.h"



#define err_msg1(s)   err_msg(__FILE__,__LINE__,s)
#define err_msg2(s1,s2)  err_msg(__FILE__,__LINE__,s1,s2)
#define err_msg3(s1,s2,s3)  err_msg(__FILE__,__LINE__,s1,s2,s3)
#define err_msg4(s1,s2,s3,s4)  err_msg(__FILE__,__LINE__,s1,s2,s3,s4)

int err_msg (char *filename,int line,const char * fmt, ...);
 
#endif
/* GLOBAL_H_ */
