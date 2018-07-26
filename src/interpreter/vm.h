#ifndef LGX_VM_H
#define LGX_VM_H

#include "../common/val.h"
#include "../compiler/compiler.h"

typedef struct lgx_stack_s {
    lgx_list_t head;
    // 当前执行的函数
    lgx_fun_t *fun;
    // 返回地址
    unsigned ret;
    // 返回值
    unsigned char ret_idx;
    // 参数与局部变量
    unsigned short size;
    lgx_val_t stack[];
} lgx_stack_t;

typedef struct {
    // 字节码
    unsigned *bc;
    unsigned bc_size;

    // 运行时堆栈
    lgx_stack_t stack;

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