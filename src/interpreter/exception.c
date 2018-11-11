#include "exception.h"

void lgx_exception_throw(lgx_vm_t *vm, lgx_val_t *e) {
    // TODO 匹配最接近的 try-catch 块

    // TODO 没有匹配的 catch 块，退出当前协程
    if (0) {
        printf("[uncaught exception] [pc: %d] ", vm->co_running->pc-1);

        lgx_val_print(e);

        printf("\n");

        lgx_vm_backtrace(vm);

        //vm->pc = vm->bc_size - 1;
        exit(1);
    }
}