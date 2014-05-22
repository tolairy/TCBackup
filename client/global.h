/*
 * global.h
 *
 *  Created on: 6.24, 2013
 *      Author: Yifan Yang
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <dirent.h>
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
#include <mcheck.h>
#include <getopt.h>



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

extern char BinPath[];
extern char TmpPath[];
extern char cloud_dir[];
extern char static_path[];

extern bool SEG_STATISTICS;
extern bool G_PIPELINE;
extern bool CLOUD;
extern bool OUTPUT_RESULT;
extern int REWRITE;
extern bool SIMULATE;
extern bool OVERHEAD;

extern int restore_cache_size;

#define NO_REWRITE 0
#define CFL_REWRITE 1
#define PERFECT_REWRITE 2
#define CAPPING_REWRITE 3

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

/*turn on the rewrite function when REWRITE is 1*/
 


#include "lib/bnet.h"
#include "lib/sha1.h"
#include "lib/htable.h"
#include "lib/dlist.h"
#include "lib/queue.h"
#include "lib/rabin.h"
#include "lib/libz.h"
#include "lib/zmalloc.h"
#include "lib/lru_cache.h"

#include "src/jcr.h"
#include "client.h"
#include "src/container.h"
#include "src/recipe.h"
#include "src/filededup.h"
#include "src/handle.h"
#include "src/container_cache.h"
#include "src/simulate.h"
#include "src/pipeline_backup.h"
#include "src/overhead_test.h"
#include "src/backup.h"
#include "src/extreme_binning.h"
#include "src/chunkdedup.h"
#include "src/cfl.h"
#include "src/perfect_rewrite.h"
#include "src/capping.h"
#include "src/read_trace.h"

#include "src/store.h"
#include "src/restore.h"

#include "cloud/cloud.h"
#include "cloud/baidu/api.h"

#define err_msg1(s)   err_msg(__FILE__,__LINE__,s)
#define err_msg2(s1,s2)  err_msg(__FILE__,__LINE__,s1,s2)
#define err_msg3(s1,s2,s3)  err_msg(__FILE__,__LINE__,s1,s2,s3)
#define err_msg4(s1,s2,s3,s4)  err_msg(__FILE__,__LINE__,s1,s2,s3,s4)

#define TIMER_DECLARE(start,end) struct timeval start,end
#define TIMER_START(start) gettimeofday(&start, NULL)
#define TIMER_END(end) gettimeofday(&end, NULL)
#define TIMER_DIFF(diff,start,end) (diff)+=(end.tv_usec-start.tv_usec)*1.0/1000000+(end.tv_sec-start.tv_sec)
#define TIMER_AVG(avg,total,count) avg=(total)*1.0/count


int err_msg (char *filename,int line,const char * fmt, ...);


#endif 
/* GLOBAL_H_ */
