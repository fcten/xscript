#ifndef LGX_VAL_H
#define LGX_VAL_H

#include "list.h"
#include "str.h"
#include "fun.h"

typedef void lgx_obj_t;
typedef void lgx_res_t;
typedef void lgx_ref_t;

#define IS_BASIC_VALUE(x) ((x)->type <= T_BOOL)
#define IS_GC_VALUE(x) ((x)->type > T_BOOL)

// 寄存器类型
enum {
    R_NOT = 0,  // 不是寄存器
    R_TEMP,     // 临时寄存器
    R_LOCAL,    // 局部变量
    R_GLOBAL    // 全局变量
};

typedef struct {
    union {
        long long         l; // 64 位有符号整数
        double            d; // 64 位有符号浮点数
        lgx_str_t         *str;
        struct lgx_hash_s *arr;
        lgx_obj_t         *obj;
        lgx_res_t         *res;
        lgx_ref_t         *ref;
        lgx_fun_t         *fun;
        lgx_gc_t          *gc; // 方便访问任意高级类型的 gc 字段，gc 字段必须在高级类型结构体的头部
    } v;
    unsigned char type;
    union {
        // 变量所使用的寄存器类型，仅在编译时使用
        struct {
            // 寄存器类型
            unsigned char type;
            // 寄存器编号
            unsigned char reg;
        } reg;
    } u;
} lgx_val_t;

typedef struct {
    lgx_list_t head;
    lgx_val_t *v;
} lgx_val_list_t;

const char *lgx_val_typeof(lgx_val_t *v);

void lgx_val_print(lgx_val_t *v);

void lgx_val_copy(lgx_val_t *src, lgx_val_t *dest);

#define lgx_val_init(src) do {\
    memset((src), 0, sizeof(lgx_val_t)); \
} while (0);

void lgx_val_free(lgx_val_t *src);

int lgx_val_cmp(lgx_val_t *src, lgx_val_t *dst);

#endif // LGX_VAR_H