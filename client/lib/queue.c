/*
 * queue.c
 *
 *  Created on: May 21, 2012
 *      Author: fumin
 */
#include "../global.h"

Queue* queue_new() {
    Queue *queue = (Queue*)malloc(sizeof(Queue));
    queue->first = queue->last = 0;
    queue->elem_num = 0;
    return queue;
}

void queue_init(Queue *queue){
    queue->first = queue->last = 0;
    queue->elem_num = 0;
}

void queue_empty(Queue *queue){
    while(queue->elem_num){
        queue_pop(queue);
    }
}

void queue_free(Queue *queue) {
    queue_empty(queue);
    free(queue);
}

void queue_push(Queue *queue, void *element) {
    queue_ele_t *item;

    if ((item = (queue_ele_t *) malloc(sizeof(queue_ele_t))) == 0) {
        puts("Not enough memory!");
        return;
    }
    item->data = element;
    item->next = 0;

    /* Add to end of queue */
    if (queue->first == 0) {
        queue->first = item;
    } else {
        queue->last->next = item;
    }
    queue->last = item;

    ++queue->elem_num;
}

void* queue_pop(Queue *queue) {
    queue_ele_t *item = 0;
    if(queue->elem_num == 0)
        return NULL;

    item = queue->first;

    queue->first = item->next;
    if (queue->last == item)
        queue->last = NULL;
    --queue->elem_num;

    void *ret = item->data;
    free(item);
    return ret;
}

int queue_size(Queue *queue){
    return queue->elem_num;
}


//===================================================
SyncQueue *sync_queue_new(){
    SyncQueue* syncq = (SyncQueue*)malloc(sizeof(SyncQueue));
    queue_init(&syncq->queue);
    syncq->valid = SYNC_QUEUE_VALID;
    pthread_mutex_init(&syncq->mutex, NULL);
    pthread_cond_init(&syncq->work, NULL);
    return syncq;
}

void sync_queue_free(SyncQueue* sync_queue){
    pthread_mutex_lock(&sync_queue->mutex);
    sync_queue->valid = 0;
    pthread_mutex_unlock(&sync_queue->mutex);

    queue_empty(&sync_queue->queue);
    pthread_mutex_destroy(&sync_queue->mutex);
    pthread_cond_destroy(&sync_queue->work);
    free(sync_queue);
}

int sync_queue_size(SyncQueue *sync_queue){
    return queue_size(&sync_queue->queue);
}

void sync_queue_push(SyncQueue *sync_queue, void *elem){
    pthread_mutex_lock(&sync_queue->mutex);
    if(sync_queue->valid != SYNC_QUEUE_VALID){
        pthread_mutex_unlock(&sync_queue->mutex);
        return;
    }

    queue_push(&sync_queue->queue, elem);

    pthread_cond_broadcast(&sync_queue->work);

    pthread_mutex_unlock(&sync_queue->mutex);
}


void* sync_queue_pop(SyncQueue *sync_queue){
    pthread_mutex_lock(&sync_queue->mutex);
    if(sync_queue->valid != SYNC_QUEUE_VALID){
        pthread_mutex_unlock(&sync_queue->mutex);
        return NULL;
    }


    void* elem;
    while((elem = queue_pop(&sync_queue->queue)) == 0){
        pthread_cond_wait(&sync_queue->work, &sync_queue->mutex);
    }

    pthread_mutex_unlock(&sync_queue->mutex);

    return elem;
}

