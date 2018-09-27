#include "../common/str.h"
#include "../common/fun.h"
#include "std_coroutine.h"

int std_co_create(void *p) {
    return lgx_ext_return_long(p, 0);
}

int std_co_resume(void *p) {
    return lgx_ext_return_long(p, 0);
}

int std_co_yield(void *p) {
    return lgx_ext_return_long(p, 0);
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

    return 0;
}

lgx_buildin_ext_t ext_std_coroutine_ctx = {
    "std.coroutine",
    std_coroutine_load_symbols
};