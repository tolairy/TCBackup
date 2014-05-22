#ifndef DList_H
#define DList_H

/* LRU list (double-linked lists)*/
typedef  void*  ItemType;


/*����ڵ�����*/
typedef struct node_t 
{
	ItemType data;		/*������*/
	struct node_t * prev; /*ָ��ǰ��*/
	struct node_t * next;		/*ָ����*/
}Node;

typedef Node*  PNode;

/*������������*/
typedef struct
{
	Node * head;		/*ָ��ͷ�ڵ�*/
	Node * tail;		/*ָ��β�ڵ�*/
	int size;
}DList;

DList * dlist_init();
void dlist_destory(DList * plist);
void dlist_preappend(DList * plist,ItemType itme); // ͷ�巨
void dlist_append(DList * plist,ItemType item);

void dlist_insert(DList * plist,ItemType item);
int  dlist_move_ptr(DList * plist,ItemType item);// �Ƚ�ָ��ֵ
int  dlist_move_value(DList * plist,ItemType item,int size);// �Ƚ�ָ��ָ������ֵ
ItemType dlist_delete_tail(DList * plist);
int dlist_search(DList * plist,ItemType item);

int dlist_size(DList *plist);


#endif
