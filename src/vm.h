#ifndef LGX_VM_H
#define LGX_VM_H

#include "val.h"

typedef struct {
    // 字节码
    unsigned *bc;
    unsigned bc_size;

    // 运行时堆栈
    lgx_val_t *stack;
    unsigned short stack_size;
    unsigned short stack_top;

    // 程序计数寄存器
    lgx_val_t *pc;

    // 返回值寄存器
    lgx_val_t *ret;
} lgx_vm_t;

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc);
int lgx_vm_start(lgx_vm_t *vm);

#endif // LGX_VM_H