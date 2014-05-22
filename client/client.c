#include "global.h"

extern float bandwidth;
/*perfect rewirte*/
extern float segment_usage_threshold;

/*CFL*/
extern float LWM;
extern float default_container_reuse;

/*capping*/
extern int64_t buffer_capacity;
extern int max_refer_count;

enum OPER_TYPE {
	type_simulate,
	type_login = 1,
	type_logout,
	type_register,
	type_backup,
	type_pipeline_backup,
	type_restore,
	type_delete,
	type_quit,
	type_help,
	type_test,
	type_list,
	type_overhead
};

char SERVER_IP[30] = "192.168.1.65";

void test_data(int socket,char *filename)
{
	TIMER_DECLARE(start,end);
	TIMER_DECLARE(Rstart,Rend);
	TIMER_DECLARE(Sstart,Send);
	char  buf[SOCKET_BUF_SIZE+1] = {0};
	int readlen=0;
	double total_time=0;
	double send_time=0;
	double read_time=0;
	double total_len=0;
	int  fd=-1;
	if ((fd=open(filename, O_RDONLY)) < 0) {
		 printf("%s,%d open file error!\n",__FILE__,__LINE__);
		return;
	 }
	TIMER_START(start);
	while(1){
		TIMER_START(Rstart);
		if((readlen=readn(fd, buf, SOCKET_BUF_SIZE))<=0)
			break;
		TIMER_END(Rend);
		TIMER_DIFF(read_time,Rstart,Rend);
		TIMER_START(Sstart);
		bnet_send(socket,buf,readlen);
		TIMER_END(Send);
		TIMER_DIFF(send_time,Sstart,Send);
		total_len+=readlen;
	}
	TIMER_END(end);
	TIMER_DIFF(total_time,start,end);
	close(fd);
	close(socket);
	printf("read time=%.4f  %.4fMB/s\n",read_time,total_len/read_time/1036288);
	printf("send time=%.4f  %.4fMB/s\n",send_time,total_len/send_time/1036288);
	printf("total time=%.4f  %.4fMB/s\n", total_time,total_len/total_time/1036288);
}

void usage(char *program)
{
    fprintf(stderr, "Usage: ./client [OPTION...] [FILE]...\n");
    fprintf(stderr, "Examples:\n");
	fprintf(stderr, "  ./TCBackup_client -S /root/zpj.trace # simulate backup with zpj.trace \n");
    fprintf(stderr, "  ./TCBackup_client -b /root/zpj       # backup file or dirctory \n");
    fprintf(stderr, "  ./TCBackup_client -p -b /root/zpj    # backup file or dirctory with pipeline method \n");
    fprintf(stderr, "  ./TCBackup_client -r 1,/root/zpj/    # restore job 1, restore to directory /root/zpj/ \n");
	fprintf(stderr,	"  ./TCBackup_client -d 1               # delete job 1\n");
	fprintf(stderr, "  ./TCBackup_client -l                 # list available jobs\n");
    fprintf(stderr, "  ./TCBackup_client -t filename        # test the network bandwidth  \n");
	fprintf(stderr, "  -o file                              # output the backup or restore resut to file\n");
	fprintf(stderr, "  -R algorithm                         # algorithm can be \"CFL\" \"CAP\" \"PER\"\n");
	fprintf(stderr, "  --CFL=[CFL]                    \n");
	fprintf(stderr, "  --seg_reuse=[segment reuse for CFL]\n");
	fprintf(stderr, "  --cap_size=[buffer length for capping]\n");
	fprintf(stderr, "  --cap_count=[max refered segment count for capping]\n");
	fprintf(stderr, "  --ref_threshold=[segment ref threshold for Perfect rewrite]\n");
	fprintf(stderr, "  -O bandwidth -b path                 #test time overtime mode\n");
	fprintf(stderr, "  -c                                   # upload data to cloud, data will not be uploaded without this opt\n");
    fprintf(stderr, "  -H host                              # connect the destination host (ip address)  \n");
    fprintf(stderr, "  -h, --help    give this help list\n\n");
    fprintf(stderr, "Report bugs to <hustbackup@126.com>.\n\n");
}

/* create a struct for client to keep user information */
Client *create_client()
{
    Client *c = NULL;
    c = (Client *)malloc(sizeof(Client));
    if(c == NULL)
        return NULL;
    memset(c->username, 0, sizeof(c->username));
    memset(c->password, 0, sizeof(c->password));
    
    c->fd = -1;
    
    c->jcr = NULL;

    return c;
}

/* free client */
void free_client(Client *c)
{
    free(c);
    return;
}

/*main function*/
int main(int argc, char **argv)
{
    int opt;
    enum OPER_TYPE type;
    char path[FILE_NAME_LEN] = {0};
	char output_path[FILE_NAME_LEN] = {0};
    int delete_job;
    Client *c= NULL;
	
    if(argc<2)
  	    return -1;
	struct option long_options[] = {
		{"Simulate", 1, NULL, 'S'},
    	{"quit", 0, NULL, 'q'},
    	{"backup", 1, NULL, 'b'},
    	{"delete", 1, NULL, 'd'},
    	{"restore", 1, NULL, 'r'},
    	{"list", 0, NULL, 'l'},
    	{"pipeline", 0, NULL, 'p'},
    	{"output", 1, NULL, 'o'},
    	{"cloud", 0, NULL, 'c'},
    	
    	{"rewrite", 1, NULL, 'R'},
    	/*CFL*/
    	{"CFL", 1, NULL, 'C'},
    	{"seg_reuse", 1, NULL, 's'},
    	/*capping*/
    	{"cap_size", 1, NULL, 'a'},
    	{"cap_count", 1, NULL, 'e'},
    	/*perferect rewrite*/
    	{"ref_threshold", 1, NULL, 'u'},
    	
		{"overhead", 1, NULL, 'O'},
    	{"test", 1, NULL, 't'},
    	{"server", 1, NULL, 'H'},
    	{"help", 0, NULL, 'h'},
    	{NULL, 0, NULL, 0}
	};
	
    while((opt=getopt_long(argc, argv, "S:qb:d:r:lpo:cR:C:s:a:e:u:O:t:H:h", long_options, NULL))!=-1){
        switch(opt){
			case 'S':
				type=type_simulate;
				SIMULATE = true;
				strncpy(path, optarg, strlen(optarg));
				break;
            case 'q':
		        type=type_quit;
		        break;
            case 'b':
	  	        type=type_backup;
		        strncpy(path,optarg,strlen(optarg));
		        break;
            case 'd':
               	type=type_delete;
		        sscanf(optarg, "%d", &delete_job); 
		        break;
            case 'r':
              	type=type_restore;
		        strncpy(path,optarg,strlen(optarg));
		        break;
			case 'R':
				if (strcmp(optarg, "CFL") == 0)
					REWRITE = CFL_REWRITE;
				else if (strcmp(optarg, "CAP") == 0)
					REWRITE = CAPPING_REWRITE;
				else if (strcmp(optarg, "PER") == 0)
					REWRITE = PERFECT_REWRITE;
				break;
				
			case 'C':
				sscanf(optarg, "%f", &LWM);
				break;
			case 's':
				sscanf(optarg, "%f", &default_container_reuse);
				break;
			case 'a':
				sscanf(optarg, "%lld", &buffer_capacity);
				break;
			case 'e':
				sscanf(optarg, "%d", &max_refer_count);
				break;
			case 'u':
				sscanf(optarg, "%f", &segment_usage_threshold);
				break;
			case 'O':
				OVERHEAD = true;
				sscanf(optarg, "%f", &bandwidth);
				break;
	        case 't':
	  	        type=type_test;
		        strncpy(path,optarg,strlen(optarg));
		        break;
	        case 'h':
	   	        type=type_help;
		        break;
	            break;
            case 'H':
	  	        memset(SERVER_IP,0,30);
	  	        strncpy(SERVER_IP,optarg,strlen(optarg));
	  	        break;
			case 'l':
				type=type_list;
				break;
			case 'p':
				G_PIPELINE = true;
				break;
			case 'o':
				OUTPUT_RESULT = true;
				strncpy(output_path, optarg, strlen(optarg));
				break;
			case 'c':
				CLOUD = true;
				break;
	        default:
	   	        printf("Your command is wrong \n");
		        type=type_help;
		        break;
        }
    }

    if(type==type_help){
   	    usage(argv[0]);
	    return 0;
    }
    c = create_client();
	
	if (type == type_simulate) {
		simulata_backup(c, path, output_path);
		free(c);
		return 0;
	}else if(OVERHEAD == true) {
		backup_overhead(c, path, output_path);
		free(c);
		return 0;
	}
	
	
    if ((c->fd = bnet_connect(SERVER_IP, SERVER_PORT)) == -1){
	    err_msg1("Connection rejected!");
	    return 0;
    }

	if (G_PIPELINE) {
		c->recipe_fd = bnet_connect(SERVER_IP, SERVER_OTH_PORT);
		if (c->recipe_fd == -1) {
			err_msg1("Connection rejected!");
			return 0;
		}
	}
    
    if(type==type_test){
  	    test_data(c->fd, path);
	    return 0;
    }
    switch(type){
	    case type_login:
	        login(c);
	        break;
	    case type_logout:
	        logout(c);
	        break;
	    case type_quit:
            quit_client(c);
            break;
        case type_backup:
			if (G_PIPELINE)
				pipeline_backup(c, path, output_path);
			else
            	backup_client(c, path, output_path);
            break;
        case type_restore:
            restore_client(c, path, output_path);
            break;
        case type_delete:
            delete_client(c, delete_job);
            break;	
		case type_list:
			list_client(c);
			break;
	   default:
	   	    printf("Your command is wrong \n");
		    break;
    }
    free_client(c);
    return 0;
}
