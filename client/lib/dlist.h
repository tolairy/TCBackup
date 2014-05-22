#ifndef DList_H
#define DList_H

/* LRU list (double-linked lists)*/
typedef  void*  ItemType;


/*定义节点类型*/
typedef struct node_t 
{
	ItemType data;		/*数据域*/
	struct node_t * prev; /*指向前驱*/
	struct node_t * next;		/*指向后继*/
}Node;

typedef Node*  PNode;

/*定义链表类型*/
typedef struct
{
	Node * head;		/*指向头节点*/
	Node * tail;		/*指向尾节点*/
	int size;
}DList;

DList * dlist_init();
void dlist_destory(DList * plist);
void dlist_preappend(DList * plist,ItemType itme); // 头插法
void dlist_append(DList * plist,ItemType item);

void dlist_insert(DList * plist,ItemType item);
int  dlist_move_ptr(DList * plist,ItemType item);// 比较指针值
int  dlist_move_value(DList * plist,ItemType item,int size);// 比较指针指向内容值
ItemType dlist_delete_tail(DList * plist);
int dlist_search(DList * plist,ItemType item);

int dlist_size(DList *plist);


#endif
