#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../src/lab.h"

struct queue
{
    void **data;
    int capacity;
    int front;
    int rear;
    int size;
    bool shutdown;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

typedef struct queue *queue_t;

// Initialize a new queue
queue_t queue_init(int capacity)
{
    queue_t q = (queue_t)malloc(sizeof(struct queue));
    if (!q)
        return NULL;

    q->data = (void **)malloc(sizeof(void *) * capacity);
    if (!q->data)
    {
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    q->shutdown = false;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);

    return q;
}

// Destroy the queue and free all memory
void queue_destroy(queue_t q)
{
    if (!q)
        return;

    pthread_mutex_lock(&q->lock);
    q->shutdown = true;
    pthread_cond_broadcast(&q->cond); // Wake up all waiting threads
    pthread_mutex_unlock(&q->lock);

    free(q->data);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
    free(q);
}

// Enqueue an element at the rear of the queue
void enqueue(queue_t q, void *data)
{
    pthread_mutex_lock(&q->lock);

    while (q->size == q->capacity && !q->shutdown)
    {
        pthread_cond_wait(&q->cond, &q->lock); // Wait for space in the queue

        if (q->shutdown)
        {
            pthread_mutex_unlock(&q->lock);
            return;
        }
    }

    q->rear = (q->rear + 1) % q->capacity;
    q->data[q->rear] = data;
    q->size++;

    pthread_cond_signal(&q->cond); // Signal waiting threads
    pthread_mutex_unlock(&q->lock);
}

// Dequeue an element from the front of the queue
void *dequeue(queue_t q)
{
    pthread_mutex_lock(&q->lock);

    while (q->size == 0 && !q->shutdown)
    {
        pthread_cond_wait(&q->cond, &q->lock); // Wait for an element to be available
    }

    if (q->shutdown && q->size == 0)
    {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    void *data = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    pthread_cond_signal(&q->cond); // Signal waiting threads
    pthread_mutex_unlock(&q->lock);

    return data;
}

// Set the shutdown flag in the queue
void queue_shutdown(queue_t q)
{
    pthread_mutex_lock(&q->lock);
    q->shutdown = true;
    pthread_cond_broadcast(&q->cond); // Wake up all waiting threads
    pthread_mutex_unlock(&q->lock);
}

// Check if the queue is empty
bool is_empty(queue_t q)
{
    pthread_mutex_lock(&q->lock);
    bool empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);
    return empty;
}

// Check if the queue is shutdown
bool is_shutdown(queue_t q)
{
    pthread_mutex_lock(&q->lock);
    ;
    bool shutdown = q->shutdown;
    pthread_mutex_unlock(&q->lock);
    return shutdown;
}