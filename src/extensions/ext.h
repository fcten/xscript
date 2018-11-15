#ifndef LGX_EXT_H
#define LGX_EXT_H

#include "../common/common.h"
#include "../interpreter/vm.h"
#include "../interpreter/gc.h"
#include "../interpreter/coroutine.h"

typedef struct lgx_buildin_ext_s {
    const char *package;
    int (*load_symbols)(lgx_hash_t *hash);
} lgx_buildin_ext_t;

typedef struct lgx_ext_s {

} lgx_ext_t;

int lgx_ext_load_symbols(lgx_hash_t *hash);

lgx_val_t* lgx_ext_get_symbol(lgx_hash_t *hash, char *symbol);
int lgx_ext_add_symbol(lgx_hash_t *hash, char *symbol, lgx_val_t *v);

#endif // LGX_EXT_H