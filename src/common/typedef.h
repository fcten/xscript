#ifndef LGX_TYPEDEF_H
#define LGX_TYPEDEF_H

#include "list.h"

typedef struct lgx_gc_s {
    // 双向链表
    lgx_list_t head;
    // GC 对象的内存占用，字节
    unsigned size;
    // 引用计数
    // 为了保持内存对齐并且不浪费空间，这里使用 short 类型存储引用计数
    // TODO 如果引用计数超过限制，则复制一个新对象
    unsigned short ref_cnt;
    // 该 gc 所管理的类型
    unsigned char type;
    // GC 标记
    unsigned char color;
} lgx_gc_t;

#endif // LGX_TYPEDEF_H