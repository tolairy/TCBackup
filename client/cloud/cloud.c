#include "../global.h"

#ifdef CLOUD_TEST_
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "../src/store.h"
#include "cloud.h"
#include "baidu/api.h"
#endif


#define NAME_LENGTH 32

static SyncQueue *upload_queue = NULL;
static SyncQueue *download_queue = NULL;
static pthread_t upload_thread;
static pthread_t download_thread;
float bandwidth = 0;
double upload_time_first = 0;
double upload_time_second = 0;

void * handle_upload(void *arg)
{
	TIMER_DECLARE(start,end);
	TIMER_START(start);
	
	char local_path[FILE_NAME_LEN];
	char cloud_path[FILE_NAME_LEN];
	while (1) {
		char *name = (char *)(sync_queue_pop(upload_queue));
		if (strcmp(name, "end") == 0) {
			free(name);
			break;
		}
		printf("upload %s left %d\n", name, sync_queue_size(upload_queue));
		
		strcpy(local_path, TmpPath);
		strcat(local_path, name);
		strcpy(cloud_path, cloud_dir);
		strcat(cloud_path, name);

		
		if (OVERHEAD) {
			
			struct stat buf;
			stat(local_path, &buf);
			if (upload_queue->queue.last == NULL || strcmp((char *)(((upload_queue->queue).last)->data),"end") != 0){
				usleep(1000*1000*((float)buf.st_size)/(1024*1024*bandwidth));
			} else {
				upload_time_second += ((float)buf.st_size)/(1024*1024*bandwidth);
			}
			free(name);
			continue;
		}

		put_to_cloud(cloud_path, local_path);
		free(name);
	}

	TIMER_END(end);
	TIMER_DIFF(upload_time_first,start,end);
	return NULL;

}
void * handle_download(void *arg)
{
	char local_path[FILE_NAME_LEN];
	char cloud_path[FILE_NAME_LEN];
	while (1) {
		char *name =(char *)sync_queue_pop(download_queue);
		if (strcmp(name, "end") == 0) {
			free(name);
			break;
		}
		
		strcpy(local_path, TmpPath);
		strcat(local_path, name);
		strcpy(cloud_path, cloud_dir);
		strcat(cloud_path, name);
		get_from_cloud(cloud_path, local_path);
		free(name);
	}
	return NULL;
}




void cloud_upload_init()
{
	init_bcs();
	upload_queue = sync_queue_new();
	
	int res = pthread_create(&upload_thread, NULL, handle_upload, NULL);
	if (res != 0) {
		err_msg1( "create upload thread error!");
		return ;
		} 

}

bool cloud_upload(char *name)
{	

	char *buf = (char *)malloc(NAME_LENGTH);
	memset(buf, 0, NAME_LENGTH);
	strcpy(buf, name);
	sync_queue_push(upload_queue, buf);
	return true;
}
void cloud_upload_destory()
{
	char *destory = "end";
	cloud_upload(destory);
	pthread_join(upload_thread, NULL);
	sync_queue_free(upload_queue);
	free_bcs();

}
void cloud_download_init()
{
	init_bcs();
	download_queue = sync_queue_new();

	int res = pthread_create(&download_thread, NULL, handle_download, NULL);
	if (res != 0) {
		err_msg1( "create upload thread error!");
		return ;
	} 
}

bool cloud_download(char *name)
{

	char *buf = (char *)malloc(NAME_LENGTH);
	memset(buf, 0, NAME_LENGTH);
	strcpy(buf, name);
	sync_queue_push(download_queue, buf);
	return true;
}

void cloud_download_destory()
{
	char *destory = "end";
	cloud_download(destory);
	pthread_join(download_thread, NULL);
	sync_queue_free(download_queue);
	free_bcs();

}


#ifdef CLOUD_TEST
int  main()
{
	segment_name segment;
	strcpy(segment, "12162561777");
	delete_file(segment);
	//terminate_cloud_download();
}

#endif

