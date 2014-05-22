#include "global.h"

char command_err[] = "0001 Command is invalid or error";
char database_err[] = "0002 Database error";

char login_cmd[] = "LOGIN username=%s password=%s";
char login_ok[] = "1000 login success";
char login_fail1[] = "1001 fail login:username has not existed";
char login_fail2[] = "1002 fail login:password does not match with the username";
char login_fail3[] = "1003 fail login:username has loged on";

char register_cmd[] = "REGISTER username=%s password=%s";
char register_ok[] = "2000 register ok";
char register_fail[] = "2001 register fail:username has existed";

char logout_cmd[] = "LOGOUT username=%s";
char logout_ok[] = "3000 logout ok";
char logout_fail1[] = "3001 logout fail:username has quited";
char logout_fail2[] = "3002 logout fail:username has not existed";


char backup_cmd[] = "BACK fileset=%s";
char backup_ok[] = "4001 backup ok";
char backup_fail[] = "4002 backup fail";


char update_attr[] = "UPDATEATTR chunknum=%d filepath=%s";
char updateattr_ok[] = "5001 update attr ok";
char updateattr_fail[] = "5002 update attr fail";

char restore_cmd[] = "REST jobid=%d";
char restore_msg[] = "RMSG %s"; 

char delete_cmd[] = "DELE jobid=%d";
char delete_ok[] = "6001 delet job success";
char delete_fail[] = "6002 delet job error";

char list_cmd[] = "LIST";
char list_job[] = "JobId=%s FileSet=%s";
char list_none[] = "NO JOB AVAILABLE";


char BinPath[] = "./bin/";
char TmpPath[] = "./tmp/";
char cloud_dir[] = "/TCBackupTest/";

char static_path[] = "./tmp/static_output.txt";

bool SEG_STATISTICS = true;
bool G_PIPELINE = false;
bool CLOUD = false;
bool OUTPUT_RESULT = false;
bool SIMULATE = false;
//int REWRITE = CFL_REWRITE;
int REWRITE = NO_REWRITE;
//int REWRITE = PERFECT_REWRITE;
bool OVERHEAD = false;

static char buf[1024];

int restore_cache_size = 4;


int err_msg (char *filename,int line,const char *fmt, ...)
{
    va_list args;
    int n=0;
    printf("\033[40;31m %s,%d Error:",filename,line);
    va_start(args, fmt);
    n=vsprintf(buf, fmt, args);
    buf[n]='\0';
    printf(buf);
    va_end(args);
    printf("\033[0m \n");
    return n;
}
 
 
