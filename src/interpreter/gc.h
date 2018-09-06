#ifndef LGX_GC_H
#define LGX_GC_H

#include "../common/val.h"
#include "vm.h"

// 启用垃圾回收
void lgx_gc_enable(lgx_vm_t *vm);

// 禁用垃圾回收
// 对于一些非长期执行的脚本可以选择禁用垃圾回收，因为
// 执行任何脚本所使用的内存会在脚本执行完毕时彻底释放
void lgx_gc_disable(lgx_vm_t *vm);

// 把一个变量加入 GC 跟踪
int lgx_gc_trace(lgx_vm_t *vm, lgx_val_t *val);

// 获取指定 val 的引用计数
#define lgx_gc_ref_get(n, p) do {\
    if ((p)->type > T_BOOL) {\
        n = (p)->v.gc->ref_cnt;\
    } else {\
        n = 0;\
    }\
} while(0)

// 引用计数设置为指定值
#define lgx_gc_ref_set(p, cnt) do {\
    if ((p)->type > T_BOOL) {\
        (p)->v.gc->ref_cnt = cnt;\
    }\
} while(0)

// 引用计数加一
#define lgx_gc_ref_add(p) do {\
    if ((p)->type > T_BOOL) {\
        (p)->v.gc->ref_cnt ++;\
    }\
} while(0)

// 引用计数减一
#define lgx_gc_ref_del(p) do {\
    if ((p)->type > T_BOOL) {\
        (p)->v.gc->ref_cnt --;\
        if ((p)->v.gc->ref_cnt == 0) {\
            lgx_val_free(p);\
        }\
    }\
} while(0)

#endif // LGX_GC_H