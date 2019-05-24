#ifndef LGX_VM_H
#define LGX_VM_H

#include "../parser/type.h"
#include "../compiler/compiler.h"

typedef enum {
    CO_READY,
    CO_RUNNING,
    CO_SUSPEND,
    CO_DIED
} lgx_co_status;

typedef struct {
    // 栈内存
    lgx_value_t *buf;
    // 栈总长度
    unsigned int size;
    // 可用栈起始地址
    unsigned int base;
} lgx_co_stack_t;

typedef struct lgx_vm_s lgx_vm_t;

typedef struct lgx_co_s {
    lgx_list_t head;
    // 协程 ID
    unsigned long long id;
    // 协程状态
    lgx_co_status status;
    // 协程栈
    lgx_co_stack_t stack;
    // 程序计数
    unsigned pc;
    // 协程 yield 时触发的回调函数
    int (*on_yield)(lgx_vm_t *vm);
    void *ctx;
    // 所属的虚拟机对象
    lgx_vm_t *vm;
    // 父协程
    struct lgx_co_s *parent;
    // 当前阻塞等待的子协程
    struct lgx_co_s *child;
    // 引用计数
    unsigned ref_cnt;
} lgx_co_t;

struct lgx_vm_s {
    // 字节码
    lgx_compiler_t *c;

    // 协程
    lgx_co_t *co_running;
    lgx_co_t *co_main;
    lgx_list_t co_ready;
    lgx_list_t co_suspend;
    lgx_list_t co_died;
    // 协程创建统计
    unsigned long long co_id;
    // 协程数量统计
    unsigned co_count;

    // 寄存器组 (指向当前协程栈)
    lgx_value_t *regs;

    // 堆内存
    // 新的 value 会加入新生代链表。每当 young_size 超过阈值，会触发一次 Minor GC，
    // 清理该内存区，把其中依然存活的 value 移入老年代。
    // Minor GC 使用 引用计数 算法。
    // 每当 old_size 超过阈值，会触发一次 Full GC。
    // Full GC 使用 标记/清除/压缩 算法
    // 如果 Full GC 触发过于频繁，将会抛出 OutOfMemory 异常。
    struct {
        // 新生代
        lgx_list_t young;
        unsigned young_size;
        // 老年代
        lgx_list_t old;
        unsigned old_size;
    } heap;

    // 常量表
    lgx_ht_t *constant;

    // 异常
    lgx_rb_t *exception;

    // GC 开关
    unsigned gc_enable;
};

int lgx_vm_init(lgx_vm_t *vm, lgx_compiler_t *c);
int lgx_vm_execute(lgx_vm_t *vm);
int lgx_vm_start(lgx_vm_t *vm);
int lgx_vm_cleanup(lgx_vm_t *vm);

void lgx_vm_throw(lgx_vm_t *vm, lgx_value_t *e);
void lgx_vm_throw_s(lgx_vm_t *vm, const char *fmt, ...);
void lgx_vm_throw_v(lgx_vm_t *vm, lgx_value_t *v);

int lgx_vm_checkstack(lgx_vm_t *vm, unsigned int stack_size);

#endif // LGX_VM_H