#ifndef _EXBIN_H_
#define _EXBIN_H_


typedef struct extreme_binning_tag{
    htable *main_table;
    htable *bin_cache;
}ExtremeBinning;

/*
 * Primary index in RAM
 */
typedef struct main_index{
    Fingerprint rep_finger;
    Fingerprint file_hash;
    int64_t binaddr;
    hlink next;
}MainIndex;

/*
 * Secondary index
 */
typedef struct bin_volume_tag{
    int32_t current_bin_num;
    int64_t current_volume_length;
    char filename[FILE_NAME_LEN];
    int level;
}BinVolume;


typedef struct bin_tag{
    Fingerprint rep_finger;
    htable *fingers;
    int64_t address;
    hlink next;
}Bin;

typedef struct chunkmeta_tag{
    Fingerprint finger;
    Chunkaddress address;
    hlink next;
}ChunkMeta;


bool ExtremeBinningInit();
void ExtremeBinningDestroy();
bool ExtremeBinningFlush();

BinVolume* BinVolumeInit(int level);
bool BinVolumeFlush(BinVolume *bvol);
int64_t WriteBinToVolume(Bin *bin);
bool ReadBinFromVolume(Bin *bin);

Bin *NewBin(int64_t addr, Fingerprint rep_finger, int32_t chunk_num);
int64_t FindBin(FileInfo *fileinfo);
int64_t LocationBin(FileInfo *fileinfo);
Bin* LoadBin(FileInfo *fileinfo);
void FreeBin(Bin* bin);
bool free_cache_bin();

int64_t level_to_max_amount(int64_t level);

#endif
