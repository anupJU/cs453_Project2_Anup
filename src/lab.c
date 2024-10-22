#include<pthread.h>
#include<stdio.h>
#include "../src/lab.h"

// Array of void* to store the queue elements
// Maximum capacity of the queue
// Index of the front element
// Index of the rear element
// Current number of elements in the queue
// Shutdown flag
// Mutex to protect shared data
// Condition variable for signaling
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

// Helper function for acquiring the lock
static void lock_queue(queue_t q)
{
    pthread_mutex_lock(&q->lock);
}

// Helper function for releasing the lock
static void unlock_queue(queue_t q)
{
    pthread_mutex_unlock(&q->lock);
}

// Helper function for waiting on the condition
static void wait_on_condition(queue_t q)
{
    pthread_cond_wait(&q->cond, &q->lock);
}

// Helper function for signaling the condition
static void signal_condition(queue_t q)
{
    pthread_cond_signal(&q->cond);
}

// Initialize a new queue
queue_t queue_init(int capacity)
{
    queue_t q = (queue_t)malloc(sizeof(struct queue));
    if (!q)
    {
        return NULL;
    }
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
    if (q)
    {
        lock_queue(q);
        q->shutdown = true;
        pthread_cond_broadcast(&q->cond);  // Wake up all waiting threads

        unlock_queue(q);
        free(q->data);
        pthread_mutex_destroy(&q->lock);
        pthread_cond_destroy(&q->cond);
        free(q);
    }
}

// Enqueue an element at the rear of the queue
void enqueue(queue_t q, void *data)
{
    lock_queue(q);
    while (q->size == q->capacity && !q->shutdown)
    {
        wait_on_condition(q);  // Wait for space in the queue
    }
    if (q->shutdown)
    {
        unlock_queue(q);
        return;
    }
    q->rear = (q->rear + 1) % q->capacity;
    q->data[q->rear] = data;
    q->size++;
    signal_condition(q); // Signal waiting threads
    unlock_queue(q);
}

// Dequeue an element from the front of the queue
void *dequeue(queue_t q)
{
    lock_queue(q);
    while (q->size == 0 && !q->shutdown)
    {
        wait_on_condition(q);  // Wait for an element to be available
    }
    if (q->shutdown && q->size == 0)
    {
        unlock_queue(q);
        return NULL;
    }
    void *data = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    signal_condition(q);  // Signal waiting threads
    unlock_queue(q);
    return data;
}

// Set the shutdown flag in the queue
void queue_shutdown(queue_t q)
{
    lock_queue(q);
    q->shutdown = true;
    pthread_cond_broadcast(&q->cond); // Wake up all waiting threads
    unlock_queue(q);
}

// Check if the queue is empty
bool is_empty(queue_t q)
{
    lock_queue(q);
    bool empty = (q->size == 0);
    unlock_queue(q);
    return empty;
}

// Check if the queue is shutdown
bool is_shutdown(queue_t q)
{
    lock_queue(q);
    bool shutdown = q->shutdown;
    unlock_queue(q);
    return shutdown;
}
