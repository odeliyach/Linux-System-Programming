/**
 * @file queue.h
 * @brief Thread-safe queue API.
 */

#ifndef SYSTEM_PROGRAMMING_PROJECTS_QUEUE_H
#define SYSTEM_PROGRAMMING_PROJECTS_QUEUE_H

#include <stddef.h>

void initQueue(void);
void destroyQueue(void);
void enqueue(void *item);
void *dequeue(void);
size_t visited(void);

#endif /* SYSTEM_PROGRAMMING_PROJECTS_QUEUE_H */
