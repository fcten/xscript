#ifndef LGX_TYPE_H
#define LGX_TYPE_H

typedef enum lgx_val_type_e {
    // 基本类型
    T_UNKNOWN = 0,  // 未知类型
    T_LONG,         // 64 位有符号整数
    T_DOUBLE,       // 64 位有符号浮点数
    T_BOOL,         // T_BOOL 必须是基本类型的最后一种，因为它被用于是否为基本类型的判断
    // 高级类型
    T_STRING,
    T_ARRAY,
    T_STRUCT,
    T_INTERFACE,
    T_FUNCTION
} lgx_val_type_t;

typedef struct lgx_type_s {
    lgx_val_type_t type;

    union {

    } u;
} lgx_type_t;

#endif // LGX_TYPE_H