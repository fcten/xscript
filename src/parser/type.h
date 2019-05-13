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

typedef struct lgx_value_s     lgx_value_t;
typedef struct lgx_string_s    lgx_string_t;
typedef struct lgx_array_s     lgx_array_t;
typedef struct lgx_struct_s    lgx_struct_t;
typedef struct lgx_interface_s lgx_interface_t;
typedef struct lgx_function_s  lgx_function_t;

struct lgx_value_s {
    // 8 字节
    union {
        long long       l; // 64 位有符号整数
        double          d; // 64 位有符号浮点数
        lgx_string_t    *str;
        lgx_array_t     *arr;
        lgx_struct_t    *obj;
        lgx_interface_t *res;
        lgx_function_t  *ref;
    } v;

    // 以下控制在 4 字节以内
    lgx_val_type_t type:8;
};

typedef struct lgx_val_list_s {
    lgx_list_t head;
    lgx_value_t v;
} lgx_val_list_t;

typedef struct lgx_gc_s {
    lgx_list_t head;

    // 引用计数
    unsigned ref_cnt;

    // GC 标记
    unsigned char color;
} lgx_gc_t;

struct lgx_string_s {
    // GC 信息
    lgx_gc_t gc;

    // 字符串信息
    lgx_str_t string;
};

struct lgx_array_s {
    // GC 信息
    lgx_gc_t gc;

    // 类型
    lgx_value_t key;
    lgx_value_t value;

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

    // 参数列表
    unsigned args_num;
    lgx_list_t args; // lgx_val_list_t

    // 返回值类型
    lgx_value_t ret;

    // 需求的堆栈空间
    unsigned stack_size;

    // 入口地址
    unsigned addr;

    // 结束地址（必然为 RET undefined 语句）
    unsigned end;

    // 内建函数指针
    int (*buildin)(struct lgx_vm_s *vm);
};

#endif // LGX_TYPE_H