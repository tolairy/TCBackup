#include "../global.h"

double read_time = 0;

int  walk_dir(Client *c, char *path) 
{
	struct stat state;
	if (stat(path, &state) != 0) {
		err_msg1("file does not exist! ignored!");
		return 0;
	}
	if (S_ISDIR(state.st_mode)) {
		DIR *dir = opendir(path);
		struct dirent *entry;
		char newpath[512];
		memset(newpath,0,512);
		if (strcmp(path + strlen(path) - 1, "/")) {
			strcat(path, "/");
		}

		while ((entry = readdir(dir)) != 0) {
			/*ignore . and ..*/
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))//(entry->d_name[0]=='.')
				continue;
			strcpy(newpath, path);
			strcat(newpath, entry->d_name);
			if (walk_dir(c, newpath) != 0) {
				return -1;
			}
		}
		//printf("*** out %s direcotry ***\n", path);
		closedir(dir);
	} 
	else if (S_ISREG(state.st_mode)) {
		if (G_PIPELINE) {
			//printf("%s,%d\n",__FILE__, __LINE__);
			pipeline_file_dedup(c, path);
		}
		else
			file_dudup(c, path);

	} else {
		err_msg1("illegal file type! ignored!");
		return 0;
	}
	return 0;
}

int chunk_file(FileInfo *fileinfo) 
{
	
	int subFile;
    int32_t srclen=0, left_bytes = 0;
    int32_t size=0,len=0; 
    int32_t n = MAX_CHUNK_SIZE;
	int64_t offset = 0;

	
	unsigned char *p;
	FingerChunk *fc;
    unsigned char *src = (unsigned char *)malloc(MAX_CHUNK_SIZE*2);	
	//SHA1Context ctx;
	
	//SHA1Init(&ctx);
    chunk_alg_init();
	if(src == NULL) {
        err_msg1("Memory allocation failed");
		return FAILURE;
    }

	if ((subFile = open(fileinfo->file_path, O_RDONLY)) < 0) {
	    printf("%s,%d: open file %s error\n",__FILE__, __LINE__, fileinfo->file_path);
		free((char*)src);
		return FAILURE;
	}

	while(1) 
	{
		TIMER_DECLARE(start,end);
		TIMER_START(start);
	 	if((srclen = readn(subFile, (char *)(src+left_bytes), MAX_CHUNK_SIZE)) <= 0)
		    break;
		TIMER_END(end);
		TIMER_DIFF(read_time,start,end);
		if (left_bytes > 0){ 
			srclen += left_bytes;
			left_bytes = 0;
		} 

		if(srclen < MIN_CHUNK_SIZE)
		 	break;
		
		p = src;
		len = 0;
		while (len < srclen) 
		{
          	n = srclen -len;
			size = chunk_data(p, n);
			if(n==size && n < MAX_CHUNK_SIZE)
			{ 	
          		memmove(src, src+len, n );
          		left_bytes = n;
                break;
			}  
      			
			fc = fingerchunk_new();
			chunk_finger(p, size, fc->chunk_hash);
			//SHA1Update(&ctx, p, size);
			
			fc->chunklen = size;
			fc->offset = offset;
			file_append_fingerchunk(fileinfo, fc);
			
			
			offset += size;
			p = p + size;
			len += size;
		}
    }
	
	/******more******/
	len = 0;
	if(srclen > 0)
	    len=srclen;
	else if(left_bytes > 0)
	 	len=left_bytes;
	if(len > 0){
	 	p= src;
	 	fc = fingerchunk_new();
		chunk_finger(p, len, fc->chunk_hash);
		//SHA1Update(&ctx, p, len);
		fc->chunklen = len;
		fc->offset = offset;
		file_append_fingerchunk(fileinfo, fc);
		
	 }	
	//SHA1Final(&ctx, fileinfo->file_hash);
	 close(subFile);
   	 free(src);
   	 return SUCCESS;
}