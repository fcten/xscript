#ifndef LGX_VM_H
#define LGX_VM_H

#include "../common/val.h"
#include "../compiler/compiler.h"

typedef struct {
    // 字节码
    unsigned *bc;
    unsigned bc_size;

    // 运行时堆栈
    lgx_val_t *stack;
    // 堆栈总长度
    unsigned int stack_size;
    // 当前函数正在使用的堆栈起始地址
    unsigned int stack_base;

    // 寄存器组 (指向堆栈)
    lgx_val_t *regs;

    // 常量表
    lgx_hash_t *constant;

    // 全局变量
    lgx_hash_t *global;

    // 程序计数
    unsigned pc;
} lgx_vm_t;

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc);
int lgx_vm_start(lgx_vm_t *vm);

#endif // LGX_VM_H