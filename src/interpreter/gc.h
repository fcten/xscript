#ifndef LGX_GC_H
#define LGX_GC_H

#include "../common/val.h"
#include "vm.h"

typedef struct {
    lgx_list_t head;
    // 该 gc 所管理的类型
    unsigned char type;
    // GC 标记
    unsigned char color;
    // 引用计数
    unsigned ref_cnt;
} lgx_gc_t;

// 启用垃圾回收
void lgx_gc_enable(lgx_vm_t *vm);

// 禁用垃圾回收
// 对于一些非长期执行的脚本可以选择禁用垃圾回收，因为
// 执行任何脚本所使用的内存会在脚本执行完毕时彻底释放
void lgx_gc_disable(lgx_vm_t *vm);

// 把一个变量加入 GC 跟踪
int lgx_gc_trace(lgx_vm_t *vm, lgx_val_t *val);

// 释放一个变量
void lgx_gc_free(lgx_val_t *val);

// 引用计数设置为指定值
#define lgx_gc_ref_set(p, cnt) do {\
    switch ((p)->type) {\
        case T_STRING: (p)->v.str->gc.ref_cnt = cnt; break;\
        case T_ARRAY: (p)->v.arr->gc.ref_cnt = cnt; break;\
    }\
} while(0);

// 引用计数加一
#define lgx_gc_ref_add(p) do {\
    switch ((p)->type) {\
        case T_STRING: (p)->v.str->gc.ref_cnt ++; break;\
        case T_ARRAY: (p)->v.arr->gc.ref_cnt ++; break;\
    }\
} while(0);

// 引用计数减一
#define lgx_gc_ref_del(p) do {\
    switch ((p)->type) {\
        case T_STRING: (p)->v.str->gc.ref_cnt --; break;\
        case T_ARRAY: (p)->v.arr->gc.ref_cnt --; break;\
    }\
} while(0);

#endif // LGX_GC_H