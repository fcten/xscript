#ifndef LGX_EXTENSION_H
#define LGX_EXTENSION_H

#include "../interpreter/vm.h"

typedef struct lgx_buildin_ext_s {
    const char *package;
} lgx_buildin_ext_t;

typedef struct lgx_ext_s {

} lgx_ext_t;

int lgx_ext_load(lgx_vm_t *vm);

int lgx_ext_register_function(const char *package, const char *name, lgx_fun_t *fun);

#endif // LGX_EXTENSION_H