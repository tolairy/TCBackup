#ifndef _RABIN_H
#define _RABIN_H

#define MIN_CHUNK_SIZE  2048 /*2k */
#define MAX_CHUNK_SIZE  65536 /*64k */

 void chunk_alg_init();
 void chunk_finger(unsigned char *buf, uint32_t len,unsigned char hash[]);
  int   chunk_data (unsigned char *p, int n);
void digestToHash(unsigned char digest[20],char hash[41]);

#endif

