#include <stdlib.h>
#include <time.h>

#include "../common/str.h"
#include "../common/fun.h"
#include "std_math.h"

int std_rand(void *p) {
    return lgx_ext_return_long((lgx_vm_t *)p, rand());
}

int std_math_load_symbols(lgx_hash_t *hash) {
    // 初始化随机数种子
    srand((unsigned)time(NULL));

    lgx_val_t symbol;

    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(0);
    symbol.v.fun->buildin = std_rand;

    if (lgx_ext_add_symbol(hash, "rand", &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_math_ctx = {
    "std.math",
    std_math_load_symbols
};