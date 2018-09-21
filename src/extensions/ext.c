#include "../common/hash.h"
#include "../common/str.h"
#include "ext.h"

#include "std_time.h"

lgx_buildin_ext_t *buildin_exts[] = {
    &ext_std_time_ctx
};

int lgx_ext_init(lgx_vm_t *vm) {
//    lgx_extensions = lgx_hash_new(32);

    return 0;
}

int lgx_ext_add_symbol(lgx_hash_t *hash, char *symbol, lgx_val_t *v) {
    lgx_hash_node_t n;
    n.k.type = T_STRING;
    n.k.v.str = lgx_str_new_ref(symbol, strlen(symbol));
    n.v = *v;
    n.v.u.c.used = 0;

    if (lgx_hash_get(hash, &n.k)) {
        // 符号已存在
        return 1;
    }

    lgx_hash_set(hash, &n);

    return 0;
}

int lgx_ext_load_symbols(lgx_hash_t *hash) {
    // 从内建扩展中加载符号
    int i;
    for (i = 0; i < sizeof(buildin_exts)/sizeof(lgx_buildin_ext_t*); i++) {
        if (buildin_exts[i]->load_symbols) {
            if (buildin_exts[i]->load_symbols(hash)) {
                return 1;
            }
        }
    }

    return 0;
}

int lgx_ext_return(lgx_vm_t *vm, lgx_val_t *v) {
    // 参数起始地址
    int base = vm->regs[0].v.fun->stack_size;

    // 返回值地址
    int ret = vm->regs[base + 1].v.l;

    // 写入返回值
    lgx_gc_ref_del(&vm->regs[ret]);
    vm->regs[ret] = *v;

    return 0;
}