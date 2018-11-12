#ifndef LGX_EXT_H
#define LGX_EXT_H

#include "../common/common.h"
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

int lgx_ext_return(lgx_co_t *co, lgx_val_t *v);
int lgx_ext_return_long(lgx_co_t *co, long long v);
int lgx_ext_return_double(lgx_co_t *co, double v);
int lgx_ext_return_true(lgx_co_t *co);
int lgx_ext_return_false(lgx_co_t *co);
int lgx_ext_return_undefined(lgx_co_t *co);
int lgx_ext_return_string(lgx_co_t *co, lgx_str_t *str);

#endif // LGX_EXT_H