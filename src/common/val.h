#ifndef LGX_VAL_H
#define LGX_VAL_H

#include "typedef.h"

#define IS_BASIC_VALUE(x) ((x)->type <= T_BOOL)
#define IS_GC_VALUE(x) ((x)->type > T_BOOL)

enum {
    // 基本类型
    T_UNDEFINED = 0,// 变量尚未定义
    T_LONG,         // 64 位有符号整数
    T_DOUBLE,       // 64 位有符号浮点数
    T_BOOL,         // T_BOOL 必须是基本类型的最后一种，因为它被用于是否为基本类型的判断
    // 高级类型
    T_REDERENCE,
    T_STRING,
    T_ARRAY,
    T_OBJECT,
    T_FUNCTION,
    T_RESOURCE
};

// 寄存器类型
enum {
    R_NOT = 0,  // 不是寄存器
    R_TEMP,     // 临时寄存器
    R_LOCAL,    // 局部变量
    R_GLOBAL    // 全局变量
};

const char *lgx_val_typeof(lgx_val_t *v);

void lgx_val_print(lgx_val_t *v);

void lgx_val_copy(lgx_val_t *src, lgx_val_t *dest);

#define lgx_val_init(src) do {\
    memset((src), 0, sizeof(lgx_val_t)); \
} while (0);

void lgx_val_free(lgx_val_t *src);

int lgx_val_cmp(lgx_val_t *src, lgx_val_t *dst);

#endif // LGX_VAR_H