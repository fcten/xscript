#ifndef WBT_RING_BUFFER_H
#define WBT_RING_BUFFER_H

#include "../webit.h"

typedef struct {
    unsigned size;
    unsigned long long head;
    unsigned long long tail;
    wbt_socket_t buf[];
} wbt_queue_t;

wbt_queue_t* wbt_queue_create(unsigned size);
int wbt_queue_delete(wbt_queue_t *queue);

int wbt_queue_read(wbt_queue_t *queue, wbt_socket_t *val);
int wbt_queue_write(wbt_queue_t *queue, wbt_socket_t val);

#endif // WBT_RING_BUFFER_H