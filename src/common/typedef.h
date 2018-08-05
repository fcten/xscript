#ifndef LGX_TYPEDEF_H
#define LGX_TYPEDEF_H

#include "list.h"

// sizeof_32 = 16
// sizeof_64 = 24
typedef struct lgx_gc_s {
    // 单向链表指针
    lgx_list_t head;
    // 引用计数
    unsigned ref_cnt:2;
    // 该 gc 所管理的类型
    unsigned char type:4;
    // GC 标记
    unsigned char color:2;
    // GC 对象的内存占用，字节
    unsigned size;
} lgx_gc_t;

#endif // LGX_TYPEDEF_H