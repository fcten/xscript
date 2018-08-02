#ifndef LGX_VM_H
#define LGX_VM_H

#include "../common/list.h"
#include "../common/val.h"
#include "../compiler/compiler.h"

typedef struct {
    lgx_list_t head;
    // 栈内存
    lgx_val_t *buf;
    // 栈总长度
    unsigned int size;
    // 可用栈起始地址
    unsigned int base;
} lgx_vm_stack_t;

typedef struct {
    // 字节码
    unsigned *bc;
    unsigned bc_size;

    // 栈内存
    lgx_vm_stack_t stack;
    // 寄存器组 (指向栈)
    lgx_val_t *regs;

    // 堆内存
    // 新的 value 会在新生代内存区中创建。新生代内存区分为 16 段，每当一段内存用完，
    // 会触发一次 Minor GC，清理该内存区，把其中依然存活的 value 移入老年代。
    // Minor GC 使用 引用计数 算法。
    // 每当老年代内存用完时，会触发一次 Full GC。
    // Full GC 使用 标记/清除/压缩 算法
    // 如果 Full GC 触发过于频繁，将会抛出 OutOfMemory 异常。
    struct {
        // 新生代
        lgx_vm_stack_t young;
        // 老年代
        lgx_vm_stack_t old;
    } heap;

    // 常量表
    lgx_hash_t *constant;

    // 全局变量
    lgx_hash_t *global;

    // 程序计数
    unsigned pc;

    // GC 开关
    unsigned gc_enable;
} lgx_vm_t;

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc);
int lgx_vm_start(lgx_vm_t *vm);
int lgx_vm_cleanup(lgx_vm_t *vm);

#endif // LGX_VM_H