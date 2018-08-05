#ifndef LGX_TYPEDEF_H
#define LGX_TYPEDEF_H

#include "list.h"

typedef struct lgx_gc_s {
    // 双向链表
    lgx_list_t head;
    // GC 对象的内存占用，字节
    unsigned size;
    // 引用计数
    unsigned ref_cnt;
    // 该 gc 所管理的类型
    unsigned char type;
    // GC 标记
    unsigned char color;
} lgx_gc_t;

#endif // LGX_TYPEDEF_H