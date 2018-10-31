#include <stdio.h>
#include <assert.h>

#include "wbt_memory.h"
#include "wbt_ringbuffer.h"

// ringbuffer 支持一个生产者和一个消费者并发访问

wbt_queue_t* wbt_queue_create(unsigned size) {
    // 规范化 size 取值
    //size = ALIGN(size);

    wbt_queue_t *rb = wbt_calloc(sizeof(wbt_queue_t) + size * sizeof(unsigned long long));
    if (!rb) {
        return NULL;
    }

    rb->size = size;
    rb->head = rb->tail = 0;

    return rb;
}

int wbt_queue_delete(wbt_queue_t *rb) {
    wbt_free(rb);
    return 0;
}

int wbt_queue_read(wbt_queue_t *rb, wbt_socket_t *val) {
    if (rb->tail < rb->head) {
        *val = rb->buf[rb->tail % rb->size];
        rb->tail ++;
        return 0;
    } else {
        // buffer 是空的
        return 1;
    }
}

int wbt_queue_write(wbt_queue_t *rb, wbt_socket_t val) {
    if (rb->head + 1 >= rb->tail + rb->size) {
        // buffer 已满
        return 1;
    }

    rb->buf[rb->head % rb->size] = val;
    rb->head += 1;

    return 0;
}