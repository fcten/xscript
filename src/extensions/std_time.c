#include <time.h>

#include "../common/str.h"
#include "../common/fun.h"
#include "std_time.h"

int std_time(void *p) {
    lgx_val_t ret;
    ret.type = T_LONG;
    ret.v.l = time(NULL);

    return lgx_ext_return(p, &ret);
}

int std_time_load_symbols(lgx_hash_t *hash) {
    lgx_val_t symbol;

    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(0);
    symbol.v.fun->buildin = std_time;

    if (lgx_ext_add_symbol(hash, "time", &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_time_ctx = {
    "std.time",
    std_time_load_symbols
};