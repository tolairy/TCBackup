#include "../../global.h"
#include "include/bcs_sdk.h"

#ifdef API_TEST_
#include <string.h>
#include <stdbool.h>

//#include "api.h"
#include "../../src/store.h"
#endif


char *ak = "F6a0680645eb65925df4cdedf1f0a804";
char *sk = "E47dfb4f4de3b46b8b4887c06bf7d485";
char bucket[130] = "cumulus";
char *host = "bcs.duapp.com";
bcs_sdk *bcs = NULL;





void init_bcs(){

	bcs = new bcs_sdk(ak, sk, host, "./cloud/baidu/log/bcs.log");
	if(bcs != NULL)
		printf("init bcs complete\n");

}
void free_bcs(){

	if(bcs != NULL){
		delete bcs;
		bcs = NULL;	
	}

}
void get_path(char *name, char *dir, char *path)
{
	strcpy(path, dir);
	strcat(path, name);
	
}

bool put_to_cloud(char *dest, char *source)
{
	response res;
	//char cloud_path[128];
	//char local_path[128];

	//get_path(name, cloud_dir, cloud_path);
	//get_path(name, TmpPath, local_path);

	//if (bcs == NULL)
	//	init_bcs();

	printf("upload %s\n", source);
	int ret = bcs -> put_object(bucket, dest, source, res);
	if (ret == 1) {
		return false;
	}
	else {
		printf("upload %s complete\n", source);
		return true;
	}
}

bool get_from_cloud(char *dest, char *source)
{
		
	response res;
	//char cloud_path[128];
	//char local_path[128];

	//get_path(name, cloud_dir, cloud_path);
	//get_path(name, TmpPath, local_path);
	
	printf("%s, %d, get from cloud %s\n",__FILE__,__LINE__,source);
	
//	if (bcs == NULL)
	//	init_bcs();

	int ret = bcs -> get_object(bucket, source, dest, res);

	printf("%s, %d, get from cloud complete\n",__FILE__,__LINE__);
	if (ret == 1) {
		return false;
	}
	else return true;

}

bool delete_file(char *path)
{

	response res;
	//char cloud_path[128];
	//char local_path[128];

	//get_path(name, cloud_dir, cloud_path);

	//if (bcs == NULL)
	//	init_bcs();

	int ret = bcs -> delete_object(bucket, path, res);
	if (ret == 1) {
		return false;
	}
	else return true;


}

bool list_files(char * object_list)
{
	response res;
	int ret = bcs->list(bucket, res);
	if (ret == 0) {
		object_list = (char *)malloc(res.body.length()+1);
		strcpy(object_list, res.body.c_str());
		printf("list object, object info:[%s]\n", object_list);
		return true;

	}
	else {

		printf("list object failed, error msg:[%s]\n", res.body.c_str());
		return false;
	}


}


/*
int main(){
	printf("%s\n","fuck");
	char test_segment[32] = "12162561777";
	//strcpy(test_segment, "12162561777");
	printf("%s\n", test_segment);
	int res = put_to_cloud(test_segment);
	if (res) printf("%s\n", "success!");
	else printf("%s\n", "fail!!");
	return 1;
}
*/
