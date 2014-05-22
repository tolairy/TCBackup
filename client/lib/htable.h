
/* ========================================================================
 *
 *   Hash table class -- htable
 *
 */

/*
 * Loop var through each member of table
 */
//#include "../exbin.h"

#ifndef __HTABLE__
#define __HTABLE__

//typedef unsigned long long int u_int64_t;

#define foreach_htable(var, tbl) \
        for((*((void **)&(var))=(void *)((tbl)->first())); \
            (var); \
            (*((void **)&(var))=(void *)((tbl)->next())))

//typedef unsigned int u_int32_t;
//typedef unsigned long long int u_int64_t;

//typedef unsigned int UINT32;
//typedef unsigned long long int UINT64;

#define MAX_PATH 256

typedef struct {
   void *next;                        /* next hash item */
   unsigned char *key;                         /* key this item */
   uint64_t hash;                     /* hash for this key */
}hlink;

class htable {
public:
   hlink **table;                     /* hash table */
   int hashlength;
   int loffset;                       /* link offset in item */
   u_int32_t num_items;                /* current number of items */
   u_int32_t max_items;                /* maximum items before growing */
   u_int32_t buckets;                  /* size of hash table */
   uint64_t hash;                     /* temp storage */
   u_int32_t index;                    /* temp storage */
   u_int32_t mask;                     /* "remainder" mask */
   u_int32_t rshift;                   /* amount to shift down */
   hlink *walkptr;                    /* table walk pointer */
   u_int32_t walk_index;               /* table walk index */
   void hash_index(unsigned char *key);        /* produce hash key,index */
   void grow_table();                 /* grow the table */
   htable(int offset,int nhlen=21, int tsize = 31);
   ~htable() { destroy(); }
   void init(int offset,int nhlen, int tsize = 31);
   bool  insert(unsigned char *key, void *item);
   void *lookup(unsigned char *key);
   void *search(unsigned char *key);
   void *remove(unsigned char *key);
   void *first();                     /* get first item in table */
   void *next();                      /* get next item in table */
   void destroy();
   void stats();                      /* print stats about the table */
   u_int32_t size();                   /* return size of table */
};

#endif
