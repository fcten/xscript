#ifndef LGX_GC_H
#define LGX_GC_H

#include "../parser/type.h"
#include "vm.h"

#define IS_GC_VALUE(p) ((p)->type > T_BOOL)

// 启用垃圾回收
void lgx_gc_enable(lgx_vm_t *vm);

// 禁用垃圾回收
// 对于一些非长期执行的脚本可以选择禁用垃圾回收，因为
// 执行任何脚本所使用的内存会在脚本执行完毕时彻底释放
void lgx_gc_disable(lgx_vm_t *vm);

// 把一个变量加入 GC 跟踪
int lgx_gc_trace(lgx_vm_t *vm, lgx_value_t *v);

void lgx_gc_cleanup(lgx_gc_t* gc);

#endif // LGX_GC_H