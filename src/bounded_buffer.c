#include "bounded_buffer.h"

#include <errno.h>
#include <stdlib.h>

int bounded_buffer_init(BoundedBuffer *buffer, size_t capacity)
{
    if (buffer == NULL || capacity == 0)
        return EINVAL;

    buffer->items = malloc(capacity * sizeof(*buffer->items));
    if (buffer->items == NULL)
        return ENOMEM;

    buffer->capacity = capacity;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    buffer->closed = 0;

    int rc = pthread_mutex_init(&buffer->mutex, NULL);
    if (rc != 0) {
        free(buffer->items);
        return rc;
    }

    rc = pthread_cond_init(&buffer->not_empty, NULL);
    if (rc != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->items);
        return rc;
    }

    rc = pthread_cond_init(&buffer->not_full, NULL);
    if (rc != 0) {
        pthread_cond_destroy(&buffer->not_empty);
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->items);
        return rc;
    }

    return 0;
}

int bounded_buffer_push(BoundedBuffer *buffer, int item)
{
    if (buffer == NULL)
        return EINVAL;

    int rc = pthread_mutex_lock(&buffer->mutex);
    if (rc != 0)
        return rc;

    while (buffer->count == buffer->capacity && !buffer->closed) {
        rc = pthread_cond_wait(&buffer->not_full, &buffer->mutex);
        if (rc != 0) {
            pthread_mutex_unlock(&buffer->mutex);
            return rc;
        }
    }

    if (buffer->closed) {
        pthread_mutex_unlock(&buffer->mutex);
        return ECANCELED;
    }

    buffer->items[buffer->tail] = item;
    buffer->tail = (buffer->tail + 1) % buffer->capacity;
    buffer->count++;

    pthread_cond_signal(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);
    return 0;
}

int bounded_buffer_pop(BoundedBuffer *buffer, int *item)
{
    if (buffer == NULL || item == NULL)
        return -EINVAL;

    int rc = pthread_mutex_lock(&buffer->mutex);
    if (rc != 0)
        return -rc;

    while (buffer->count == 0 && !buffer->closed) {
        rc = pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
        if (rc != 0) {
            pthread_mutex_unlock(&buffer->mutex);
            return -rc;
        }
    }

    if (buffer->count == 0 && buffer->closed) {
        pthread_mutex_unlock(&buffer->mutex);
        return 0;
    }

    *item = buffer->items[buffer->head];
    buffer->head = (buffer->head + 1) % buffer->capacity;
    buffer->count--;

    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);
    return 1;
}

int bounded_buffer_close(BoundedBuffer *buffer)
{
    if (buffer == NULL)
        return EINVAL;

    int rc = pthread_mutex_lock(&buffer->mutex);
    if (rc != 0)
        return rc;

    buffer->closed = 1;
    pthread_cond_broadcast(&buffer->not_empty);
    pthread_cond_broadcast(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);
    return 0;
}

void bounded_buffer_destroy(BoundedBuffer *buffer)
{
    if (buffer == NULL)
        return;

    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);
    pthread_mutex_destroy(&buffer->mutex);
    free(buffer->items);
    buffer->items = NULL;
}
