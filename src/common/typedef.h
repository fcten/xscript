#ifndef LGX_TYPEDEF_H
#define LGX_TYPEDEF_H

#include "../webit/common/wbt_list.h"

#define IS_BASIC_TYPE(type) ((type) <= T_BOOL)

#define IS_BASIC_VALUE(x) (IS_BASIC_TYPE((x)->type))
#define IS_GC_VALUE(x) (!IS_BASIC_VALUE(x))

typedef enum {
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
} lgx_val_type_t;

// 寄存器类型
typedef enum {
    R_NOT = 0,  // 不是寄存器
    R_TEMP,     // 临时寄存器
    R_LOCAL,    // 局部变量
    R_GLOBAL    // 全局变量
} lgx_reg_type_t;

// 符号表类型

typedef enum {
    S_VARIABLE = 0,
    S_CONSTANT,
    S_FUNCTION,
    S_CLASS
} lgx_symbol_type_t;

// 访问权限
typedef enum {
    P_PACKAGE = 0,
    P_PUBLIC,
    P_PROTECTED,
    P_PRIVATE
} lgx_access_type_t;

// 修饰符
typedef struct {
    // 是否为静态
    unsigned char is_static:1;
    // 是否为常量
    unsigned char is_const:1;
    // 是否为 async
    unsigned char is_async:1;
    // 是否为 abstract
    unsigned char is_abstract:1;
    // 访问权限
    lgx_access_type_t access:2;
} lgx_modifier_t;

typedef struct lgx_val_s  lgx_val_t;
typedef struct lgx_str_s  lgx_str_t;
typedef struct lgx_hash_s lgx_hash_t;
typedef struct lgx_obj_s  lgx_obj_t;
typedef struct lgx_res_s  lgx_res_t;
typedef struct lgx_ref_s  lgx_ref_t;
typedef struct lgx_fun_s  lgx_fun_t;
typedef struct lgx_gc_s   lgx_gc_t;

struct lgx_val_s {
    // 8 字节
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

    // 以下控制在 4 字节以内
    lgx_val_type_t type:8;

    // 所有成员控制在 4 个字节以内
    union {
        // 在符号表中使用
        // 在常量表中使用
        // 在 lgx_exceptipn_block_t 中使用
        struct {
            // 符号类型
            lgx_symbol_type_t type:2;
            // 该符号在定义后是否被使用
            unsigned char is_used:1;
            // 是否为静态
            unsigned char is_static:1;

            // 作为变量时
            // 寄存器类型
            lgx_reg_type_t reg_type:2;
            // 寄存器编号
            unsigned char reg_num;

            // 作为常量、函数、类时，编号从常量表中获取
        } symbol;

        // 在类的属性表中使用
        struct {
            unsigned char is_static:1;
            unsigned char is_const:1;
            lgx_access_type_t access:2;
        } property;

        // 在类的方法表中使用
        struct {
            unsigned char is_static:1;
            unsigned char is_abstract:1;
            lgx_access_type_t access:2;
        } method;

        // 在函数参数列表中使用
        struct {
            // 是否有默认值
            unsigned char init:1;
        } args;
    } u;
};

typedef struct {
    wbt_list_t head;
    lgx_val_t *v;
} lgx_val_list_t;

struct lgx_gc_s {
    // 双向链表
    wbt_list_t head;
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

struct lgx_vm_s;

struct lgx_fun_s {
    // GC 信息
    lgx_gc_t gc;
    // 函数名称
    lgx_str_t name;
    // 是否为异步函数
    unsigned is_async;
    // 需求的堆栈空间
    unsigned stack_size;
    // 入口地址
    unsigned addr;
    // 结束地址（必然为 RET undefined 语句）
    unsigned end;
    // 内建函数指针
    int (*buildin)(struct lgx_vm_s *vm);
    // 返回值类型
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
    // 下一个相同哈希值的元素
    struct lgx_hash_node_s *next;
    // 下一个插入哈希表的元素
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
    lgx_str_t *name;
    // 是否为内置（否则为用户定义）
    unsigned char is_buildin:1;
    // 是否为接口（否则为类）
    unsigned char is_interface:1;
    // 是否为抽象类（否则为普通类）
    unsigned char is_abstract:1;
    // 继承的父类
    lgx_obj_t *parent;
    // 属性
    lgx_hash_t *properties;
    // 方法
    lgx_hash_t *methods;
    // 常量
    lgx_hash_t *constants;
};

struct lgx_res_s {
    // GC 信息
    lgx_gc_t gc;
    // 资源类型
    unsigned type;
    // 资源具体数据
    void *buf;
    // 资源释放方法
    void (*on_delete)(lgx_res_t *res);
};

#endif // LGX_TYPEDEF_H