#include "wbt_thread.h"

int wbt_thread_init(wbt_thread_t *thread) {
    thread->queue = wbt_queue_create(256);
    if (!thread->queue) {
        return 1;
    }

    if (pthread_create(&thread->ntid, NULL, thread->fn, thread) != 0) {
        wbt_queue_delete(thread->queue);
        thread->queue = NULL;
        return 1;
    }

    return 0;
}

int wbt_thread_wait(wbt_thread_t *thread) {
    return pthread_join(thread->ntid, NULL);
}

int wbt_thread_send(wbt_thread_t *thread, wbt_socket_t val) {
    if (wbt_queue_write(thread->queue, val) != 0) {
        return 1;
    }

    pthread_kill(thread->ntid, SIGUSR1);

    return 0;
}

int wbt_thread_recv(wbt_thread_t *thread, wbt_socket_t *val) {
    return wbt_queue_read(thread->queue, val);
}

wbt_thread_pool_t * wbt_thread_create_pool(unsigned size, void* (*fn)(void *thread), void *ctx) {
    wbt_thread_pool_t *pool = wbt_malloc(sizeof(wbt_thread_pool_t) + size * sizeof(wbt_thread_t));
    if (!pool) {
        return NULL;
    }

    pool->size = size;
    pool->count = 0;

    int i;
    for (i = 0; i < size; i ++) {
        pool->threads[i].fn = fn;
        pool->threads[i].ctx = ctx;
        if (wbt_thread_init(&pool->threads[i]) != 0) {
            if (i == 0) {
                wbt_free(pool);
                return NULL;
            } else {
                pool->size = i;
                return pool;
            }
        }
    }

    return pool;
}

wbt_thread_t * wbt_thread_get(wbt_thread_pool_t *pool) {
    return &pool->threads[pool->count ++ % pool->size];
}