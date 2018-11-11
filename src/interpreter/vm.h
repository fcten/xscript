#ifndef LGX_VM_H
#define LGX_VM_H

#include "../common/val.h"
#include "../compiler/compiler.h"
#include "../webit/wbt.h"

typedef enum {
    CO_READY,
    CO_RUNNING,
    CO_SUSPEND,
    CO_DIED
} lgx_co_status;

typedef struct {
    // 栈内存
    lgx_val_t *buf;
    // 栈总长度
    unsigned int size;
    // 可用栈起始地址
    unsigned int base;
} lgx_co_stack_t;

typedef struct lgx_vm_s lgx_vm_t;

typedef struct lgx_co_s {
    wbt_list_t head;
    // 协程状态
    lgx_co_status status;
    // 协程栈
    lgx_co_stack_t stack;
    // 程序计数
    unsigned pc;
    // 协程 yield 时触发的回调函数
    int (*on_yield)(lgx_vm_t *vm);
    void *ctx;
} lgx_co_t;

struct lgx_vm_s {
    // 字节码
    lgx_bc_t *bc;

    // 协程
    lgx_co_t *co_running;
    lgx_co_t *co_main;
    wbt_list_t co_ready;
    wbt_list_t co_suspend;
    wbt_list_t co_died;
    // 协程数量统计
    unsigned co_count;

    // 寄存器组 (指向当前协程栈)
    lgx_val_t *regs;

    // 堆内存
    // 新的 value 会加入新生代链表。每当 young_size 超过阈值，会触发一次 Minor GC，
    // 清理该内存区，把其中依然存活的 value 移入老年代。
    // Minor GC 使用 引用计数 算法。
    // 每当 old_size 超过阈值，会触发一次 Full GC。
    // Full GC 使用 标记/清除/压缩 算法
    // 如果 Full GC 触发过于频繁，将会抛出 OutOfMemory 异常。
    struct {
        // 新生代
        wbt_list_t young;
        unsigned young_size;
        // 老年代
        wbt_list_t old;
        unsigned old_size;
    } heap;

    // 常量表
    lgx_hash_t *constant;

    // GC 开关
    unsigned gc_enable;

    // 事件池
    wbt_event_pool_t *events;
};

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc);
int lgx_vm_execute(lgx_vm_t *vm);
int lgx_vm_start(lgx_vm_t *vm);
int lgx_vm_cleanup(lgx_vm_t *vm);
int lgx_vm_backtrace(lgx_vm_t *vm);
void lgx_vm_throw(lgx_vm_t *vm, lgx_val_t *e);

#endif // LGX_VM_H