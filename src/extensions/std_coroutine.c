#include "../common/str.h"
#include "../common/fun.h"
#include "std_coroutine.h"

int std_co_create(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;

    unsigned base = vm->regs[0].v.fun->stack_size;
    lgx_fun_t *fun = vm->regs[base+4].v.fun;
    long long l = vm->regs[base+5].v.l;

    lgx_co_t *co = lgx_co_create(vm, fun);
    if (!co) {
        return lgx_ext_return_false(vm->co_running);
    }

    // 写入参数
    co->stack.buf[4].type = T_LONG;
    co->stack.buf[4].v.l = l;

    // 在协程切换前写入返回值
    lgx_ext_return_true(vm->co_running);

    return lgx_co_resume(vm, co);
}

int std_co_yield(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;

    // 在协程切换前写入返回值
    lgx_ext_return_true(vm->co_running);

    return lgx_co_yield(vm);
}

typedef struct {
    wbt_timer_t timer;
    lgx_vm_t *vm;
    lgx_co_t *co;
} co_sleep_ctx;

static wbt_status timer_wakeup(wbt_timer_t *timer) {
    co_sleep_ctx *ctx = (co_sleep_ctx *)timer;

    lgx_co_resume(ctx->vm, ctx->co);

    return WBT_OK;
}

extern time_t wbt_cur_mtime;

int std_co_sleep(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;

    unsigned base = vm->regs[0].v.fun->stack_size;
    long long sleep = vm->regs[base+4].v.l;

    co_sleep_ctx *ctx = (co_sleep_ctx *)xcalloc(1, sizeof(co_sleep_ctx));
    if (!ctx) {
        return lgx_ext_return_false(vm->co_running);
    }
    ctx->timer.on_timeout = timer_wakeup;
    ctx->timer.timeout = wbt_cur_mtime + sleep;
    ctx->vm = vm;
    ctx->co = vm->co_running;

    if (wbt_timer_add(&vm->events->timer, &ctx->timer) != WBT_OK) {
        xfree(ctx);
        return lgx_ext_return_false(vm->co_running);
    }

    // 在协程切换前写入返回值
    lgx_ext_return_true(vm->co_running);

    return lgx_co_suspend(vm);
}

int std_coroutine_load_symbols(lgx_hash_t *hash) {
    lgx_val_t symbol;

    // TODO co_create 支持变长参数
    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(2);
    symbol.v.fun->buildin = std_co_create;

    symbol.v.fun->args[0].type = T_FUNCTION;
    symbol.v.fun->args[1].type = T_LONG;

    if (lgx_ext_add_symbol(hash, "co_create", &symbol)) {
        return 1;
    }

    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(0);
    symbol.v.fun->buildin = std_co_yield;

    if (lgx_ext_add_symbol(hash, "co_yield", &symbol)) {
        return 1;
    }

    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(1);
    symbol.v.fun->buildin = std_co_sleep;

    symbol.v.fun->args[0].type = T_LONG;

    if (lgx_ext_add_symbol(hash, "co_sleep", &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_coroutine_ctx = {
    "std.coroutine",
    std_coroutine_load_symbols
};