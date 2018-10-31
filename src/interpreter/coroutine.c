#include <assert.h>
#include "coroutine.h"

int lgx_co_stack_init(lgx_co_stack_t *stack, unsigned size) {
    stack->size = size;
    stack->base = 0;
    stack->buf = (lgx_val_t *)xcalloc(stack->size, sizeof(lgx_val_t));
    if (!stack->buf) {
        return 1;
    }
    return 0;
}

int lgx_co_stack_cleanup(lgx_co_stack_t *stack) {
    if (stack->buf) {
        xfree(stack->buf);
        stack->buf = NULL;
    }

    return 0;
}

lgx_co_t* lgx_co_create(lgx_vm_t *vm, lgx_fun_t *fun) {
    if (vm->co_count > LGX_MAX_CO_LIMIT) {
        return NULL;
    }

    lgx_co_t *co = (lgx_co_t *)xcalloc(1, sizeof(lgx_co_t));
    if (!co) {
        return NULL;
    }

    if (lgx_co_stack_init(&co->stack, fun->stack_size)) {
        xfree(co);
        return NULL;
    }

    co->pc = fun->addr;
    co->on_yield = NULL;

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

    vm->co_count ++;

    return co;
}

int lgx_co_delete(lgx_vm_t *vm, lgx_co_t *co) {
    lgx_co_stack_cleanup(&co->stack);
    xfree(co);

    vm->co_count --;

    return 0;
}

int lgx_co_set_on_yield(lgx_co_t *co, int (*on_yield)(struct lgx_vm_s *vm)) {
    return 0;
}

int lgx_co_set_on_resume(lgx_co_t *co, int (*on_yield)(struct lgx_vm_s *vm)) {
    return 0;
}

int lgx_co_has_ready_task(lgx_vm_t *vm) {
    if (!lgx_list_empty(&vm->co_ready)) {
        return 1;
    }

    return 0;
}

int lgx_co_has_task(lgx_vm_t *vm) {
    if (vm->co_running) {
        return 1;
    }

    if (!lgx_list_empty(&vm->co_ready)) {
        return 1;
    }

    if (!lgx_list_empty(&vm->co_suspend)) {
        return 1;
    }

    return 0;
}

int lgx_co_schedule(lgx_vm_t *vm) {
    assert(vm->co_running == NULL);
    assert(lgx_co_has_ready_task(vm));

    lgx_co_t *co = lgx_list_first_entry(&vm->co_ready, lgx_co_t, head);

    return lgx_co_resume(vm, co);
}

int lgx_co_resume(lgx_vm_t *vm, lgx_co_t *co) {
    if (vm->co_running) {
        lgx_co_yield(vm);
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

int lgx_co_yield(lgx_vm_t *vm) {
    if (!vm->co_running) {
        return 1;
    }

    lgx_co_t *co = vm->co_running;
    co->status = CO_READY;

    if (co->on_yield) {
        co->on_yield(vm);
    }

    vm->co_running = NULL;
    lgx_list_add_tail(&co->head, &vm->co_ready);

    return 0;
}

int lgx_co_suspend(lgx_vm_t *vm) {
    if (!vm->co_running) {
        return 1;
    }

    lgx_co_t *co = vm->co_running;
    co->status = CO_SUSPEND;

    if (co->on_yield) {
        co->on_yield(vm);
    }

    vm->co_running = NULL;
    lgx_list_add_tail(&co->head, &vm->co_suspend);

    return 0;
}

int lgx_co_died(lgx_vm_t *vm) {
    if (!vm->co_running) {
        return 1;
    }

    lgx_co_t *co = vm->co_running;
    co->status = CO_DIED;

    if (co->on_yield) {
        co->on_yield(vm);
    }

    vm->co_running = NULL;

    if (0) {
        // 如果协程暂时不能删除
        lgx_list_add_tail(&co->head, &vm->co_died);
    } else {
        lgx_co_delete(vm, co);
    }

    return 0;
}