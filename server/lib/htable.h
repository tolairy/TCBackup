/* ========================================================================
 *
 *  stable
 *
 */

#ifndef _HTABLE_H
#define _HTABLE_H

//#define HTB_TEST /* unit test */
#ifdef HTB_TEST
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <openssl/sha.h>
#endif

/*
 * Loop var through each member of table
 */
#define KEY_LEN 20
#define bstrdup(str) strcpy((char *)malloc(strlen((str))+1),(str))
#define HASH_TBALE_SIZE 16777215    /*2^24-1*/


#define foreach_htable(var, tbl) \
        for((*((void **)&(var))=(void *)(htable_get_first((tbl)))); \
            (var);                                         \
            (*((void **)&(var))=(void *)(htable_get_next(tbl))))


typedef struct hlink_t {
   void *next;                        /* next hash item */
   uint32_t hash;                     /* hash for this key */
   unsigned char *key; /* key this item */
}hlink;


typedef struct htable_t {
   hlink **table;                     /* hash table */
   int loffset;                       /* link offset in item */
   int keylen;
   uint32_t num_items;                /* current number of items */
   uint32_t max_items;                /* maximum items before growing */
   uint32_t grow_count;               /* grow counts */
   uint32_t replace_count;            /* the numbers of items in buckets are replaced by other items */
   uint32_t buckets;                  /* size of hash table */
   uint32_t hash;                     /* temp storage */
   uint32_t index;                    /* temp storage */
   uint32_t mask;                     /* "remainder" mask */
   uint32_t rshift;                   /* amount to shift down */
   hlink *walkptr;                    /* table walk pointer */
   uint32_t walk_index;               /* table walk index */
} htable;

   htable *htable_init(int offset, int keylen, int tsize);

   int htable_insert(htable *tbl,unsigned char *key, void *item);
   int htable_add(htable *tbl,unsigned char *key, void *item);
   void *htable_lookup(htable *tbl,unsigned char *key);
   int htable_remove(htable *tbl,unsigned char *key);
   void *htable_get_first(htable *);                     /* get first item in table */
   void *htable_get_next(htable *);                      /* get next item in table */
   void htable_destroy(htable *);
   void htable_get_stats(htable *);                      /* print stats about the table */
   uint32_t htable_get_size(htable *);                   /* return size of table */

#endif


