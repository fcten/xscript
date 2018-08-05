#ifndef LGX_STR_H
#define LGX_STR_H

#include "typedef.h"

typedef struct {
    // GC 信息
    lgx_gc_t gc;

    // 缓冲区长度
    unsigned size;
    // 已使用的缓冲区长度（字符串长度）
    unsigned length;
    // 缓冲区
    char buffer[];
} lgx_str_t;

typedef struct {
    unsigned length;
    char *buffer;
} lgx_str_ref_t;

lgx_str_t* lgx_str_new(char *str, unsigned len);

int lgx_str_cmp(lgx_str_t *str1, lgx_str_t *str2);

#endif // LGX_STR_H