#ifndef LGX_VAL_H
#define LGX_VAL_H

#include "list.h"
#include "str.h"
#include "fun.h"

typedef void lgx_arr_t;
typedef void lgx_obj_t;
typedef void lgx_res_t;
typedef void lgx_ref_t;

enum {
    T_UNDEFINED = 0,// 变量尚未定义
    T_UNINITIALIZED,// 变量尚未初始化
    T_LONG,         // 64 位有符号整数
    T_DOUBLE,       // 64 位有符号浮点数
    T_BOOL,
    T_STRING,
    T_ARRAY,
    T_OBJECT,
    T_RESOURCE,
    T_REDERENCE,
    T_FUNCTION,
    // --------
    T_REGISTER,     // 临时寄存器，仅在编译时使用
    T_IDENTIFIER    // 变量寄存器，仅在编译时使用
};

typedef struct {
    union {
        long long         l; // 64 位有符号整数
        double            d; // 64 位有符号浮点数
        lgx_str_t        *str;
        lgx_arr_t        *arr;
        lgx_obj_t        *obj;
        lgx_res_t        *res;
        lgx_ref_t        *ref;
        lgx_fun_t        *fun;
    } v;
    unsigned type:4;
    union {
        lgx_str_ref_t name; // 变量名称
        unsigned char reg;  // 分配的寄存器
    } u;
} lgx_val_t;

typedef struct {
    lgx_list_t head;
    lgx_val_t *v;
} lgx_val_list_t;

void lgx_val_print(lgx_val_t *v);

#endif // LGX_VAR_H