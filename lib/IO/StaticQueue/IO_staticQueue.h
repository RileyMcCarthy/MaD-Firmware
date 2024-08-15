#ifndef __STATIC_QUEUE_H__
#define __STATIC_QUEUE_H__
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct IO_staticQueue_t
{
    uint8_t *buf;
    int front;
    int rear;
    int size;
    int max_size;
    int item_size;
    int _lock;
} IO_staticQueue;

bool IO_staticQueue_init(IO_staticQueue *queue, void *buf, int max_size, int item_size);
bool IO_staticQueue_init_lock(IO_staticQueue *queue, void *buf, int max_size, int item_size, int _lock);
bool IO_staticQueue_push(IO_staticQueue *queue, void *data);
bool IO_staticQueue_pop(IO_staticQueue *queue, void *data);
void IO_staticQueue_empty(IO_staticQueue *queue);
bool IO_staticQueue_isempty(IO_staticQueue *queue);
bool IO_staticQueue_isfull(IO_staticQueue *queue);
int32_t IO_staticQueue_count(IO_staticQueue *queue);

#endif // __STATIC_QUEUE_H__
