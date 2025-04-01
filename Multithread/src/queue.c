/* queue.c */
#include "queue.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static int errorGeneral(char *message) {
    printf("Bad command: %s\n", message);
    return 1;
}

struct queue *make_queue() {
    // TODO: Allocate memory and initialize synchronization mechanisms
    struct queue *q = (struct queue *) malloc(sizeof(struct queue));
    if (q == NULL) {
        errorGeneral("Could not allocate memory to make queue");
        return NULL;
    }
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->queueMutex, NULL);
    q->semaphore = (sem_t *) malloc(sizeof(sem_t));
    if (q->semaphore == NULL) {
        errorGeneral("Failed to allocate semaphore");
        pthread_mutex_destroy(&q->queueMutex);
        free(q);
        return NULL;
    }
    if (sem_init(q->semaphore, 0, 0) != 0) {
        errorGeneral("Failed to initialize semaphore");
        free(q->semaphore);
        pthread_mutex_destroy(&q->queueMutex);
        free(q);
        return NULL;
    }
    return q;
}

void enqueue(struct queue *q, void *item) {
    // TODO: Add item to the queue and update necessary pointers
    pthread_mutex_lock(&q->queueMutex);
    // add item to queue
    struct queue_node *node =
        (struct queue_node *) malloc(sizeof(struct queue_node));
    if (node == NULL) {
        errorGeneral("Could not allocate memory to make new item in queue");
        pthread_mutex_unlock(&q->queueMutex);
        return;
    }
    node->item = item;
    node->next = NULL;          // end of queue
    if (q->head == NULL || q->tail == NULL) {
        q->head = node;
        q->tail = node;
        node->prev = NULL;
    } else {
        q->tail->next = node;
        node->prev = q->tail;
        q->tail = node;
    }
    pthread_mutex_unlock(&q->queueMutex);
    //signal item added to queue
    sem_post(q->semaphore);
    return;
}

void *dequeue(struct queue *q) {
    // TODO: Retrieve an item while ensuring thread safety
    sem_wait(q->semaphore);     //wait for item to be available to take from queue
    pthread_mutex_lock(&q->queueMutex);
    // dequeue item 
    struct queue_node *temp = q->head;
    if (q->head == NULL) {
        // shouldn't happen bc of the semaphore but just in case
        errorGeneral("Could not dequeue, queue is empty");
        // don't have to free temp bc it's NULL
        pthread_mutex_unlock(&q->queueMutex);
        return NULL;
    }
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    pthread_mutex_unlock(&q->queueMutex);
    void *item = temp->item;
    free(temp);
    return item;
}

void destroy_queue(struct queue *q) {
    // TODO: Clean up allocated resources and synchronization mechanisms
    while (q->head != NULL) {
        struct queue_node *temp = q->head;
        q->head = q->head->next;
        free(temp);
    }
    pthread_mutex_destroy(&q->queueMutex);
    sem_destroy(q->semaphore);
    free(q->semaphore);
    free(q);
}
