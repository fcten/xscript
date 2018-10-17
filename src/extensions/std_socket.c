#include "../common/str.h"
#include "../common/fun.h"
#include "std_coroutine.h"

int std_socket_accept(void *p) {
    lgx_vm_t *vm = p;

    return 0;
}

int std_socket_load_symbols(lgx_hash_t *hash) {
    lgx_val_t symbol;

    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(0);
    symbol.v.fun->buildin = std_socket_accept;

    if (lgx_ext_add_symbol(hash, "accept", &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_socket_ctx = {
    "std.socket",
    std_socket_load_symbols
};