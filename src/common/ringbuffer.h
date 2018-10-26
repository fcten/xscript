#ifndef LGX_RING_BUFFER_H
#define LGX_RING_BUFFER_H

#include "typedef.h"

typedef struct {
    unsigned size;
    unsigned long long head;
    unsigned long long tail;
    lgx_val_t buf[];
} lgx_rb_t;

lgx_rb_t* lgx_rb_create(unsigned size);
int lgx_rb_delete(lgx_rb_t *rb);
lgx_val_t* lgx_rb_read(lgx_rb_t *rb);
int lgx_rb_remove(lgx_rb_t *rb);
int lgx_rb_write(lgx_rb_t *rb, lgx_val_t *val);

#endif // LGX_RING_BUFFER_H