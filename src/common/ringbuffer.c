#include <assert.h>

#include "common.h"
#include "ringbuffer.h"

// ringbuffer 支持一个生产者和一个消费者并发访问

lgx_rb_t* lgx_rb_create(unsigned size) {
    // 规范化 size 取值
    size = ALIGN(size);

    lgx_rb_t *rb = xcalloc(1, sizeof(lgx_rb_t) + size * sizeof(lgx_val_t));
    if (!rb) {
        return NULL;
    }

    rb->size = size;
    rb->head = rb->tail = 0;

    return rb;
}

lgx_val_t* lgx_rb_read(lgx_rb_t *rb) {
    if (rb->tail < rb->head) {
        return &rb->buf[rb->tail % rb->size];
    } else {
        // buffer 是空的
        return NULL;
    }
}

int lgx_rb_remove(lgx_rb_t *rb) {
    assert(rb->tail < rb->head);

    rb->tail ++;
    return 0;
}

int lgx_rb_write(lgx_rb_t *rb, lgx_val_t *val) {
    if (rb->head + 1 >= rb->tail + rb->size) {
        // buffer 已满
        return 1;
    }

    rb->buf[rb->head % rb->size] = *val;
    rb->head += 1;

    return 0;
}