/**
 * @file queue_benchmark.c
 * @brief Comprehensive performance benchmarking for thread-safe queue.
 *
 * Measures throughput, latency, and scalability under various workloads.
 */

#define _POSIX_C_SOURCE 200809L
#include "queue/queue.h"
#include "common/logger.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

typedef struct {
    int id;
    int items_to_handle;
} thread_args;

static int producer(void *arg)
{
    thread_args *args = arg;
    for (int i = 0; i < args->items_to_handle; ++i) {
        int *payload = malloc(sizeof(int));
        if (!payload) return -1;
        *payload = args->id * 10000 + i;
        if (enqueue(payload) != 0) {
            free(payload);
            return -1;
        }
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

static void run_benchmark(int num_producers, int num_consumers, int items_per_producer)
{
    LOG_INFO("Benchmark: %d producers, %d consumers, %d items/producer",
             num_producers, num_consumers, items_per_producer);

    initQueue();

    const int total_items = num_producers * items_per_producer;

    thread_args *producer_args = malloc(sizeof(thread_args) * num_producers);
    thread_args *consumer_args = malloc(sizeof(thread_args) * num_consumers);
    thrd_t *producer_threads = malloc(sizeof(thrd_t) * num_producers);
    thrd_t *consumer_threads = malloc(sizeof(thrd_t) * num_consumers);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < num_producers; ++i) {
        producer_args[i].id = i;
        producer_args[i].items_to_handle = items_per_producer;
        thrd_create(&producer_threads[i], producer, &producer_args[i]);
    }

    for (int i = 0; i < num_consumers; ++i) {
        consumer_args[i].id = i;
        consumer_args[i].items_to_handle = total_items / num_consumers;
        thrd_create(&consumer_threads[i], consumer, &consumer_args[i]);
    }

    for (int i = 0; i < num_producers; ++i) {
        thrd_join(producer_threads[i], NULL);
    }
    for (int i = 0; i < num_consumers; ++i) {
        thrd_join(consumer_threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    LOG_INFO("  Items processed: %zu", visited());
    LOG_INFO("  Elapsed: %.6f seconds", elapsed);
    LOG_INFO("  Throughput: %.2f items/sec", visited() / elapsed);
    LOG_INFO("  Avg latency: %.6f ms/item", (elapsed * 1000.0) / visited());

    destroyQueue();

    free(producer_args);
    free(consumer_args);
    free(producer_threads);
    free(consumer_threads);
}

int main(void)
{
    logger_init(LOG_LEVEL_INFO, stdout);

    LOG_INFO("=== Queue Performance Benchmark Suite ===");

    /* Scenario 1: Low concurrency */
    run_benchmark(2, 2, 10000);

    /* Scenario 2: Medium concurrency */
    run_benchmark(4, 4, 10000);

    /* Scenario 3: High concurrency */
    run_benchmark(8, 8, 10000);

    /* Scenario 4: Producer-heavy workload */
    run_benchmark(8, 2, 5000);

    /* Scenario 5: Consumer-heavy workload */
    run_benchmark(2, 8, 5000);

    LOG_INFO("=== Benchmark Suite Complete ===");

    logger_shutdown();
    return EXIT_SUCCESS;
}
