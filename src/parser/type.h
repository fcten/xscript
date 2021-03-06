#ifndef LGX_TYPE_H
#define LGX_TYPE_H

#include "../common/list.h"
#include "../common/ht.h"
#include "../common/str.h"

typedef enum lgx_val_type_e {
    T_UNKNOWN = 0,  // 未知类型
    T_CUSTOM,       // 自定义类型
    T_NULL,
    T_LONG,         // 64 位有符号整数
    T_DOUBLE,       // 64 位有符号浮点数
    T_BOOL,
    T_STRING,
    T_ARRAY,
    T_MAP,
    T_STRUCT,
    T_INTERFACE,
    T_FUNCTION
} lgx_val_type_t;

typedef struct lgx_type_s      lgx_type_t;
typedef struct lgx_string_s    lgx_string_t;
typedef struct lgx_array_s     lgx_array_t;
typedef struct lgx_map_s       lgx_map_t;
typedef struct lgx_struct_s    lgx_struct_t;
typedef struct lgx_interface_s lgx_interface_t;
typedef struct lgx_function_s  lgx_function_t;
typedef struct lgx_gc_s        lgx_gc_t;

typedef union {
    long long       l; // 64 位有符号整数
    double          d; // 64 位有符号浮点数
    lgx_string_t    *str;
    lgx_array_t     *arr;
    lgx_map_t       *map;
    lgx_struct_t    *sru;
    lgx_interface_t *itf;
    lgx_function_t  *fun;
    lgx_gc_t        *gc;
} lgx_v_t;

typedef struct lgx_value_s {
    // 8 字节
    lgx_v_t v;

    lgx_val_type_t type;
} lgx_value_t;

typedef struct lgx_val_list_s {
    lgx_list_t head;
    lgx_value_t v;
} lgx_val_list_t;

struct lgx_type_s {
    // 基础类型
    lgx_val_type_t type;

    // 类型字面量
    lgx_str_t literal;

    union {
        struct lgx_type_array_s     *arr;
        struct lgx_type_map_s       *map;
        struct lgx_type_struct_s    *sru;
        struct lgx_type_interface_s *itf;
        struct lgx_type_function_s  *fun;
    } u;
};

struct lgx_gc_s {
    lgx_list_t head;

    // 类型
    lgx_type_t type;

    // 引用计数
    //unsigned ref_cnt;

    // GC 标记
    unsigned char color;
};

typedef struct lgx_type_array_s {
    // 值类型
    lgx_type_t value;
} lgx_type_array_t;

typedef struct lgx_type_map_s {
    // 键类型
    lgx_type_t key;
    // 值类型
    lgx_type_t value;
} lgx_type_map_t;

typedef struct lgx_type_struct_s {
    // 属性
    lgx_ht_t properties;

    // 方法
    lgx_ht_t methods;
} lgx_type_struct_t;

typedef struct lgx_type_interface_s {
    // 方法
    lgx_ht_t methods;
} lgx_type_interface_t;

typedef struct lgx_type_function_s {
    // 函数接收者类型
    lgx_type_t receiver;

    // 返回值类型
    lgx_type_t ret;

    // 参数列表
    unsigned arg_len;
    lgx_type_t* args;
} lgx_type_function_t;

struct lgx_string_s {
    // GC 信息
    lgx_gc_t gc;

    // 字符串信息
    lgx_str_t string;
};

struct lgx_array_s {
    // GC 信息
    lgx_gc_t gc;

    // 哈希表
    lgx_ht_t table;
};

struct lgx_map_s {
    // GC 信息
    lgx_gc_t gc;

    // 哈希表
    lgx_ht_t table;
};

struct lgx_struct_s {
    // GC 信息
    lgx_gc_t gc;

    // 属性
    lgx_ht_t properties;

    // 方法
    lgx_ht_t methods;
};

struct lgx_interface_s {
    // GC 信息
    lgx_gc_t gc;

    // 方法
    lgx_ht_t methods;
};

struct lgx_vm_s;

struct lgx_function_s {
    // GC 信息
    lgx_gc_t gc;

    // 名字
    lgx_str_t name;

    // 需求的堆栈空间
    unsigned stack_size;

    // 入口地址
    unsigned addr;

    // 结束地址（必然为 RET undefined 语句）
    unsigned end;

    // 内建函数指针
    int (*buildin)(struct lgx_vm_s *vm);
};

int lgx_type_to_string(lgx_type_t* type, lgx_str_t* str);

int lgx_type_init(lgx_type_t* type, lgx_val_type_t t);
void lgx_type_cleanup(lgx_type_t* type);

int lgx_type_cmp(lgx_type_t* t1, lgx_type_t* t2);
int lgx_type_dup(lgx_type_t* src, lgx_type_t* dst);
int lgx_type_inference(lgx_type_t* src, lgx_type_t* dst);

int lgx_type_is_definite(lgx_type_t* type);
int lgx_type_is_fit(lgx_type_t* src, lgx_type_t* dst);

#endif // LGX_TYPE_H