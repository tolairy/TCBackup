#include "../global.h"

/*分配值为i的节点，并返回节点地址*/
Node * generate_node(ItemType data)
{
	PNode p = NULL; 
	p = (PNode)malloc(sizeof(Node));
	if(p!=NULL)
	{
		p->data = data;
		p->prev = NULL;
		p->next = NULL;
	}	
	return p;
}


DList * dlist_init(){
	DList *plist = (DList *)malloc(sizeof(DList));
	plist->head=NULL;
	plist->tail=NULL;
	plist->size=0;
	return plist;
}
void dlist_destory(DList * plist){
	PNode p=NULL;
	PNode q=plist->head;
	while(q){
		p=q->next;
		free(q->data);
		free(q);
		q=p;
	}
	free(plist);
}
void dlist_preappend(DList * plist,ItemType item){// 头插法
	
	PNode p=generate_node(item);

	if(plist->head==NULL){
		plist->tail=p;
	}
	else{
		plist->head->prev=p;
		p->next=plist->head;
	}
	plist->head=p;
	plist->size++;
}
void dlist_append(DList * plist,ItemType i){ // 尾插法

	PNode p=generate_node(i);

	if(plist->head==NULL){
		plist->head=p;
	}
	else{
		plist->tail->next=p;
		p->prev=plist->tail;
	}
	plist->tail=p;
	plist->size++;
	
}

void dlist_insert(DList * plist,ItemType item){
	dlist_preappend(plist, item);
}
int dlist_search(DList * plist,ItemType item){
	PNode p=plist->head;
	while(p){   
		if(memcmp(p->data,item,4)==0){
			if(plist->head==p)
				return 1;
			if(p->prev)
				p->prev->next=p->next;
			else
				plist->head=p->next;
			if(p->next)
				p->next->prev=p->prev;
			else
				plist->tail=p->prev;
			break;
		}
		p=p->next;
	}
	if(p)
		return 1;
	else
		return 0;
}
//LRU
int  dlist_move_value(DList * plist,ItemType item,int size){
	//search
	PNode p=plist->head;
	while(p){   
		if(memcmp(p->data,item,size)==0){
			if(plist->head==p)
				return 1;
			if(p->prev)
				p->prev->next=p->next;
			else
				plist->head=p->next;
			if(p->next)
				p->next->prev=p->prev;
			else
				plist->tail=p->prev;
			break;
		}
		p=p->next;
	}
	if(p){// exist. move it to the head
		p->next=p->prev=NULL;
	
		if(plist->head==NULL){
			plist->tail=p;
		}
		else{
			plist->head->prev=p;
			p->next=plist->head;
		}
		plist->head=p;
		return 1;
	}
	return 0;
}
int  dlist_move_ptr(DList * plist,ItemType item){
	//search
	PNode p=plist->head;
	while(p){   
		if(p->data==item){
			if(plist->head==p)
				return 1;
			if(p->prev)
				p->prev->next=p->next;
			else
				plist->head=p->next;
			if(p->next)
				p->next->prev=p->prev;
			else
				plist->tail=p->prev;
			break;
		}
		p=p->next;
	}
	if(p){// exist. move it to the head
		p->next=p->prev=NULL;
	
		if(plist->head==NULL){
			plist->tail=p;
		}
		else{
			plist->head->prev=p;
			p->next=plist->head;
		}
		plist->head=p;
		return 1;
	}
	return 0;
}

ItemType dlist_delete_tail(DList * plist){
	ItemType data;
	PNode p;
	if(plist->head==NULL)
		return NULL;
	if(plist->tail->prev){
		plist->tail->prev->next=NULL;
		data=plist->tail->data;
		p=plist->tail;
		plist->tail=p->prev;
		free(p);
	}
	else{
		data=plist->tail->data;
		free(plist->tail);
		plist->head=NULL;
		plist->tail=NULL;
	}
	plist->size--;
	return data;
}
int dlist_size(DList *plist){
	return plist->size;
}

void dlist_traver(DList *plist){
	PNode p=plist->head;
	printf("list item :");
	while(p){
		printf("%d  ",*(int *)p->data);
		p=p->next;
	}
	printf("\n");
}

/*
int main(){
	DList *list=dlist_init();
	int i;
	int *save;
	for( i=0;i<100;i++){
		int * p=(int *)malloc(4);
		*p=i;
		save=p;
		if(dlist_size(list)>=20){
			int *q;
			q=dlist_delete_tail(list);
			free(q);
		}
		dlist_append(list,p);
	}
	dlist_traver(list);
	printf("move \n");
	dlist_move_ptr(list,save);
	dlist_traver(list);
	dlist_destory(list);
	
}
*/
