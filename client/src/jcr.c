#include "../global.h"


JCR* jcr_new() 
{
	JCR *psJcr = (JCR*) malloc(sizeof(JCR));

	psJcr->old_size = 0;
	psJcr->file_dedup_size = 0;
	psJcr->chunk_dedup_size = 0;
	psJcr->total_dedup_size = 0;
	psJcr->total_chunk_size = 0;
	
	psJcr->file_count = 0;
	psJcr->dedup_file_count = 0;

	psJcr->chunk_count = 0;
	psJcr->dedup_chunk_count = 0;
	
	psJcr->read_time = 0;
	psJcr->chunk_time = 0;  
	psJcr->sha_time = 0; 
	psJcr->send_time = 0;
	psJcr->search_time = 0;
	psJcr->read2_time = 0;
	
	psJcr->total_time = 0;
	psJcr->file_dedup_time = 0;
	psJcr->recv_chunk_info_time = 0;
	psJcr->send_recipe_time = 0;

	memset(psJcr->backup_path,0,256);
	memset(psJcr->restore_path,0,256);
	return psJcr;
}


void jcr_free(JCR *jcr) 
{
	free(jcr);
}