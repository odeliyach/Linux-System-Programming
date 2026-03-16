#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

/**
 * @file queue.c
 * @brief Thread-safe FIFO queue for producer-consumer patterns.
 *
 * Synchronization is provided with a mutex and per-waiter condition variable.
 * If a consumer arrives before an item, it parks on a wait queue and is
 * handed the next enqueued element directly, minimizing unnecessary wakeups.
 */

typedef struct ItemNode {
    void *data;
    struct ItemNode *next;
} ItemNode;

typedef struct WaitNode {
    void *data_transfer;
    cnd_t cond;
    struct WaitNode *next;
    int awakened;
} WaitNode;

static ItemNode *item_head = NULL, *item_tail = NULL;
static WaitNode *wait_head = NULL, *wait_tail = NULL;
static mtx_t queue_lock;
static atomic_size_t visited_count;

/** Initialize the queue state and synchronization primitives. */
void initQueue(void)
{
    item_head = item_tail = NULL;
    wait_head = wait_tail = NULL;
    atomic_store(&visited_count, 0);
    mtx_init(&queue_lock, mtx_plain);
}

/** Destroy queue synchronization primitives. */
void destroyQueue(void)
{
    mtx_destroy(&queue_lock);
}

/** Enqueue an item in a FIFO manner, waking a waiting consumer if present. */
void enqueue(void *item)
{
    mtx_lock(&queue_lock);

    if (wait_head != NULL) {
        WaitNode *w = wait_head;
        wait_head = w->next;
        if (wait_head == NULL) {
            wait_tail = NULL;
        }

        w->data_transfer = item;
        w->awakened = 1;
        cnd_signal(&w->cond);
        mtx_unlock(&queue_lock);
        return;
    }

    ItemNode *node = malloc(sizeof(ItemNode));
    if (!node) {
        mtx_unlock(&queue_lock);
        return;
    }
    node->data = item;
    node->next = NULL;
    if (item_tail == NULL) {
        item_head = item_tail = node;
    } else {
        item_tail->next = node;
        item_tail = node;
    }
    mtx_unlock(&queue_lock);
}

/** Dequeue an item, blocking if the queue is empty. */
void *dequeue(void)
{
    mtx_lock(&queue_lock);

    if (item_head != NULL && wait_head == NULL) {
        ItemNode *node = item_head;
        item_head = node->next;
        if (item_head == NULL) {
            item_tail = NULL;
        }
        void *result = node->data;
        free(node);
        atomic_fetch_add_explicit(&visited_count, 1, memory_order_relaxed);
        mtx_unlock(&queue_lock);
        return result;
    }

    WaitNode self;
    self.next = NULL;
    self.awakened = 0;
    self.data_transfer = NULL;
    cnd_init(&self.cond);

    if (wait_tail == NULL) {
        wait_head = wait_tail = &self;
    } else {
        wait_tail->next = &self;
        wait_tail = &self;
    }
    while (!self.awakened) {
        cnd_wait(&self.cond, &queue_lock);
    }

    void *result = self.data_transfer;
    cnd_destroy(&self.cond);
    atomic_fetch_add_explicit(&visited_count, 1, memory_order_relaxed);
    mtx_unlock(&queue_lock);

    return result;
}

/**
 * @brief Return the number of items that have traversed the queue.
 *
 * The count is incremented when an element is dequeued, providing a
 * non-blocking statistic for monitoring throughput.
 */
size_t visited(void)
{
    return atomic_load_explicit(&visited_count, memory_order_relaxed);
}
