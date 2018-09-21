#include "../common/str.h"
#include "../common/fun.h"
#include "std_coroutine.h"

int std_co_create(void *p) {
    lgx_val_t ret;
    ret.type = T_LONG;
    ret.v.l = 0;

    return lgx_ext_return(p, &ret);
}

int std_co_resume(void *p) {
    lgx_val_t ret;
    ret.type = T_LONG;
    ret.v.l = 0;

    return lgx_ext_return(p, &ret);
}

int std_co_yield(void *p) {
    lgx_val_t ret;
    ret.type = T_LONG;
    ret.v.l = 0;

    return lgx_ext_return(p, &ret);
}

int std_co_status(void *p) {
    lgx_val_t ret;
    ret.type = T_LONG;
    ret.v.l = 0;

    return lgx_ext_return(p, &ret);
}

int std_coroutine_load_symbols(lgx_hash_t *hash) {
    lgx_val_t symbol;

    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(0);
    symbol.v.fun->buildin = std_co_create;

    if (lgx_ext_add_symbol(hash, "co_create", &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_coroutine_ctx = {
    "std.coroutine",
    std_coroutine_load_symbols
};