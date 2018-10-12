#include "coroutine.h"

int lgx_co_stack_init(lgx_co_stack_t *stack, unsigned size) {
    stack->size = size;
    stack->base = 0;
    stack->buf = xcalloc(stack->size, sizeof(lgx_val_t));
    if (!stack->buf) {
        return 1;
    }
    return 0;
}

lgx_co_t* lgx_co_create(lgx_vm_t *vm, lgx_fun_t *fun) {
    lgx_co_t *co = xcalloc(1, sizeof(lgx_co_t));
    if (!co) {
        return NULL;
    }

    if (lgx_co_stack_init(&co->stack, 256)) {
        xfree(co);
        return NULL;
    }

    co->pc = fun->addr;

    co->status = CO_READY;
    lgx_list_add_tail(&co->head, &vm->co_ready);

    lgx_val_t *R = co->stack.buf;

    // 初始化函数
    R[0].type = T_FUNCTION;
    R[0].v.fun = fun;
    // 写入返回值地址
    R[1].type = T_LONG;
    R[1].v.l = 1;
    // 写入返回地址
    R[2].type = T_LONG;
    R[2].v.l = -1;
    // 写入堆栈地址
    R[3].type = T_LONG;
    R[3].v.l = 0;

    return co;
}

int lgx_co_yield(lgx_vm_t *vm) {
    if (lgx_list_empty(&vm->co_ready)) {
        return 1;
    }

    lgx_co_t *co = lgx_list_first_entry(&vm->co_ready, lgx_co_t, head);

    return lgx_co_resume(vm, co);
}

int lgx_co_resume(lgx_vm_t *vm, lgx_co_t *co) {
    if (vm->co_running) {
        vm->co_running->status = CO_READY;
        lgx_list_add_tail(&vm->co_running->head, &vm->co_ready);
    }

    if (!lgx_list_empty(&co->head)) {
        lgx_list_del(&co->head);
    }
    lgx_list_init(&co->head);

    co->status = CO_RUNNING;
    vm->co_running = co;

    vm->regs = co->stack.buf + co->stack.base;

    return 0;
}

int lgx_co_suspend(lgx_vm_t *vm) {
    if (vm->co_running) {
        vm->co_running->status = CO_SUSPEND;
        lgx_list_add_tail(&vm->co_running->head, &vm->co_suspend);
        vm->co_running = NULL;
    }

    return 0;
}