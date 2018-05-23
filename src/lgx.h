#ifndef LGX_H
#define LGX_H

#include "val.h"

typedef struct {
    lgx_val_t *stack;
    unsigned short stack_size;
    unsigned short stack_top;

    lgx_val_t r[4];

    unsigned pc;
} lgx_ctx_t;

int lgx_ctx_init(lgx_ctx_t *ctx);

#endif // LGX_H