#ifndef LGX_REF_H
#define LGX_REF_H

#include "typedef.h"
#include "val.h"

typedef struct lgx_ref_s {
    // GC 信息
    lgx_gc_t gc;
    // 所引用的变量
    lgx_val_t v;
} lgx_ref_t;

#endif // LGX_REF_H