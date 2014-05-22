#include "../global.h"

#define MSB64 0x8000000000000000LL
#define FINGERPRINT_PT  0xbfe6b8a5bf378d83LL
#define BREAKMARK_VALUE 0x78

enum {size = 48};
uint64_t fingerprint;
static int rabin_init_flag = 0;
static int bufpos;
static uint64_t  U[256];
static uint8_t buf[size];
static int shift;
static uint64_t T[256];
static uint64_t poly;

/*�뺯��slide8��Ч����ʹ�ú��滻�ٶȸ���*/
#define SLIDE(m) do{       \
	    unsigned char om;   \
	    uint64_t x;	 \
        if (++bufpos >= size)  \
        bufpos = 0;				\
        om = buf[bufpos];		\
        buf[bufpos] = m;		 \
		fingerprint ^= U[om];	 \
		x = fingerprint >> shift;  \
		fingerprint <<= 8;		   \
		fingerprint |= m;		  \
		fingerprint ^= T[x];	 \
}while(0)


static unsigned long _last_pos;
static unsigned long _cur_pos;
  
static unsigned int _num_chunks;
static const unsigned chunk_size = 0x1FFF; 

double cdtime = 0,nctime = 0;

const char bytemsb[0x100] = {
  0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

/******************************the rabin************************/
static  uint8_t  fls32 (uint32_t v)
{
  if (v & 0xffff0000) {
    if (v & 0xff000000)
      return 24 + bytemsb[v>>24];
    else
      return 16 + bytemsb[v>>16];
  }
  if (v & 0x0000ff00)
    return 8 + bytemsb[v>>8];
  else
    return bytemsb[v];
}

static uint8_t fls64 (uint64_t v)
{
  uint32_t h;
  if ((h = v >> 32))
    return 32 + fls32 (h);
  else
    return fls32 ((uint32_t) v);
}



static uint64_t polymod (uint64_t nh, uint64_t nl, uint64_t d)
{

  int k = fls64 (d) - 1;
  int i;
  d <<= (63 - k);
  if (nh) {
    if (nh & MSB64)
      nh ^= d;
    for (i = 62; i >= 0; i--)
      if (nh & ((uint64_t) 1 << i)) {
    nh ^= d >> (63 - i);
    nl ^= d << (i + 1);
      }
  }
  for (i = 63; i >= k; i--)
  {  
    if (nl & ((uint64_t) 1) << i)
      nl ^= d >> (63 - i);
  }
  return nl;
}

static void polymult (uint64_t *php, uint64_t *plp, uint64_t x, uint64_t y)
{

  int i;

  uint64_t ph = 0, pl = 0;
  if (x & 1)
    pl = y;
  for ( i = 1; i < 64; i++)
    if (x & ((int64_t)1 << i)) {
      ph ^= y >> (64 - i);
      pl ^= y << i;
    }
  if (php)
    *php = ph;
  if (plp)
    *plp = pl;
}

static uint64_t append8 (uint64_t p, uint8_t m){ 
     return ((p << 8) | m) ^ T[p >> shift]; 
};


static uint64_t slide8 (uint8_t m) 
{
   unsigned char om;
	uint64_t x;
    if (++bufpos >= size)
        bufpos = 0;
        om = buf[bufpos];
        buf[bufpos] = m;
		fingerprint ^= U[om];
		x = fingerprint >> shift;
		fingerprint <<= 8;
		fingerprint |= m;
		fingerprint ^= T[x];
    return fingerprint /*= append8 (fingerprint ^ U[om], m)*/;
}

static uint64_t polymmult (uint64_t x, uint64_t y, uint64_t d)
{
  uint64_t h, l;
  polymult (&h, &l, x, y);
  return polymod (h, l, d);
}

static void calcT (uint64_t poly)
{

  int j;
  uint64_t T1;

  int xshift = fls64 (poly) - 1;
  shift = xshift - 8;
  T1 = polymod (0, ((int64_t) 1<< xshift), poly);
  for (j = 0; j < 256; j++)
  {
    T[j] = polymmult (j, T1, poly) | ((uint64_t) j << xshift);
  }
}

static void rabinpoly_init (uint64_t  p)
{
  poly=p;
  calcT (poly);
}

static void window_init (uint64_t poly)
{

  int i;
  uint64_t sizeshift;

  rabinpoly_init(poly);
  fingerprint = 0;
  bufpos = -1;
  sizeshift = 1;
  for (i = 1; i < size; i++)
    sizeshift = append8 (sizeshift, 0);
  for (i = 0; i < 256; i++)
    U[i] = polymmult (i, sizeshift, poly);
  memset ((char*) buf,0, sizeof (buf));
}

static void windows_reset () { 
    fingerprint = 0; 
    memset((char*) buf,0,sizeof (buf));
}

void chunk_alg_init(){
	if(rabin_init_flag == 0){
    	window_init(FINGERPRINT_PT);
		rabin_init_flag = 1;
	}
    _last_pos = 0;
    _cur_pos = 0;
    windows_reset();
    _num_chunks = 0;
}

/*********************SHA1 hash**************************/
/*
*��ָ��
*/
void chunk_finger(unsigned char *buf, uint32_t len,unsigned char hash[])
{
 /*  SHA1Context context;
   unsigned char digest[20];
   SHA1Init(&context);
   SHA1Update(&context, buf, (int)len);
   SHA1Final(&context,digest);*/

   SHA_CTX context;
   unsigned char digest[20];
   SHA1_Init(&context);
   SHA1_Update(&context, buf, (int)len);
   SHA1_Final(digest,&context);
   memcpy(hash,digest,20);

}

/*********************rabin chunking*************************/

/*
*�ֿ�
*/
int chunk_data(unsigned char *p, int n)
{
	int i=1;
	//windows_reset();
	while(i<=n){
		SLIDE(p[i-1]);
		if (((fingerprint & chunk_size) == BREAKMARK_VALUE && i >= MIN_CHUNK_SIZE) 
        || i >= MAX_CHUNK_SIZE || i == n) {  /*Ѱ���и�㣬�����ΪMIN_CHUNK_SIZE���ΪMAX_CHUNK_SIZE*/
			break;
		}
		else i++;
  }
  return i;
}

void digestToHash(unsigned char digest[20],char hash[41])
{
	int i, j, k, b;
	unsigned char a,c;
	i=0;
	for (i = 0; i < 5; i++) {
        for (j = 0; j < 4; j++) {

              k=2;
              a=digest[i*4+j];
              while(k>0){
                  b=a / 16;
                  switch(b){
                     case 10:
                       c='A';
                       break;
                     case 11:
                       c='B';
                       break;
                     case 12:
                       c='C';
                       break;
                     case 13:
                       c='D';
                       break;
                     case 14:
                       c='E';
                       break;
                     case 15:
                       c='F';
                       break;
                     default:
                       c=b+48;
                       break;

                  }
				  hash[2*(i*4+j)+2-k]=c;
                  a = a << 4;
                  k--;
                }

        }
    }
	hash[40] = '\0';
}


