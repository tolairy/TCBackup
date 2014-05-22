#ifndef RECIPE_H_
#define RECIPE_H_


#define NEW_FILE 0
#define DEDUP_FILE 1

#define recipe_volume_head_size 16

typedef struct finger_chunk_tag {
    Fingerprint chunk_hash;
    Chunkaddress chunk_address;
    struct finger_chunk_tag *next;
}FingerChunk;


typedef struct recipe_tag {
	int64_t chunknum;
	char filename[FILE_NAME_LEN];
	Fingerprint file_hash;
	int is_new;
    FingerChunk *first;
    FingerChunk *last;
}Recipe;

typedef struct recipe_volume {
	int32_t file_num;
    int64_t volume_length;
    pthread_mutex_t mutex;
}RecipeVolume;

FingerChunk* fingerchunk_new(Fingerprint hash, Chunkaddress address);
void fingerchunk_free(FingerChunk *fc);
Recipe* recipe_new();
void recipe_free(Recipe* trash);
Recipe* recipe_append_fingerchunk(Recipe *recipe, FingerChunk *fchunk);
bool recipe_volume_init();
bool recipe_volume_flush();
bool read_recipe_from_vol(int64_t offset, Recipe *rp);
bool write_recipe_to_vol(int64_t *offset, Recipe *rp);



#endif /* RECIPE_H_ */

