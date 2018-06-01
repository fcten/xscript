#ifndef LGX_VM_H
#define LGX_VM_H

#include "../common/val.h"

typedef struct {
    // 字节码
    unsigned char *bc;
    unsigned bc_size;

    // 运行时堆栈
    lgx_val_t *stack;
    unsigned short stack_size;
    unsigned short stack_top;

    // 常量表
    lgx_val_t *constant;
    unsigned short constant_top;

    // 程序计数
    unsigned pc;
} lgx_vm_t;

int lgx_vm_init(lgx_vm_t *vm, unsigned char *bc, unsigned bc_size);
int lgx_vm_start(lgx_vm_t *vm);

#endif // LGX_VM_H