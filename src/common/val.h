#ifndef LGX_VAL_H
#define LGX_VAL_H

#include "typedef.h"

const char *lgx_val_typeof(lgx_val_t *v);

void lgx_val_print(lgx_val_t *v);

void lgx_val_copy(lgx_val_t *src, lgx_val_t *dest);

#define lgx_val_init(src) do {\
    memset((src), 0, sizeof(lgx_val_t)); \
} while (0);

void lgx_val_free(lgx_val_t *src);

int lgx_val_cmp(lgx_val_t *src, lgx_val_t *dst);

#endif // LGX_VAR_H