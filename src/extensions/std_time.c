#include <time.h>

#include "../common/str.h"
#include "../common/fun.h"
#include "std_time.h"

int std_time(void *p) {
    lgx_vm_t *vm = p;

    // 参数起始地址
    int base = vm->regs[0].v.fun->stack_size;

    // 返回值地址
    int ret = vm->regs[base + 1].v.l;

    // 写入返回值
    vm->regs[ret].type = T_LONG;
    vm->regs[ret].v.l = time(NULL);

    return 0;
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