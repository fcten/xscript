#ifndef LGX_VAL_H
#define LGX_VAL_H

typedef void lgx_str_t;
typedef void lgx_arr_t;
typedef void lgx_obj_t;
typedef void lgx_res_t;
typedef void lgx_ref_t;
typedef void lgx_fun_t;

enum {
    T_UNDEFINED = 1,// 变量尚未定义
    T_UNINITIALIZED,// 变量尚未初始化
    T_LONG,         // 64 位有符号整数
    T_DOUBLE,       // 64 位有符号浮点数
    T_STRING,
    T_ARRAY,
    T_OBJECT,
    T_RESOURCE,
    T_REDERENCE,
    T_FUNCTION
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
    } value;
    unsigned type:4;
} lgx_val_t;

#endif // LGX_VAR_H