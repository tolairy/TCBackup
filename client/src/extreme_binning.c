#include "../global.h"

#define OFF_MASK (0xffffffffffffff)

const static int32_t magic_key=0xa1b2c3d4;//bin volume validation

const static int64_t bvolume_head_size = 20;
const static int64_t max_bin_level = 30;
const static int64_t level_factor = 512;

static struct extreme_binning_tag *exbin;
static struct bin_volume_tag *bin_volume_array[max_bin_level];
/*
 * Bin Volume operations.
 */
BinVolume* BinVolumeInit(int level)
{
    int fd;
    int32_t key;
    char volname[20] = {0};
    int tmp;
    BinVolume* bvol = (BinVolume*)malloc(sizeof(BinVolume));
	memset(bvol, 0, sizeof(BinVolume));
    bvol->level = level;
    bvol->current_bin_num = 0;
    bvol->current_volume_length = bvolume_head_size;

    sprintf(volname, "bvol%d", level);
    strcpy(bvol->filename, BinPath);
    strcat(bvol->filename, volname);
    printf("%s, %d, filename=%s\n",__FILE__,__LINE__,bvol->filename);

    if((fd=open(bvol->filename, O_CREAT|O_RDWR, S_IRWXU))<0){
        printf("%s, %d, failed to open bin volume %d", __FILE__, __LINE__, level);
        return bvol;
    }
    if((read(fd, &key, 4) == 4) && (key==magic_key)){
        if(read(fd, &tmp, 4)!=4 || tmp!=level){
            printf("%s, %d, Read a wrong volume.\n", __FILE__, __LINE__);
            close(fd);
            return bvol;
        }
        read(fd, &bvol->current_bin_num, 4);
        read(fd, &bvol->current_volume_length, 8);
    }else{
        printf("%s, %d, New volume!\n", __FILE__, __LINE__);
        lseek(fd, 0, SEEK_SET);
        write(fd, &magic_key, 4);
        write(fd, &level, 4);
        write(fd, &bvol->current_bin_num, 4);
        write(fd, &bvol->current_volume_length, 8);
    }

    close(fd);
    return bvol;
} 

bool BinVolumeFlush(BinVolume *bvol)
{
    int fd;
    if(!bvol)
        return false;
    if((fd=open(bvol->filename, O_CREAT|O_RDWR, S_IRWXU))<0){
        printf("%s, %d, failed to open bin volume %d", __FILE__, __LINE__, bvol->level);
        return false;
    }
    lseek(fd, 0, SEEK_SET);
    write(fd, &magic_key, 4);
    write(fd, &bvol->level, 4);
    write(fd, &bvol->current_bin_num, 4);
    write(fd, &bvol->current_volume_length, 8);
    close(fd);
    return true;
}

/*
 * Primary Index operations.
 */
bool ExtremeBinningInit()
{
    int fd;
    char filename[FILE_NAME_LEN];
    MainIndex mainindex;
    int32_t key;
    int item_num;
    int i;
    Bin tmpbin;
    exbin = (ExtremeBinning *)malloc(sizeof(ExtremeBinning));
    exbin->main_table = new htable((char*)&mainindex.next - (char*)&mainindex, sizeof(Fingerprint), 3200);
    exbin->bin_cache = new htable((char*)&tmpbin.next - (char*)&tmpbin, sizeof(Fingerprint), 100);

    strcpy(filename, BinPath);
    strcat(filename, "repfinger");
    printf("%s, %d, filename=%s\n",__FILE__,__LINE__,filename);
    
    /* Read Main Table from disk */
    if((fd=open(filename, O_CREAT|O_RDWR, S_IRWXU)) < 0) {
        printf("%s, %d, failed to open repfinger\n", __FILE__, __LINE__);
        return false;
    }else{
        if((read(fd, &key, 4)==4) && (key == magic_key)){  
            read(fd, &item_num, 4);
            for(i=0; i<item_num; ++i) {
		 MainIndex *mainindex1 = (MainIndex*)malloc(sizeof(MainIndex));
                read(fd, mainindex1->rep_finger, FINGER_LENGTH);
		  read(fd, mainindex1->file_hash, FINGER_LENGTH);
                read(fd, &mainindex1->binaddr, sizeof(mainindex1->binaddr));
                exbin->main_table->insert((unsigned char*)&mainindex1->rep_finger, mainindex1);
            }
        }
        close(fd);
    }
    /* Initialize bin volume array. */
    i = 0;
    for(; i < max_bin_level; ++i){
        bin_volume_array[i] = BinVolumeInit(i);
    }
    return true;
}

void ExtremeBinningDestroy()
{
	ExtremeBinningFlush();
       free(exbin);
}

bool ExtremeBinningFlush()
{
    int fd;
    int item_num;
    int i = 0;
    char filename[256];
    strcpy(filename, BinPath);
    strcat(filename, "repfinger");
    printf("%s, %d, filename=%s\n",__FILE__,__LINE__,filename);

    if((fd=open(filename, O_CREAT|O_RDWR, S_IRWXU))<0){
        printf("%s, %d, failed to open repfinger\n",  __FILE__, __LINE__);
        return false;
    }else {
        write(fd, &magic_key, 4);
        item_num = exbin->main_table->size();
        write(fd, &item_num, 4);
        MainIndex *mainindex = (MainIndex*)exbin->main_table->first();
        while(mainindex){
            write(fd, mainindex->rep_finger, FINGER_LENGTH);
	     write(fd, mainindex->file_hash, FINGER_LENGTH);
            write(fd, &mainindex->binaddr, sizeof(mainindex->binaddr));

            mainindex = (MainIndex *)exbin->main_table->next();
        }
    }
    close(fd);
    i = 0;
    for(; i < max_bin_level; i++){
        BinVolumeFlush(bin_volume_array[i]);
    }
    return true;
}


int64_t level_to_max_amount(int64_t level)
{
    if(level < 0 || level > max_bin_level){
        printf("%s, %d: invalid level %ld.\n", __FILE__, __LINE__, level);
        return -1;
    }
    int64_t amount = ((uint64_t)pow(2, level))*level_factor;
    return amount;
}

/*
 * return : the address of bin.
 */
int64_t WriteBinToVolume(Bin *bin)
{
    int64_t level = bin->address >> 56;
    int64_t offset = bin->address & OFF_MASK;
    int32_t nlen;
    int32_t chunknum;
    int fd;
    int errno;
    int64_t current_bin_size;
    char *tmp = NULL, *p = NULL;
    int64_t sumWriteNums = 0;
    int64_t writeNums = 0;
    chunknum = bin->fingers->size();
    nlen = (FINGER_LENGTH + sizeof(int32_t)) + bin->fingers->size()*(FINGER_LENGTH + CHUNK_ADDRESS_LENGTH);
    while( nlen > level_to_max_amount(level)){
      	printf("%s, %d: beyond the scope of this level.\n", __FILE__, __LINE__);
        level++;
        offset = 0;
    }

    current_bin_size = level_to_max_amount(level);
    
    tmp = (char*)malloc(current_bin_size);
    memset(tmp, 0, current_bin_size);
    p = tmp;
    
    memcpy(p, bin->rep_finger, FINGER_LENGTH);
	int i;
	//printf("%s, %d, rep finger:", __FILE__, __LINE__);
	//for (i = 0; i < FINGER_LENGTH; i++)
	//	printf("%d/", ((char *)bin->rep_finger)[i]);
	//printf("\n");
	
    p = p + FINGER_LENGTH;
    memcpy(p, &chunknum, sizeof(chunknum));
    p = p + sizeof(chunknum);

    if(bin->fingers){
        ChunkMeta *cmeta= (ChunkMeta*)bin->fingers->first();
        while(cmeta){
			int i;
			//printf("%s, %d, finger:", __FILE__, __LINE__);
		//	for (i = 0; i < FINGER_LENGTH; i++)
			//	printf("%d/", ((char *)cmeta->finger)[i]);
			//printf("\n");
			//printf("%s, %d, address:%s\n", __FILE__, __LINE__, cmeta->address);
            memcpy(p, cmeta->finger, FINGER_LENGTH);
            p = p + FINGER_LENGTH;
            memcpy(p, cmeta->address, CHUNK_ADDRESS_LENGTH);
            p = p + CHUNK_ADDRESS_LENGTH;
            cmeta = (ChunkMeta*)bin->fingers->next();
        }
    }
    if((p-tmp) < level_to_max_amount(level)){
        memset(p, 0xcf, current_bin_size - (p - tmp));
    }

    BinVolume *bvol = bin_volume_array[level];
	
	
    if(offset == 0){
        /* get a new offset ,is a new bin to the volume*/
        offset = bvol->current_volume_length;
		bvol->current_bin_num ++;
    	bvol->current_volume_length += current_bin_size;
    }
	
    fd = open(bvol->filename, O_RDWR);
    lseek(fd, offset, SEEK_SET);
    while(sumWriteNums != current_bin_size){
        writeNums = write(fd, tmp + sumWriteNums, current_bin_size - sumWriteNums);
        if(writeNums < 0) {
            if(errno == EINTR){
                continue;
            }
            if(errno == EAGAIN){
                continue;
            }
            err_msg1("write bin failed!");
            free(tmp);
            close(fd);
            return 0;
        }
        sumWriteNums += writeNums;
    }
    close(fd);
   

    free(tmp);
    return (level<<56)+offset;
}

bool ReadBinFromVolume(Bin *bin)
{
    int fd;
    int i;
    int32_t chunknum;
    int errno;
    int64_t level = bin->address >> 56;
    int64_t offset = bin->address & OFF_MASK;
    int64_t sumReadNums = 0;
    int64_t readNums = 0;
    int64_t current_bin_size = 0;
    Fingerprint repfinger;
    char *p = NULL, *tmp = NULL;
    BinVolume *bvol = bin_volume_array[level];

    printf("%s,%d:read bin using ReadBinFromVolume(), level %d, offset %d\n",__FILE__,__LINE__,level, offset);
	
    current_bin_size = level_to_max_amount(level);
	
	fd = open(bvol->filename, O_RDWR);
    if(fd <= 0){
        err_msg2("open bin %s error", bvol->filename);
        return false;
    }
    lseek(fd, offset, SEEK_SET);
    p = (char*)malloc(current_bin_size);
    tmp = p;
    while(sumReadNums != current_bin_size){
        readNums = read(fd, p+sumReadNums, current_bin_size - sumReadNums);
        if(readNums < 0){
            if(errno == EINTR){
                continue;
            }
            if(errno == EAGAIN){
                continue;
            }
            err_msg1("read failed!");
            free(p);
            close(fd);
            return false;
        }
        sumReadNums += readNums;
    }
    close(fd);

    memset(repfinger, 0, FINGER_LENGTH);
    memcpy(repfinger, tmp, FINGER_LENGTH);
    tmp = tmp + FINGER_LENGTH;
    if(memcmp(&bin->rep_finger, &repfinger, sizeof(Fingerprint))!=0){
        printf("%s, %d: Read an incompatible bin.\n", __FILE__, __LINE__);
        free(p);
        return false;
    }
    memcpy(&chunknum, tmp, sizeof(chunknum));
    tmp = tmp + sizeof(chunknum);
    for(i=0; i < chunknum; i++) {
        ChunkMeta *cmeta = (ChunkMeta*)malloc(sizeof(ChunkMeta));
        memset(cmeta, 0, sizeof(ChunkMeta));
        memcpy(cmeta->finger, tmp, FINGER_LENGTH);
        tmp = tmp + FINGER_LENGTH;
        memcpy(cmeta->address, tmp, CHUNK_ADDRESS_LENGTH);
        tmp = tmp + CHUNK_ADDRESS_LENGTH;

        bin->fingers->insert((unsigned char*)&cmeta->finger, cmeta);
    }

    free(p);
    return true;
}


Bin *NewBin(int64_t addr, Fingerprint rep_finger, int32_t chunk_num)
{
    ChunkMeta chunkmeta;
    Bin *bin = (Bin*)malloc(sizeof(Bin));
    printf("%s,%d:%s\n",__FILE__,__LINE__,"load a new bin using NewBin()");
    bin->address = addr;
    memcpy(bin->rep_finger, rep_finger, sizeof(Fingerprint));

    bin->fingers = new htable((char*)&chunkmeta.next - (char*)&chunkmeta, FINGER_LENGTH, chunk_num);
    return bin;
}

/* 
 * lookup the representative fingerprint,
 * and locate the bin.
 */
int64_t LocationBin(FileInfo *fileinfo)
{
    MainIndex *mainindex = NULL;
    printf("%s,%d:LocationBin()\n",__FILE__,__LINE__);
    mainindex = (MainIndex *)exbin->main_table->lookup((unsigned char*)&fileinfo->rep_finger);
    if(!mainindex){
	  mainindex = (MainIndex*)malloc(sizeof(MainIndex));
	  memset(mainindex, 0, sizeof(mainindex));
         memcpy(mainindex->rep_finger, fileinfo->rep_finger, sizeof(Fingerprint));
	  memcpy(mainindex->file_hash, fileinfo->file_hash, sizeof(Fingerprint));
         mainindex->binaddr =  0;
         exbin->main_table->insert((unsigned char*)&mainindex->rep_finger, mainindex);
    }
    return mainindex->binaddr;
}

/*
 * retrieve the bin from volume or create a new bin if offset is zero.
 * LoadBin when you need it, and FreeBin after work.
 */
Bin* LoadBin(FileInfo *fileinfo)
{
    Bin *bin = NULL; 
    //printf("%s,%d, load bin.\n",__FILE__,__LINE__);
    bin = (Bin*)exbin->bin_cache->lookup((unsigned char*)fileinfo->rep_finger);
    if(bin){
	   return bin;
    }
    int64_t addr = LocationBin(fileinfo);
    bin = NewBin(addr, fileinfo->rep_finger, fileinfo->chunknum);
    exbin->bin_cache->insert((unsigned char*)&bin->rep_finger, bin);

    if(addr == 0) {
	  return bin;
    }

    /*retrieve the bin from volume*/
    ReadBinFromVolume(bin);
    if(memcmp(&fileinfo->rep_finger, &bin->rep_finger, sizeof(Fingerprint))!=0)
        printf("%s, %d: Read an incompatible bin.\n", __FILE__, __LINE__);
    return bin;
}

void FreeBin(Bin* bin)
{
    int64_t new_addr = 0;
    MainIndex *mainindex = NULL;
	
    new_addr = WriteBinToVolume(bin);
    if(new_addr !=0 ){
		mainindex  = (MainIndex*)exbin->main_table->lookup((unsigned char*)&bin->rep_finger);
        if(mainindex == NULL){
		err_msg1("can not find bin_rep_finger in mainindex");
		return;
        }
	 	mainindex->binaddr = new_addr;
    }
    bin = (Bin*)exbin->bin_cache->remove((unsigned char*)&bin->rep_finger);
    if(bin){
        if(bin->fingers)
             bin->fingers->destroy();
	free(bin);
    }
    return;
}

/* write all bin to disk and update the primary index */
bool free_cache_bin()
{
    Bin *bin = NULL;
    printf("%s,%d:free_cache_bin,cache_size:%d\n",__FILE__,__LINE__,exbin->bin_cache->size());
    bin = (Bin*)exbin->bin_cache->first();
    while(bin) {
	    FreeBin(bin);
	    bin = (Bin*)exbin->bin_cache->next();
    }
    return true;
}
