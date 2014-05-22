#include "../global.h"
//#include "htable.h"



#define MAX_COUNT 4 /* the number at erery bucket*/
static   void hash_index(htable *,unsigned char *);  /* produce hash key,index */
static   void grow_table(htable *);                 /* grow the table */
static   void cut_table(htable *);					/*cut the table*/


/*	 
 * Create hash of key, stored in hash then
 *  create and return the pseudo random bucket index
 */

/*
*对关键字key求取hash值和索引index
*/


void hash_index(htable *tbl,unsigned char *key)
{ 
	int i;
	/* we need to change the for cycle  at practice */
     /*tbl->hash = 0; 
 	for(i=KEY_LEN;tbl->hash <tbl->buckets && i>=0;i--)
	{
		tbl->hash+=(tbl->hash*16)+(key[i]<'9'?key[i]-'0':10+key[i]-'A');
	}
	tbl->index=tbl->hash & tbl->mask;
	 */

	
	 tbl->hash=*(uint32_t *)key;
	//memcpy(tbl->hash,key,4);
	 tbl->index=tbl->hash & tbl->mask;
	

}

/*
*初始化hash表
*参数为ITEM和link 的link 偏移，key 大小，总的规模大小
桶的长度, 求索引值函数
*/
htable *htable_init(int offset, int keylen, int tsize)
{
   int pwr;
   htable *tbl;
   tbl = (htable *)malloc(sizeof(htable));
 
   tsize >>= 2;
   for (pwr=0; tsize; pwr++) {
      tsize >>= 1;
   	}
   tbl->loffset =offset;
   tbl->mask = ~((~0)<<pwr);               /* 3 bits => table size = 8 */
   tbl->rshift = 30 - pwr;                 /* start using bits 28, 29, 30 */
   tbl->num_items = 0;                     /* number of entries in table */
   tbl->grow_count=0;
   tbl->keylen=keylen;
   tbl->replace_count=0;
   tbl->buckets = 1<<pwr;                  /* hash table size -- power of two */
   tbl->max_items = tbl->buckets * MAX_COUNT;   /* allow average 4 entries per chain */
   tbl->table = (hlink **)malloc(tbl->buckets * sizeof(hlink *));
   memset(tbl->table, 0, tbl->buckets * sizeof(hlink *));
   tbl->walkptr = NULL;
   tbl->walk_index = 0;
   tbl->index=0;
   tbl->hash=0;
   return tbl;
}

/*
*获得hash表的当前规模
*/
uint32_t htable_get_size(htable *tbl)
{
   return tbl->num_items;
}

/*
 * Take each hash link and walk down the chain of items
 *  that hash there counting them (i.e. the hits), 
 *  then report that number.
 *  Obiously, the more hits in a chain, the more time
 *  it takes to reference them. Empty chains are not so
 *  hot either -- as it means unused or wasted space.
 */

/*
*获取hash表的状态
*此函数用于测试
*/
void htable_get_stats(htable *tbl)
{
   int hits[MAX_COUNT+1+1];
   int max = 0;
   int i, j;
   hlink *p;
   printf("\n=====================htable stat======================\n");
   printf("current counts: %u\n total counts: %u\n", tbl->num_items, tbl->max_items);
   printf("total buckets: %u \n ",tbl->buckets);
   printf("grow table counts: %u\n",tbl->grow_count);
   printf("replaced items counts: %u \n",tbl->replace_count);
   printf("item_counts/bucket: buckets\n");
   for (i=0; i <= MAX_COUNT+1; i++) {
      hits[i] = 0;
   }
   for (i=0; i<(int)tbl->buckets; i++) {
      p = tbl->table[i];
      j = 0;
      while (p) {/*获取每个桶的长度*/
         p = (hlink *)(p->next);
         j++;
      }
      if (j > max) {
         max = j;/*获取所有桶中，最长链表的长度*/
      }
      if (j <= MAX_COUNT) {
         hits[j]++;/*get the number of buckets have the length j */
      }
     else
	  hits[MAX_COUNT+1]++;
   }
   for (i=0; i <=MAX_COUNT; i++) {
      printf("%2d:           %d\n",i, hits[i]);
   }
      printf("more:           %d\n", hits[MAX_COUNT+1]);
   printf("max itme counts in a bucket : %d\n", max);
   printf("\n=====================htable stat======================\n");
}

/*
*如果hash表已经满了，则增长hash表的规模为原来的2倍
*/
void grow_table(htable *htb)
{
   
   void *item;
   htb->grow_count++;
   /* Setup a bigger table */
   htable *big = (htable *)malloc(sizeof(htable));
   big->loffset = htb->loffset;
   big->keylen=htb->keylen;
   big->mask = htb->mask<<1 | 1;
   big->rshift = htb->rshift - 1;
   big->num_items = 0;
   big->grow_count=htb->grow_count;
   big->replace_count=0;
   big->buckets = htb->buckets * 2;
   big->max_items = big->buckets * MAX_COUNT;
   big->table = (hlink **)malloc(big->buckets * sizeof(hlink *));
   memset(big->table, 0, big->buckets * sizeof(hlink *));
   big->walkptr = NULL;
   big->walk_index = 0;
   /*
    * We walk through the old smaller tree getting items,
    * but since we are overwriting the colision links, we must
    * explicitly save the item->next pointer and walk each
    * colision chain ourselves.  We do use next() for getting
    * to the next bucket.
    */

   for (item=htable_get_first(htb); item; ) {

      void *ni = ((hlink *)((char *)item+htb->loffset))->next;  /* save link overwritten by insert */
	  /*将原hash表的元素插入到新hash表中*/
     if(htable_insert(big,((hlink *)((char *)item+htb->loffset))->key, item)==0)
	 	err_msg1("the same key exists");
      if (ni) {
         item = (void *)((char *)ni-htb->loffset);
      } else {
         htb->walkptr = NULL;
         item = htable_get_next(htb);
      }
   }
   if (htb->num_items != big->num_items) {
	  printf("%s,%d,grow table wrong  %u != %u \n",__FILE__,__LINE__,htb->num_items,big->num_items);
   }
   big->replace_count+=htb->replace_count;
   free((char*)htb->table);
   memcpy(htb, big, sizeof(htable));  /* move everything across */
   free((char*)big);
 
}

/*cut the last one in every bucket*/
/*
*如果hash表已经满了，则一次性将所有访问得比较少的条目删除
*删除的标准是凡是链表长度大于等于2的桶，将最后一个条目删除
*/
void cut_table(htable *tbl){
	void *item;
	hlink *freeptr = NULL;
	int count = 0;
   	for (item = htable_get_first(tbl); item; ) {/*遍历hash表*/
		hlink *ni,*li;
		li = (hlink *)((char *)item+tbl->loffset);
		ni = (hlink *)li->next; 
		if(ni){
			count++;
			if(count <= 2)
		      	item = (void *)((char *)ni-tbl->loffset);	
			else{
				freeptr = ni;
				li->next = NULL;
				tbl->walkptr = NULL;
				item = htable_get_next(tbl);
				count = 0;
				if(freeptr){
					hlink *ni2,*li2;
					li2 = ni2 = freeptr;
					do{/*释放删除条目的内存*/
						ni2 = (hlink *)li2->next;
						free((char *)((char *)li2-tbl->loffset));
						tbl->num_items--;
						li2 = ni2;
					}while(li2);			
					freeptr = NULL;
				}
			}
      	}
	  	else {
          	tbl->walkptr = NULL;
          	item = htable_get_next(tbl);
		  	count = 0;
      	}
	}
}	

/*
*往hash表中插入新的条目, 插入之前应先查询或计算hash 与index
*/
// key must in item
int htable_insert(htable *tbl,unsigned char *key, void *item)
{
   hlink *hp,*ni,*li;
   int rc = 1;
   int count;
	do{
   		if(key == NULL)
   		{
        			rc = 0;
			printf("%s,%d,key==NULL\n",__FILE__,__LINE__);
			break;
   		}
		/*如果在hash表中找到相同关键字的条目，则不必插入，直接返回*/
   		if (htable_lookup(tbl,key)) 
		{
      			rc = 0;  
			//printf("%s,%d,key has existed\n",__FILE__,__LINE__);
	  		break;
   		}
	          /*   ASSERT(tbl->index < tbl->buckets);*/		
		hp = (hlink *)(((char *)item)+tbl->loffset);
		hp->next = tbl->table[tbl->index]; /*some pro*//*将新条目插入到对应桶的链表头*/
  		hp->hash = tbl->hash;
		hp->key=key;
   		//memcpy(hp->key,key,tbl->keylen);
   		tbl->table[tbl->index] = hp;
                /*
		for(li = NULL,ni = tbl->table[tbl->index],count = 0; ni && count < MAX_COUNT; li = ni,ni = ni->next,count++);
		if(ni != NULL){//保证每个桶最多有4个item
			li->next = NULL;
			free((char *)((char *)ni-tbl->loffset));
			--tbl->num_items;
			ni = NULL;
			tbl->replace_count++;
		}
            */
   		if (++tbl->num_items >= tbl->max_items) {
      		  /*cut_table(tbl);如果hash表已满，则降低hash表的规模，即删除一些数据*/
			grow_table(tbl);/*如果hash表已满，则增长hash表的规模，即增加一些空间*/
  		}
		
	}while(0);
   	return rc;
}
//needn't to search
int htable_add(htable *tbl,unsigned char *key, void *item)
{
   hlink *hp,*ni,*li;
   int rc = 1;
   int count;
	do{
   		if(key == NULL)
   		{
        			rc = 0;
			printf("%s,%d,key==NULL\n",__FILE__,__LINE__);
			break;
   		}
		hp = (hlink *)(((char *)item)+tbl->loffset);
		hp->next = tbl->table[tbl->index]; /*some pro*//*将新条目插入到对应桶的链表头*/
  		hp->hash = tbl->hash;
		hp->key=key;
   		
   		tbl->table[tbl->index] = hp;
             
   		if (++tbl->num_items >= tbl->max_items) {
			grow_table(tbl);
  		}
		
	}while(0);
   	return rc;
}


/*
*查询关键字是否在hash表中
*/
void *htable_lookup(htable *tbl,unsigned char *key)
{
   hlink *hp,*head,*last = NULL;
   hash_index(tbl,key);/*求关键字对应的索引*/
   head = tbl->table[tbl->index];
   for (hp=tbl->table[tbl->index]; hp; last = hp,hp=(hlink *)hp->next)
   {

      if (tbl->hash == hp->hash && (memcmp(key, hp->key,tbl->keylen) == 0)) 
      {
		/*if hits,move to head*/
		/*	
		if(hp != head){//如果找到，则将该条目一移至链表的头部
				last->next = hp->next;
				hp->next = head;
				tbl->table[tbl->index] = hp;
			}
		*/
         	return ((char *)hp)-tbl->loffset;
      }
   }
   return NULL;
}
int htable_remove(htable * tbl,unsigned char *key){
	 hlink *hp,*head,*last = NULL;
	   hash_index(tbl,key);/*求关键字对应的索引*/
	   head = tbl->table[tbl->index];
	   for (hp=tbl->table[tbl->index]; hp; last = hp,hp=(hlink *)hp->next)
	   {
		      if (tbl->hash == hp->hash && memcmp(key, hp->key,tbl->keylen) == 0) 
		      {
				/*if hits,delete it*/
				
				if(hp != head){//如果找到
					last->next = hp->next;
					free( ((char *)hp)-tbl->loffset);
				}
				else{
					tbl->table[tbl->index] = (hlink *)hp->next;
					free( ((char *)hp)-tbl->loffset);
				}
				tbl->num_items--;
		         	return SUCCESS;
		      }
	   }
	   return FAILURE;
}

/*
*遍历hash表，获取下一个条目
*/
void *htable_get_next(htable *tbl)
{
   if (tbl->walkptr) {
      tbl->walkptr = (hlink *)(tbl->walkptr->next);
   }
   while (!tbl->walkptr && tbl->walk_index < tbl->buckets) {
      tbl->walkptr = tbl->table[tbl->walk_index++];
   }
   if (tbl->walkptr) {
      return ((char *)tbl->walkptr)-tbl->loffset;
   }
   return NULL;
}

/*
*遍历hash表，获得第一个条目
*/
void *htable_get_first(htable *tbl)
{
   tbl->walkptr = tbl->table[0];    /* get first bucket */
   tbl->walk_index = 1;            /* Point to next index */
   while (!tbl->walkptr && tbl->walk_index < tbl->buckets) {
      tbl->walkptr = tbl->table[tbl->walk_index++];  /* go to next bucket */
   }
   if (tbl->walkptr) {
      return ((char *)tbl->walkptr)-tbl->loffset;
   }
   return NULL;
}

/* Destroy the table and its contents */
/*
*销毁hash表，释放所有条目所占的内存
*/
void htable_destroy(htable *tbl)
{
   void *first,* next;
   first=htable_get_first( tbl);
 while(first){
 	next=htable_get_next( tbl);
   	free(first);
	first=next;
   }
   free((char*)tbl->table);
   tbl->table = NULL;
   free(tbl);
   tbl=NULL;
}

/*=====================================test===============================*/
//#define HTB_TEST
#ifdef HTB_TEST

/*hash表的测试函数*/
typedef struct{
   uint8_t key[21];
   hlink link;
} MYJCR ;

#define NITEMS 80000              /**/
#define HASH_TBALE_SIZE 16777215    /*2^24-1*/



/* total memory is about 528M which will be used mostly ,
but,actually,using about 141M,the season of which,you can understand,I think .*/
int main()
{

   char chunk[200];
   htable *jcrtbl;
   MYJCR *save_jcr = NULL, *item;
   MYJCR *jcr;
   uint32_t i,count = 0,saveid;

   /*jcr = (MYJCR *)malloc(sizeof(MYJCR));*/
   jcrtbl= htable_init((char*)&(jcr->link)-(char*)jcr, 20,1024);
   printf("%u\n",jcrtbl->max_items);

   for (i=0; i < NITEMS; i++) {
       memset(chunk,0, 200);

       sprintf(chunk,"%d EFFD  SED %d thiemdfg456456dgdf %d",i,(i-1)*(i+1),i=i);
       jcr = (MYJCR *)malloc(sizeof(MYJCR));		
	   
	   finger_chunk(chunk,strlen(chunk),jcr->key);
	   jcr->key[20]='\0';
	   //if(htable_lookup(jcrtbl,jcr->key)==NULL)
			htable_insert(jcrtbl,jcr->key, jcr);
	//   else
	   //	printf("same \n");
	   
	   if (i == 100) {
			save_jcr = jcr;
      	}
   	}
   if (!(item = (MYJCR *)htable_lookup(jcrtbl,save_jcr->key))) {
	   printf("not found %s\n",save_jcr->key);
   } else {
      printf("find %s \n",save_jcr->key);
   	}
   htable_get_stats(jcrtbl);
   printf("Walk the hash table:\n");
   foreach_htable (jcr, jcrtbl) {
      count++;
   }
   printf("Got %lu items -- %s\n", count, count==NITEMS? "OK":"there are repeated fingers or the replaced");
   htable_destroy(jcrtbl);
	return 0;
}
#endif









