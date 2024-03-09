//
// Created by Mahamadou DOUMBIA [OML DSI] on 28/02/2024.
//

#ifndef MOSCC_QUEUE_H
#define MOSCC_QUEUE_H

#include <stdlib.h>


typedef struct {
    int rear;
    int front;
    int size;
    int capacity;
    void **data;

    void *(*copy)(const void *);

    void (*destroy)(const void *);
} Queue;

bool queueIsEmpty(Queue* queue) {
    return queue->size == 0;
}
void *queueFront(Queue *queue) {
    if (queue->size == 0) {
        return NULL;
    }
    return queue->data[queue->front];
}

void dequeItem(Queue *queue) {
    if (queue->size == 0) {
        return;
    }
    queue->destroy(queue->data[queue->front]);
    queue->size--;
    queue->front++;
    if (queue->front == queue->capacity) {
        queue->front = 0;
    }
}

void *queueTake(Queue *queue) {
    void * item = queueFront(queue);
    if(item == NULL) return NULL;
    dequeItem(queue);
    return item;
}
void enqueItem(Queue *queue, void *data) {
    if (queue->size == queue->capacity) {
        return;
    }
    queue->size++;
    queue->rear++;
    if (queue->rear == queue->capacity) {
        queue->rear = 0;
    }
    queue->data[queue->rear] = queue->copy(data);
}

Queue *initQueue(int max, void *(*copy)(const void *), void (*destroy)(const void *)) {
    Queue *q = (Queue *) malloc(sizeof(Queue));
    q->data = (void **) malloc(sizeof(*q->data) * max);
    q->capacity = max;
    q->copy = copy;
    q->destroy = destroy;
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    return q;
}


#endif //MOSCC_QUEUE_H
