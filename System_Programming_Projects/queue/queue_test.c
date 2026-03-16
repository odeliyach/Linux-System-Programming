/**
 * @file queue_test.c
 * @brief Simple producer-consumer harness for the queue implementation.
 */

#define _POSIX_C_SOURCE 200809L
#include "queue.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 2
#define ITEMS_PER_PRODUCER 50

typedef struct {
    int id;
    int items_to_handle;
} thread_args;

static int producer(void *arg)
{
    thread_args *args = arg;
    for (int i = 0; i < args->items_to_handle; ++i) {
        int *payload = malloc(sizeof(int));
        if (!payload) {
            return -1;
        }
        *payload = args->id * 1000 + i;
        enqueue(payload);
    }
    return 0;
}

static int consumer(void *arg)
{
    thread_args *args = arg;
    for (int i = 0; i < args->items_to_handle; ++i) {
        int *payload = dequeue();
        if (payload) {
            free(payload);
        }
    }
    return 0;
}

int main(void)
{
    initQueue();

    const int total_items = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
    thread_args producer_args[NUM_PRODUCERS];
    thread_args consumer_args[NUM_CONSUMERS];
    thrd_t producer_threads[NUM_PRODUCERS];
    thrd_t consumer_threads[NUM_CONSUMERS];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        producer_args[i].id = i;
        producer_args[i].items_to_handle = ITEMS_PER_PRODUCER;
        thrd_create(&producer_threads[i], producer, &producer_args[i]);
    }
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        consumer_args[i].id = i;
        consumer_args[i].items_to_handle = total_items / NUM_CONSUMERS;
        thrd_create(&consumer_threads[i], consumer, &consumer_args[i]);
    }

    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        thrd_join(producer_threads[i], NULL);
    }
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        thrd_join(consumer_threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Dequeued items: %zu\n", visited());
    printf("Elapsed: %.6f seconds\n", elapsed);
    printf("Throughput: %.2f items/sec\n", visited() / elapsed);

    destroyQueue();
    return (visited() == (size_t)total_items) ? EXIT_SUCCESS : EXIT_FAILURE;
}
