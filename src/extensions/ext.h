#ifndef LGX_EXT_H
#define LGX_EXT_H

#include "../interpreter/vm.h"
#include "../interpreter/gc.h"

typedef struct lgx_buildin_ext_s {
    const char *package;
    int (*load_symbols)(lgx_hash_t *hash);
} lgx_buildin_ext_t;

typedef struct lgx_ext_s {

} lgx_ext_t;

int lgx_ext_load_symbols(lgx_hash_t *hash);

int lgx_ext_add_symbol(lgx_hash_t *hash, char *symbol, lgx_val_t *v);

int lgx_ext_return(lgx_vm_t *vm, lgx_val_t *v);
int lgx_ext_return_long(lgx_vm_t *vm, long long v);
int lgx_ext_return_double(lgx_vm_t *vm, double v);

#endif // LGX_EXT_H