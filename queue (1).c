#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

typedef struct ItemNode {
    void *data;
    struct ItemNode *next;
} ItemNode;

typedef struct WaitNode {
    void *data_transfer; // העברת האיבר ישירות לthread מחכה
    cnd_t cond;
    struct WaitNode *next;
    int awakened;
} WaitNode;

static ItemNode *item_head = NULL, *item_tail = NULL;
static WaitNode *wait_head = NULL, *wait_tail = NULL;
static mtx_t queue_lock;
static atomic_size_t visited_count;

/*initialize the items queue*/
void initQueue(void) {
    item_head = item_tail = NULL;
    wait_head = wait_tail = NULL;
    atomic_store(&visited_count, 0);
    mtx_init(&queue_lock, mtx_plain);
}
/*cleanup when the items queue is no longer needed*/
void destroyQueue(void) {
    mtx_destroy(&queue_lock); /*No need to free nodes because the queue is guaranteed empty and no threads are waiting at this point*/
}
/*Adds an item to the queue*/
void enqueue(void *item) {
    mtx_lock(&queue_lock);

    if (wait_head != NULL) 
    { /*If threads are waiting, hand the item to them FIFO*/
        WaitNode *w = wait_head;
        wait_head = w->next;
        if (wait_head == NULL) wait_tail = NULL;

        w->data_transfer = item; // העברה ישירה
        w->awakened = 1;
        cnd_signal(&w->cond);
        mtx_unlock(&queue_lock);
        return;
    }

    /*אם אין מחכים, נוסיף לתור הרגיל*/ 
    ItemNode *node = malloc(sizeof(ItemNode));
    node->data = item;
    node->next = NULL;
    if (item_tail == NULL) 
    {
        item_head = item_tail = node;
    } 
    else 
    {
        item_tail->next = node;
        item_tail = node;
    }
    mtx_unlock(&queue_lock);
}
/*Remove an item from the queue. Will block if empty.*/
void *dequeue(void) {
    mtx_lock(&queue_lock);

    
    if (item_head != NULL && wait_head == NULL)/*אם יש פריטים בתור הרגיל ואין threads שמחכים, פשוט לוקחים את הפריט הראשון מהתור הרגיל*/
     {
        ItemNode *node = item_head;
        item_head = node->next;
        if (item_head == NULL) item_tail = NULL;
        void *result = node->data;
        free(node);
        visited_count++;
        mtx_unlock(&queue_lock);
        return result;
    }
    /*Will block if empty*/
    /* התור ריק או שיש כבר threads שמחכים*/ 
    WaitNode self;
    self.next = NULL;
    self.awakened = 0;
    self.data_transfer = NULL;
    cnd_init(&self.cond);

    if (wait_tail == NULL) 
    {
        wait_head = wait_tail = &self;
    }
     else
      {
        wait_tail->next = &self;
        wait_tail = &self;
    }
    while (!self.awakened)/*Thread נכנס למצב חסום, מחכה ל־enqueue שיעביר לו פריט*/
    {
        cnd_wait(&self.cond, &queue_lock);
    }

    void *result = self.data_transfer;
    cnd_destroy(&self.cond);
    visited_count++;
    mtx_unlock(&queue_lock);
    
    return result;
}
/*Return the number of items that have passed inside the queue: return the total count of items
that have been both enqueued and subsequently dequeued. This should not block due to concurrent
operations*/
size_t visited(void) {
    return atomic_load(&visited_count);
}