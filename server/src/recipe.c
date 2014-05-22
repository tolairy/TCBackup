#include "../global.h"

const static int32_t magic_key = 0xa1b2c3d4;//bin volume validation
static RecipeVolume *rv;


FingerChunk* fingerchunk_new(Fingerprint hash, Chunkaddress address)
{
	FingerChunk *fc = (FingerChunk *)malloc(sizeof(FingerChunk));
	memcpy(fc->chunk_hash, hash, sizeof(Fingerprint));
	memcpy(fc->chunk_address, address, sizeof(Chunkaddress));
	fc->next = NULL;
	return fc;
}

void fingerchunk_free(FingerChunk *fc)
{
	free(fc);
}

Recipe* recipe_new() 
{
	Recipe *rp = (Recipe *)malloc(sizeof(Recipe));
	rp->chunknum = 0;
	rp->first = NULL;
	rp->last = NULL;
	rp->is_new = -1;
	memset(rp->filename, 0, FILE_NAME_LEN);
	memset(rp->file_hash, 0, sizeof(Fingerprint));
	return rp;
}

void recipe_free(Recipe* trash)
{
    FingerChunk *fchunk = trash->first;
    while(fchunk) {
        trash->first = fchunk->next;
 		free(fchunk);
        fchunk = trash->first;
    }

	free(trash);
}

Recipe* recipe_append_fingerchunk(Recipe *recipe, FingerChunk *fchunk) 
{
    fchunk->next = NULL;
	if (recipe->first == NULL) {
		recipe->first = fchunk;
	}else {
       	recipe->last->next = fchunk;
        }
        recipe->last = fchunk;
	recipe->chunknum++;
	return recipe;
}


bool recipe_volume_init()
{
    int fd;
    char path[FILE_NAME_LEN] = {0};
    int32_t key;
    
    rv = (RecipeVolume*)malloc(sizeof(RecipeVolume));
    rv->file_num = 0;
    rv->volume_length = recipe_volume_head_size;
    pthread_mutex_init(&rv->mutex, 0);

    strcpy(path, store_path);
	strcat(path, "recipe");

    if((fd = open(path, O_CREAT|O_RDWR, S_IRWXU))<0){
        printf("%s, %d, failed to open recipe volume %s", __FILE__, __LINE__, path);
        return false;
    }
    if((read(fd, &key, 4) == 4) && (key==magic_key)){
        read(fd, &rv->file_num, 4);
        read(fd, &rv->volume_length, 8);
    }else{
        printf("%s, %d, New Recipe!\n", __FILE__, __LINE__);
        lseek(fd, 0, SEEK_SET);
        write(fd, &magic_key, 4);
        write(fd, &rv->file_num, 4);
        write(fd, &rv->volume_length, 8);
    }

    close(fd);
    return true;
}

bool recipe_volume_flush()
{
    P(rv->mutex);
    int fd;
    char path[FILE_NAME_LEN] = {0};
    if(!rv)
        return false;
    strcpy(path, store_path);
	strcat(path, "recipe");

    if((fd = open(path, O_CREAT|O_RDWR, S_IRWXU))<0){
        printf("%s, %d, failed to open recipe volume %s", __FILE__, __LINE__, path);
        return false;
    }
    lseek(fd, 0, SEEK_SET);
    write(fd, &magic_key, 4);
    write(fd, &rv->file_num, 4);
    write(fd, &rv->volume_length, 8);
    close(fd);
    V(rv->mutex);
    return true;
}

bool read_recipe_from_vol(int64_t offset, Recipe *rp)
{
    char path[FILE_NAME_LEN] = {0};
	int fd;
	int32_t count = 0;
	int i;
	FingerChunk *fc = NULL;
    
    P(rv->mutex);
	strcpy(path, store_path);
	strcat(path, "recipe");
	fd = open(path, O_RDWR | O_CREAT , S_IREAD|S_IWRITE);
	if(fd < 0){
		err_msg1("open recipe file error!");
		return false;
	}
	lseek(fd, offset, SEEK_SET);
	if(read(fd, &count, 4) == 4) {
		for(i = 0; i < count; i++) {
			fc = (FingerChunk *)malloc(sizeof(FingerChunk));
			read(fd, fc->chunk_hash, sizeof(Fingerprint));
			read(fd, fc->chunk_address, sizeof(Chunkaddress));

		//	printf("%s,%d, read recipe, chunkaddress is%s\n", __FILE__,__LINE__,fc->chunk_address);
			
			recipe_append_fingerchunk(rp, fc);
		}		
	}
	close(fd);
	V(rv->mutex);

	return true;
}

bool write_recipe_to_vol(int64_t *offset, Recipe *rp)
{
	char path[FILE_NAME_LEN] = {0};
	int fd;
	FingerChunk *fc = NULL;

	printf("%s,%d:write_recipe_to_vol\n",__FILE__,__LINE__);

	strcpy(path,store_path);
	strcat(path, "recipe");
	
	P(rv->mutex);
	fd = open(path, O_RDWR);
	if(fd < 0) {
		err_msg1("open recipe file error!");
		return false;
	}
      *offset = rv->volume_length;
	lseek(fd, *offset, SEEK_SET);
	write(fd, &(rp->chunknum), sizeof(int32_t));

	fc = rp->first;
	for(fc; fc != NULL; fc = fc->next) {
	    write(fd, fc->chunk_hash, sizeof(Fingerprint));
		write(fd, fc->chunk_address, sizeof(Chunkaddress));
		printf("%s,%d, write recipe, chunkaddress is%s\n", __FILE__,__LINE__,fc->chunk_address);
	}
	close(fd);
	rv->volume_length += (sizeof(rp->chunknum) + rp->chunknum*(sizeof(Fingerprint)+sizeof(Chunkaddress)));
	rv->file_num ++;
	V(rv->mutex);

	return true;
}
