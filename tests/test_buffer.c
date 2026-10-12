#include "bounded_buffer.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define ITEM_COUNT 10000
#define BUFFER_CAPACITY 8

typedef struct {
    BoundedBuffer *buffer;
    int failed;
} TestContext;

static void *producer(void *arg)
{
    TestContext *ctx = arg;

    for (int i = 0; i < ITEM_COUNT; i++) {
        int result = bounded_buffer_push(ctx->buffer, i);

        if (result != 0) {
            ctx->failed = 1;
            bounded_buffer_close(ctx->buffer);
            return NULL;
        }
    }

    bounded_buffer_close(ctx->buffer);
    return NULL;
}

static void *consumer(void *arg)
{
    TestContext *ctx = arg;

    for (int expected = 0; expected < ITEM_COUNT; expected++) {
        int value;
        int result = bounded_buffer_pop(ctx->buffer, &value);

        if (result != 1 || value != expected) {
            ctx->failed = 1;
            return NULL;
        }
    }

    int value;
    if (bounded_buffer_pop(ctx->buffer, &value) != 0) {
        ctx->failed = 1;
    }

    return NULL;
}

int main(void)
{
    BoundedBuffer buffer;
    TestContext ctx = { .buffer = &buffer, .failed = 0 };

    if (bounded_buffer_init(&buffer, BUFFER_CAPACITY) != 0) {
        fprintf(stderr, "FAIL: buffer initialization\n");
        return EXIT_FAILURE;
    }

    pthread_t producer_thread;
    pthread_t consumer_thread;

    if (pthread_create(&producer_thread, NULL, producer, &ctx) != 0) {
        bounded_buffer_destroy(&buffer);
        fprintf(stderr, "FAIL: producer thread creation\n");
        return EXIT_FAILURE;
    }

    if (pthread_create(&consumer_thread, NULL, consumer, &ctx) != 0) {
        bounded_buffer_close(&buffer);
        pthread_join(producer_thread, NULL);
        bounded_buffer_destroy(&buffer);
        fprintf(stderr, "FAIL: consumer thread creation\n");
        return EXIT_FAILURE;
    }

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    bounded_buffer_destroy(&buffer);

    if (ctx.failed) {
        fprintf(stderr, "FAIL: bounded buffer test\n");
        return EXIT_FAILURE;
    }

    printf("PASS: bounded buffer FIFO and concurrent access\n");
    printf("PASS: %d items transferred through capacity-%d buffer\n",
           ITEM_COUNT, BUFFER_CAPACITY);

    return EXIT_SUCCESS;
}
