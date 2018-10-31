#ifndef LGX_TYPEDEF_H
#define LGX_TYPEDEF_H

#include "list.h"

typedef struct lgx_val_s  lgx_val_t;
typedef struct lgx_str_s  lgx_str_t;
typedef struct lgx_hash_s lgx_hash_t;
typedef struct lgx_obj_s  lgx_obj_t;
typedef struct lgx_res_s  lgx_res_t;
typedef struct lgx_ref_s  lgx_ref_t;
typedef struct lgx_fun_s  lgx_fun_t;
typedef struct lgx_gc_s   lgx_gc_t;

struct lgx_val_s {
    union {
        long long         l; // 64 位有符号整数
        double            d; // 64 位有符号浮点数
        lgx_str_t  *str;
        lgx_hash_t *arr;
        lgx_obj_t  *obj;
        lgx_res_t  *res;
        lgx_ref_t  *ref;
        lgx_fun_t  *fun;
        lgx_gc_t   *gc; // 方便访问任意高级类型的 gc 字段，gc 字段必须在高级类型结构体的头部
    } v;
    unsigned char type;
    // 这个 union 结构有 7 个字节可用
    union {
        // 变量所使用的寄存器，仅在编译时使用
        struct {
            // 寄存器类型
            unsigned char type;
            // 寄存器编号
            unsigned char reg;
            // 是否使用过
            unsigned char used;
            // 是否有默认值(作为函数参数)
            unsigned char init;
        } c;
        // 调试信息，运行时使用
    } u;
};

typedef struct {
    lgx_list_t head;
    lgx_val_t *v;
} lgx_val_list_t;

struct lgx_gc_s {
    // 双向链表
    lgx_list_t head;
    // GC 对象的内存占用，字节
    unsigned size;
    // 引用计数
    unsigned ref_cnt;
    // 该 gc 所管理的类型
    unsigned char type;
    // GC 标记
    unsigned char color;
};

struct lgx_str_s {
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
};

struct lgx_fun_s {
    // GC 信息
    lgx_gc_t gc;
    // 函数名称
    lgx_str_t name;
    // 需求的堆栈空间
    unsigned stack_size;
    // 入口地址
    unsigned addr;
    // 内建函数指针
    int (*buildin)(void *vm);
    // 返回值
    lgx_val_t ret;
    // 参数列表
    unsigned args_num;
    lgx_val_t args[];
};

struct lgx_ref_s {
    // GC 信息
    lgx_gc_t gc;
    // 所引用的变量
    lgx_val_t v;
};

typedef struct lgx_hash_node_s {
    struct lgx_hash_node_s *next;
    struct lgx_hash_node_s *order;
    lgx_val_t k;
    lgx_val_t v;
} lgx_hash_node_t;

// TODO lgx_hash_t 太大了 size=8 时需要多达 416 字节。
// lgx_hash_t 32 字节
// lgx_hash_node_t 48 字节
struct lgx_hash_s {
    // GC 信息
    lgx_gc_t gc;

    // 总容量
    unsigned size;

    // 已使用的容量
    unsigned length;

    // 是否包含非基本类型元素
    unsigned char flag_non_basic_elements;

    // 是否包含非连续元素
    unsigned char flag_non_compact_elements;

    // 按插入顺序保存
    lgx_hash_node_t* head;
    lgx_hash_node_t* tail;

    // 存储数据的结构
    lgx_hash_node_t* table;
};

struct lgx_obj_s {
    // GC 信息
    lgx_gc_t gc;
    // 类名称
    lgx_str_t name;
    // 是否为内置（否则为用户定义）
    char is_buildin;
    // 是否为接口（否则为类）
    char is_interface;
    // 继承的父类
    lgx_obj_t *parent;
    // 属性 is_static public/protected/private
    lgx_hash_t properties;
    // 方法
    lgx_hash_t methods;
    // 常量
    lgx_hash_t constants;
};

struct lgx_res_s {
    // GC 信息
    lgx_gc_t gc;
};

#endif // LGX_TYPEDEF_H