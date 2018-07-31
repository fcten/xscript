#ifndef LGX_GC_H
#define LGX_GC_H

typedef struct {
    // 引用计数
    unsigned ref_cnt;
} lgx_gc_t;

// 启用垃圾回收
// lgx_gc_enable();

// 禁用垃圾回收
// 对于一些非长期执行的脚本可以选择禁用垃圾回收，因为
// 执行任何脚本所使用的内存会在脚本执行完毕时彻底释放
// lgx_gc_disable();

// 分配一个变量
// lgx_gc_alloc()

// 释放一个变量
// lgx_gc_free()

// 引用计数加一
// lgx_gc_refer()

// 引用计数减一
// lgx_gc_release()

// 执行一次垃圾回收
// 所有的垃圾回收操作均由该方法触发
// 该方法会自行判断是否执行垃圾回收，以及执行何种程度的垃圾回收
// lgx_gc_trigger()

#endif // LGX_GC_H