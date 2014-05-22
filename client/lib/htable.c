#include "../global.h"
/*
 * Take each hash link and walk down the chain of items
 *  that hash there counting them (i.e. the hits), 
 *  then report that number.
 *  Obiously, the more hits in a chain, the more time
 *  it takes to reference them. Empty chains are not so
 *  hot either -- as it means unused or wasted space.
 */
#define MAX_COUNT 20

/* ===================================================================
 *    htable
 */

/*
 * Create hash of key, stored in hash then
 *  create and return the pseudo random bucket index
 */
 
#define HASH_SIZE 40 
#define UINT32_SIZE 4 


void htable::hash_index(unsigned char *key)
{ 
   hash = 0;   
   if(key==NULL)
   {
   		printf("%s,%d:Insert key==NULL\n",__FILE__,__LINE__);
		index=0;
   		return ;
   }
   for (int i = hashlength-1; hash < buckets && i > 0; i--) {
      hash += (hash *16) + (key[i]<'9'?key[i]-'0':10+key[i]-'A');
   }
   ///Multiply by large prime number, take top bits, mask for remainder 
   index = hash & mask;

   return;
}

htable::htable(int offset,int nhlen, int tsize)
{
   init(offset, nhlen, tsize);
}

void htable::init(int offset,int nhlen, int tsize)
{
   int pwr;
   tsize >>= 2;
   for (pwr=0; tsize; pwr++) {
      tsize >>= 1;
   }
   loffset = offset;
   mask = ~((~0)<<pwr);               /* 3 bits => table size = 8 */
   rshift = 30 - pwr;                 /* start using bits 28, 29, 30 */
   num_items = 0;                     /* number of entries in table */
   buckets = 1<<pwr;                  /* hash table size -- power of two */
   max_items = buckets * 4;           /* allow average 4 entries per chain */
   table = (hlink **)malloc(buckets * sizeof(hlink *));
   memset(table, 0, buckets * sizeof(hlink *));
   walkptr = NULL;
   walk_index = 0;
   hashlength = nhlen;
}

u_int32_t htable::size()
{
   return num_items;
}

void htable::stats()
{
   int hits[MAX_COUNT];
   int max = 0;
   int i, j;
   hlink *p;
   printf("\n\nNumItems=%d\nTotal buckets=%d\n", num_items, buckets);
   printf("Hits/bucket: buckets\n");
   for (i=0; i < MAX_COUNT; i++) {
      hits[i] = 0;
   }
   for (i=0; i<(int)buckets; i++) {
      p = table[i];
      j = 0;
      while (p) {
         p = (hlink *)(p->next);
         j++;
      }
      if (j > max) {
         max = j;
      }
      if (j < MAX_COUNT) {
         hits[j]++;
      }
   }
   for (i=0; i < MAX_COUNT; i++) {
      printf("%2d:           %d\n",i, hits[i]);
   }
   printf("max hits in a bucket = %d\n", max);
}

void htable::grow_table()
{
   //Dmsg1(100, "Grow called old size = %d\n", buckets);
   /* Setup a bigger table */
   htable *big = (htable *)malloc(sizeof(htable));
   big->loffset = loffset;
   big->hashlength  = hashlength;
   big->mask = mask<<1 | 1;
   big->rshift = rshift - 1;
   big->num_items = 0;
   big->buckets = buckets * 2;
   big->max_items = big->buckets * 4;
   big->table = (hlink **)malloc(big->buckets * sizeof(hlink *));
   memset(big->table, 0, big->buckets * sizeof(hlink *));
   big->walkptr = NULL;
   big->walk_index = 0;
   /* Insert all the items in the new hash table */
   //Dmsg1(100, "Before copy num_items=%d\n", num_items);
   /*
    * We walk through the old smaller tree getting items,
    * but since we are overwriting the colision links, we must
    * explicitly save the item->next pointer and walk each
    * colision chain ourselves.  We do use next() for getting
    * to the next bucket.
    */
   for (void *item=first(); item; ) {
      void *ni = ((hlink *)((char *)item+loffset))->next;  /* save link overwritten by insert */
      //Dmsg1(100, "Grow insert: %s\n", ((hlink *)((char *)item+loffset))->key);
      big->insert(((hlink *)((unsigned char *)item+loffset))->key, item);
      if (ni) {
         item = (void *)((char *)ni-loffset);
      } else {
         walkptr = NULL;
         item = next();
      }
   }
   //Dmsg1(100, "After copy new num_items=%d\n", big->num_items);
   if (num_items != big->num_items) {
      //Dmsg0(000, "****** Big problems num_items mismatch ******\n");
   }
   free((char *)table);
   memcpy(this, big, sizeof(htable));  /* move everything across */
   free((char *)big);
   //Dmsg0(100, "Exit grow.\n");
}

bool htable::insert(unsigned char *key, void *item)
{
   hlink *hp;
   if(key==NULL)
   {
   		printf("%s,%d:Insert key==NULL\n",__FILE__,__LINE__);
   		return false;
   }
   hash_index(key);
   
   hp = (hlink *)(((char *)item)+loffset);
   hp->next = table[index]; /*some pro*/
   
   //hp->hash = hash;
   //memcpy(&hp->hash,&hash,sizeof(hash));
   //TRACE("%d",sizeof(hash));
   //free(item);
   hp->key = key;
   table[index] = hp;

   if (++num_items >= max_items) {
      //printf("%s,%d:grow_table_hash num_items=%d max_items=%d\n", __FILE__,__LINE__,num_items, max_items);
      grow_table();
   }
   return true;
}

void *htable::lookup(unsigned char *key)
{
   hash_index(key);
   for (hlink *hp=table[index]; hp; hp=(hlink *)hp->next)
   	{
      if (memcmp(key, hp->key,hashlength) == 0) //hash == hp->hash && 
	  {
         //Dmsg1(100, "lookup return %x\n", ((char *)hp)-loffset);
         return ((char *)hp)-loffset;
      }
   }
   return NULL;
}

void *htable::remove(unsigned char *key)
{
   hash_index(key);
   hlink *pre = table[index];
   for (hlink *hp=table[index]; hp; hp=(hlink *)hp->next)
   	{
      if (memcmp(key, hp->key,hashlength) == 0) //hash == hp->hash && 
	  {
         //Dmsg1(100, "lookup return %x\n", ((char *)hp)-loffset);
         if(pre==hp){
             table[index] = (hlink*)hp->next;
         }else{
             pre->next = hp->next;
         }
         --num_items;
         return ((char *)hp)-loffset;
      }
      pre = hp;
   }
   return NULL;
}

void *htable::search(unsigned char *key)
{
   hash_index(key);
   for (hlink *hp=table[index]; hp; hp=(hlink *)hp->next) 
   {
      if (memcmp(key, hp->key,hashlength) == 0) //hash == hp->hash && 
	  {
         //Dmsg1(100, "lookup return %x\n", ((char *)hp)-loffset);
		 return hp;
      }

  }
  return NULL;

}

void *htable::next()
{
   //Dmsg1(100, "Enter next: walkptr=0x%x\n", (unsigned)walkptr);
   if (walkptr) {
      walkptr = (hlink *)(walkptr->next);
   }
   while (!walkptr && walk_index < buckets) {
      walkptr = table[walk_index++];
      if (walkptr) {
         //Dmsg3(100, "new walkptr=0x%x next=0x%x inx=%d\n", (unsigned)walkptr,(unsigned)(walkptr->next), walk_index-1);
      }
   }
   if (walkptr) {
      //Dmsg2(100, "next: rtn 0x%x walk_index=%d\n",(unsigned)(((char *)walkptr)-loffset), walk_index);
      return ((char *)walkptr)-loffset;
   }
   //Dmsg0(100, "next: return NULL\n");
   return NULL;
}

void *htable::first()
{
   //Dmsg0(100, "Enter first\n");
   walkptr = table[0];                /* get first bucket */
   walk_index = 1;                    /* Point to next index */
   while (!walkptr && walk_index < buckets) {
      walkptr = table[walk_index++];  /* go to next bucket */
      if (walkptr) {
         //Dmsg3(100, "first new walkptr=0x%x next=0x%x inx=%d\n", (unsigned)walkptr,(unsigned)(walkptr->next), walk_index-1);
      }
   }
   if (walkptr) {
      //Dmsg1(100, "Leave first walkptr=0x%x\n", (unsigned)walkptr);
      return ((char *)walkptr)-loffset;
   }
   //Dmsg0(100, "Leave first walkptr=NULL\n");
   return NULL;
}

/* Destroy the table and its contents */
void htable::destroy()
{
   void *ni;
   void *li = first();
   while(li){
       ni = next();
       free((char*)li);
       li = ni;
   }

   free((char *)table);
   table = NULL;
   //Dmsg0(100, "Done destroy.\n");
}

