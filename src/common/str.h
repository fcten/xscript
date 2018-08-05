#ifndef LGX_STR_H
#define LGX_STR_H

#include "list.h"

typedef struct {
    // GC 信息
    struct {
        lgx_list_t head;
        // 该 gc 所管理的类型
        unsigned char type:4;
        // GC 标记
        unsigned char color:2;
        // 引用计数
        unsigned ref_cnt:26;
    } gc;

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