/*
 * queue.h
 *
 *  Created on: May 21, 2012
 *      Author: fumin
 */

#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct queue_ele_tag {
	struct queue_ele_tag *next;
	void *data;
} queue_ele_t;

/*
 * Structure describing a queue
 */
typedef struct queue_tag {
    queue_ele_t *first, *last; /* work queue */
    int elem_num;
    //int max_elem_num; //-1 means infi.
} Queue;

#define SYNC_QUEUE_VALID 0x1234

typedef struct {
    Queue queue;
    int valid;
    pthread_mutex_t mutex;
    pthread_cond_t work;
}SyncQueue;

Queue* queue_new();
void queue_free(Queue *queue);
void queue_push(Queue *queue, void *element);
void* queue_pop(Queue *queue);
int queue_size(Queue *queue);

SyncQueue *sync_queue_new();
void sync_queue_free(SyncQueue *syncq);
void sync_queue_push(SyncQueue*, void*);
void* sync_queue_pop(SyncQueue*);
int sync_queue_size(SyncQueue*);
#endif /* QUEUE_H_ */

