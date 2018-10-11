#include "../common/str.h"
#include "../common/fun.h"
#include "std_coroutine.h"

int std_co_create(void *p) {
    lgx_vm_t *vm = p;

    unsigned base = vm->regs[0].v.fun->stack_size;
    lgx_fun_t *fun = vm->regs[base+4].v.fun;

    lgx_co_create(p, fun);

    return lgx_ext_return_long(p, 0);
}

int std_co_resume(void *p) {
    return lgx_ext_return_long(p, 0);
}

int std_co_yield(void *p) {
    lgx_vm_t *vm = p;

    // 在协程切换前写入返回值
    lgx_ext_return_long(p, 0);

    lgx_co_yield(vm);

    return 0;
}

int std_co_status(void *p) {
    return lgx_ext_return_long(p, 0);
}

int std_coroutine_load_symbols(lgx_hash_t *hash) {
    lgx_val_t symbol;

    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(1);
    symbol.v.fun->buildin = std_co_create;

    symbol.v.fun->args[0].type = T_FUNCTION;

    if (lgx_ext_add_symbol(hash, "co_create", &symbol)) {
        return 1;
    }

    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(0);
    symbol.v.fun->buildin = std_co_yield;

    if (lgx_ext_add_symbol(hash, "co_yield", &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_coroutine_ctx = {
    "std.coroutine",
    std_coroutine_load_symbols
};