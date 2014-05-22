#ifndef JCR_H_
#define JCR_H_

typedef struct jcr {
	int64_t id;
	int64_t old_size;
	int64_t file_dedup_size;
	int64_t chunk_dedup_size;
	int64_t total_dedup_size;
	int64_t total_chunk_size;

    int64_t file_count;
    int64_t dedup_file_count;
	
	int64_t chunk_count;
	int64_t dedup_chunk_count;

	double read_time;    
	double chunk_time;  
	double sha_time;     
	double search_time; 
	double read2_time;  
	double send_time;
	
	double total_time;
	double file_dedup_time;
	double recv_chunk_info_time;
	double send_recipe_time;
	
	char backup_path[256];
	char restore_path[256];
}JCR;

JCR* jcr_new();

void jcr_free(JCR *jcr);

#endif /* JCR_H_ */

