#include "IO_staticQueue.h"
#include <propeller2.h>
#include <string.h>
#include <stdio.h>

bool IO_staticQueue_init(IO_staticQueue *queue, void *buf, int max_size, int item_size)
{
    queue->buf = buf;
    queue->max_size = max_size;
    queue->item_size = item_size;
    queue->front = 0;
    queue->rear = 0;
    queue->_lock = _locknew();
    return queue->_lock != -1;
}

bool IO_staticQueue_init_lock(IO_staticQueue *queue, void *buf, int max_size, int item_size, int _lock)
{
    queue->buf = buf;
    queue->max_size = max_size;
    queue->item_size = item_size;
    queue->front = 0;
    queue->rear = 0;
    queue->_lock = _lock;
    return queue->_lock != -1;
}

bool IO_staticQueue_push(IO_staticQueue *queue, void *data)
{
    if (data == NULL)
    {
        printf("IO_staticQueue_push: data is NULL\n");
        return false;
    }

    while (!_locktry(queue->_lock))
        ;

    if (IO_staticQueue_isfull(queue))
    {
        printf("IO_staticQueue_push: data is FULL\n");
        _lockrel(queue->_lock);
        return false;
    }

    memcpy((void *)&(queue->buf[queue->rear * queue->item_size]), data, queue->item_size);
    queue->rear++;
    if (queue->rear == queue->max_size)
        queue->rear = 0;

    _lockrel(queue->_lock);
    return true;
}

bool IO_staticQueue_pop(IO_staticQueue *queue, void *data)
{
    while (!_locktry(queue->_lock))
        ;
    if (IO_staticQueue_isempty(queue))
    {
        _lockrel(queue->_lock);
        return false;
    }

    if (data != NULL)
        memcpy(data, &(queue->buf[queue->item_size * queue->front]), queue->item_size);
    queue->front++;
    if (queue->front == queue->max_size)
        queue->front = 0;

    _lockrel(queue->_lock);
    return true;
}

bool IO_staticQueue_isempty(IO_staticQueue *queue)
{
    return queue->rear == queue->front;
}

bool IO_staticQueue_isfull(IO_staticQueue *queue)
{
    return queue->rear == queue->front - 1 || (queue->front == 0 && queue->rear == queue->max_size - 1);
}

void IO_staticQueue_empty(IO_staticQueue *queue)
{
    while (!_locktry(queue->_lock))
        ;
    queue->front = 0;
    queue->rear = 0;
    _lockrel(queue->_lock);
}

int32_t IO_staticQueue_count(IO_staticQueue *queue)
{
    while (!_locktry(queue->_lock))
        ;
    int count = 0;
    if (queue->rear >= queue->front)
    {
        count = queue->rear - queue->front;
    }
    else
    {
        count = queue->max_size - queue->front + queue->rear;
    }
    _lockrel(queue->_lock);
    return count;
}
