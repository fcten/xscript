#ifndef LGX_EXT_H
#define LGX_EXT_H

#include "../common/common.h"
#include "../interpreter/vm.h"
#include "../interpreter/gc.h"
#include "../interpreter/coroutine.h"

#define LGX_RETURN(v) \
    lgx_co_return(vm->co_running, v)

#define LGX_RETURN_LONG(v) \
    lgx_co_return_long(vm->co_running, v)

#define LGX_RETURN_TRUE() \
    lgx_co_return_true(vm->co_running)

#define LGX_FUNCTION(function) \
    static int lgx_internal_function_##function(lgx_vm_t *vm)

#define LGX_FUNCTION_INIT(function) \
    do { \
        lgx_val_t symbol; \
        symbol.type = T_FUNCTION; \
        symbol.v.fun = lgx_fun_new(0); \
        symbol.v.fun->buildin = lgx_internal_function_##function; \
        if (lgx_ext_add_symbol(hash, #function, &symbol)) { \
            return 1; \
        } \
    } while (0)

#define LGX_CLASS(class) \
    static int lgx_internal_class_##class(lgx_obj_t *obj)

#define LGX_CLASS_INIT(class) \
    do { \
        lgx_str_t name; \
        name.buffer = #class; \
        name.length = sizeof(#class) - 1; \
        lgx_val_t symbol; \
        symbol.type = T_OBJECT; \
        symbol.v.obj = lgx_obj_create(&name); \
        if (lgx_ext_add_symbol(hash, #class, &symbol)) { \
            return 1; \
        } \
        if (lgx_internal_class_##class(symbol.v.obj)) { \
            return 1; \
        } \
    } while (0)

#define LGX_METHOD(class, method) \
    static int lgx_internal_method_##class##method(lgx_vm_t *vm)

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