#ifndef LGX_EXTENSION_H
#define LGX_EXTENSION_H

#include "../interpreter/vm.h"

typedef struct lgx_buildin_ext_s {

} lgx_buildin_ext_t;

typedef struct lgx_ext_s {

} lgx_ext_t;

int lgx_ext_load(lgx_vm_t *vm);

#endif // LGX_EXTENSION_H