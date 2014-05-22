#ifndef JCR_H_
#define JCR_H_

typedef struct jcr {
	int64_t job_id;
	int64_t object_id;
	char backup_path[FILE_NAME_LEN];
	char restore_path[FILE_NAME_LEN];
	int32_t file_count;

	int64_t total_size;
	int64_t dedup_size;
	
	double searchTime; 
	double writeDataTime;
	double writeRecipeTime; 
	double recvTime;
	double totalTime;
}JCR;

JCR* jcr_new() ;
void jcr_free(JCR* jcr) ;


#endif /* JCR_H_ */

