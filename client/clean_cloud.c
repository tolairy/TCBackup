#include "global.h"
# include "cloud/json/include/value.h"
# include "cloud/json/include/reader.h"
using namespace std;

int  main()
{
	char *files;
    Json::Reader reader;
    Json::Value value;
	init_bcs();
	if(list_files(files)){
		string filelist = files;
		printf("%s\n", filelist.c_str());
		if (reader.parse(filelist, value)) {
			Json::Value object_list = value["object_list"];
			int i, size;
			size = object_list.size();
			for (i = 0; i<size; i++) {

				string object = object_list[i]["object"].asString();
				string parent_dir = object_list["parent_dir"].asString();
				printf("%s\n", object.c_str());
				if (parent_dir == "\/TCBackupTest\/") {
					

				}
			}
		
		}
		//free(filelist);
	}
    free_bcs();
}
