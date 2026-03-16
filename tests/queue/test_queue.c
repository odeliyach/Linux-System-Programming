/**
 * @file test_queue.c
 * @brief Comprehensive unit tests for thread-safe queue implementation.
 *
 * Tests cover: basic operations, thread safety, edge cases, stress testing,
 * and performance characteristics.
 */

#define _POSIX_C_SOURCE 200809L
#include "queue/queue.h"
#include "common/logger.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#define NUM_PRODUCERS 4
#define NUM_CONSUMERS 4
#define ITEMS_PER_PRODUCER 1000

typedef struct {
    int id;
    int items_to_handle;
    int *error_count;
} thread_args;

static int test_basic_operations(void);
static int test_single_producer_single_consumer(void);
static int test_multiple_producers_consumers(void);
static int test_stress(void);

/**
 * Test basic enqueue/dequeue operations.
 */
static int test_basic_operations(void)
{
    LOG_INFO("Running test_basic_operations");

    initQueue();

    int val1 = 42;
    int val2 = 100;

    assert(enqueue(&val1) == 0);
    assert(enqueue(&val2) == 0);

    int *result1 = dequeue();
    assert(result1 == &val1);

    int *result2 = dequeue();
    assert(result2 == &val2);
    assert(visited() == 2);

    destroyQueue();

    LOG_INFO("test_basic_operations: PASSED");
    return 0;
}

/**
 * Test single producer and single consumer.
 */
static int test_single_producer_single_consumer(void)
{
    LOG_INFO("Running test_single_producer_single_consumer");

    initQueue();

    const int count = 100;
    int values[100];

    for (int i = 0; i < count; ++i) {
        values[i] = i;
        assert(enqueue(&values[i]) == 0);
    }

    for (int i = 0; i < count; ++i) {
        int *val = dequeue();
        assert(val != NULL);
        assert(*val == i);
    }

    assert(visited() == (size_t)count);

    destroyQueue();

    LOG_INFO("test_single_producer_single_consumer: PASSED");
    return 0;
}

static int producer(void *arg)
{
    thread_args *args = arg;
    for (int i = 0; i < args->items_to_handle; ++i) {
        int *payload = malloc(sizeof(int));
        if (!payload) {
            LOG_ERROR("Producer %d: allocation failed", args->id);
            (*args->error_count)++;
            return -1;
        }
        *payload = args->id * 10000 + i;
        if (enqueue(payload) != 0) {
            LOG_ERROR("Producer %d: enqueue failed", args->id);
            free(payload);
            (*args->error_count)++;
            return -1;
        }
    }
    LOG_DEBUG("Producer %d completed %d items", args->id, args->items_to_handle);
    return 0;
}

static int consumer(void *arg)
{
    thread_args *args = arg;
    for (int i = 0; i < args->items_to_handle; ++i) {
        int *payload = dequeue();
        if (payload) {
            free(payload);
        } else {
            LOG_ERROR("Consumer %d: dequeue returned NULL", args->id);
            (*args->error_count)++;
            return -1;
        }
    }
    LOG_DEBUG("Consumer %d completed %d items", args->id, args->items_to_handle);
    return 0;
}

/**
 * Test multiple producers and consumers.
 */
static int test_multiple_producers_consumers(void)
{
    LOG_INFO("Running test_multiple_producers_consumers");

    initQueue();

    const int total_items = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
    int error_count = 0;

    thread_args producer_args[NUM_PRODUCERS];
    thread_args consumer_args[NUM_CONSUMERS];
    thrd_t producer_threads[NUM_PRODUCERS];
    thrd_t consumer_threads[NUM_CONSUMERS];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        producer_args[i].id = i;
        producer_args[i].items_to_handle = ITEMS_PER_PRODUCER;
        producer_args[i].error_count = &error_count;
        thrd_create(&producer_threads[i], producer, &producer_args[i]);
    }

    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        consumer_args[i].id = i;
        consumer_args[i].items_to_handle = total_items / NUM_CONSUMERS;
        consumer_args[i].error_count = &error_count;
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

    LOG_INFO("Dequeued items: %zu", visited());
    LOG_INFO("Elapsed: %.6f seconds", elapsed);
    LOG_INFO("Throughput: %.2f items/sec", visited() / elapsed);
    LOG_INFO("Errors: %d", error_count);

    assert(error_count == 0);
    assert(visited() == (size_t)total_items);

    destroyQueue();

    LOG_INFO("test_multiple_producers_consumers: PASSED");
    return 0;
}

/**
 * Stress test with high concurrency.
 */
static int test_stress(void)
{
    LOG_INFO("Running test_stress");

    initQueue();

    const int num_producers = 8;
    const int num_consumers = 8;
    const int items_per_producer = 5000;
    const int total_items = num_producers * items_per_producer;
    int error_count = 0;

    thread_args producer_args[8];
    thread_args consumer_args[8];
    thrd_t producer_threads[8];
    thrd_t consumer_threads[8];

    for (int i = 0; i < num_producers; ++i) {
        producer_args[i].id = i;
        producer_args[i].items_to_handle = items_per_producer;
        producer_args[i].error_count = &error_count;
        thrd_create(&producer_threads[i], producer, &producer_args[i]);
    }

    for (int i = 0; i < num_consumers; ++i) {
        consumer_args[i].id = i;
        consumer_args[i].items_to_handle = total_items / num_consumers;
        consumer_args[i].error_count = &error_count;
        thrd_create(&consumer_threads[i], consumer, &consumer_args[i]);
    }

    for (int i = 0; i < num_producers; ++i) {
        thrd_join(producer_threads[i], NULL);
    }
    for (int i = 0; i < num_consumers; ++i) {
        thrd_join(consumer_threads[i], NULL);
    }

    LOG_INFO("Stress test completed: %zu items processed", visited());
    assert(error_count == 0);
    assert(visited() == (size_t)total_items);

    destroyQueue();

    LOG_INFO("test_stress: PASSED");
    return 0;
}

int main(void)
{
    logger_init(LOG_LEVEL_INFO, stdout);

    LOG_INFO("=== Starting Queue Test Suite ===");

    int failures = 0;

    failures += test_basic_operations();
    failures += test_single_producer_single_consumer();
    failures += test_multiple_producers_consumers();
    failures += test_stress();

    if (failures == 0) {
        LOG_INFO("=== All Queue Tests PASSED ===");
        logger_shutdown();
        return EXIT_SUCCESS;
    } else {
        LOG_ERROR("=== %d Queue Tests FAILED ===", failures);
        logger_shutdown();
        return EXIT_FAILURE;
    }
}
