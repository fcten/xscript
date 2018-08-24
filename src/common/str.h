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
    // 是否为引用
    unsigned is_ref;
    // 指针
    char *buffer;
} lgx_str_t;

lgx_str_t* lgx_str_new(char *str, unsigned len);
lgx_str_t* lgx_str_new_ref(char *str, unsigned len);
void lgx_str_delete(lgx_str_t *str);

int lgx_str_cmp(lgx_str_t *str1, lgx_str_t *str2);

#endif // LGX_STR_H