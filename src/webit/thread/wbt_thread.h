#ifndef WBT_THREAD_H
#define WBT_THREAD_H

#include <pthread.h>

#include "../common/wbt_ringbuffer.h"
#include "../event/wbt_event.h"

typedef struct {
    pthread_t ntid;
    void* (*fn)(void *thread);

    wbt_queue_t *queue;
    void *ctx;
} wbt_thread_t;

typedef struct {
    unsigned size;
    unsigned long long count;
    void *ctx;
    wbt_thread_t threads[];
} wbt_thread_pool_t;

int wbt_thread_init(wbt_thread_t *thread);
int wbt_thread_wait(wbt_thread_t *thread);

int wbt_thread_send(wbt_thread_t *thread, wbt_socket_t val);
int wbt_thread_recv(wbt_thread_t *thread, wbt_socket_t *val);

wbt_thread_pool_t * wbt_thread_create_pool(unsigned size, void* (*fn)(void *thread), void *ctx);
wbt_thread_t * wbt_thread_get(wbt_thread_pool_t *pool);

#endif // WBT_THREAD_H