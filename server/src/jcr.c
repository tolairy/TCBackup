#include "../global.h"


JCR* jcr_new() 
{
	JCR *psJcr = (JCR*) malloc(sizeof(JCR));

	psJcr->job_id = 0;
	psJcr->object_id = 0;

	memset(psJcr->backup_path,0,FILE_NAME_LEN);
	memset(psJcr->restore_path,0,FILE_NAME_LEN);

	psJcr->file_count=0;
	psJcr->total_size=0;
	psJcr->dedup_size=0;

	
	psJcr->searchTime=0;
	psJcr->writeDataTime=0;
	psJcr->writeRecipeTime=0;
	psJcr->recvTime=0;
	psJcr->totalTime=0;
	return psJcr;
}

void jcr_free(JCR* jcr) {
	free(jcr);
}

