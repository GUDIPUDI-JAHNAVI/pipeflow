#ifndef BOUNDED_BUFFER_H
#define BOUNDED_BUFFER_H

#include <pthread.h>
#include <stddef.h>

typedef struct {
    int *items;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    int closed;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} BoundedBuffer;

int bounded_buffer_init(BoundedBuffer *buffer, size_t capacity);
int bounded_buffer_push(BoundedBuffer *buffer, int item);
int bounded_buffer_pop(BoundedBuffer *buffer, int *item);
int bounded_buffer_close(BoundedBuffer *buffer);
void bounded_buffer_destroy(BoundedBuffer *buffer);

#endif
