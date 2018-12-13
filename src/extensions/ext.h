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

#define LGX_RETURN_UNDEFINED() \
    lgx_co_return_undefined(vm->co_running)

#define LGX_RETURN_STRING(v) \
    lgx_co_return_string(vm->co_running, v)

#define LGX_RETURN_ARRAY(v) \
    lgx_co_return_array(vm->co_running, v)

#define LGX_RETURN_TRUE() \
    lgx_co_return_true(vm->co_running)

#define LGX_RETURN_FALSE() \
    lgx_co_return_false(vm->co_running)

#define LGX_FUNCTION(function) \
    static int lgx_internal_function_##function(lgx_vm_t *vm)

#define LGX_FUNCTION_BEGIN(function, args) \
    do { \
        lgx_val_t symbol; \
        symbol.type = T_FUNCTION; \
        symbol.v.fun = lgx_fun_new(args); \
        symbol.v.fun->name.buffer = #function; \
        symbol.v.fun->name.length = sizeof(#function) - 1; \
        symbol.v.fun->buildin = lgx_internal_function_##function;

#define LGX_FUNCTION_RET(rettype) \
        symbol.v.fun->ret.type = rettype;

#define LGX_FUNCTION_ARG(pos, argtype) \
        symbol.v.fun->args[pos].type = argtype;

#define LGX_FUNCTION_ARG_OPTIONAL(pos, argtype, argvalue) \
        symbol.v.fun->args[pos].type = argtype; \
        symbol.v.fun->args[pos].v.l = (long long)(argvalue); \
        symbol.v.fun->args[pos].u.c.init = 1;

#define LGX_FUNCTION_END \
        if (lgx_ext_add_symbol(hash, symbol.v.fun->name.buffer, &symbol)) { \
            return 1; \
        } \
    } while (0);

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

#define LGX_METHOD_BEGIN(class, method, args) \
    do { \
        lgx_hash_node_t symbol; \
        symbol.k.type = T_STRING; \
        symbol.k.v.str = lgx_str_new_ref(#method, sizeof(#method) - 1); \
        symbol.v.type = T_FUNCTION; \
        symbol.v.v.fun = lgx_fun_new(args); \
        symbol.v.v.fun->buildin = lgx_internal_method_##class##method; \

#define LGX_METHOD_RET(rettype) \
        symbol.v.v.fun->ret.type = rettype;

#define LGX_METHOD_ARG(pos, argtype) \
        symbol.v.v.fun->args[pos].type = argtype;

#define LGX_METHOD_ARG_OPTIONAL(pos, argtype, argvalue) \
        symbol.v.v.fun->args[pos].type = argtype; \
        symbol.v.v.fun->args[pos].v.l = (long long)(argvalue); \
        symbol.v.v.fun->args[pos].u.c.init = 1;

#define LGX_METHOD_ACCESS(accessmodifier) \
        symbol.v.u.c.modifier.access = accessmodifier;

#define LGX_METHOD_STATIC() \
        symbol.v.u.c.modifier.is_static = 1;

#define LGX_METHOD_END \
        if (lgx_obj_add_method(obj, &symbol)) { \
            return 1; \
        } \
    } while (0);

#define LGX_PROPERTY_BEGIN(class, property, propertytype, propertyvalue) \
    do { \
        lgx_hash_node_t symbol; \
        symbol.k.type = T_STRING; \
        symbol.k.v.str = lgx_str_new_ref(#property, sizeof(#property) - 1); \
        symbol.v.type = propertytype; \
        symbol.v.v.l = (long long)(propertyvalue);

#define LGX_PROPERTY_ACCESS(accessmodifier) \
        symbol.v.u.c.modifier.access = accessmodifier;

#define LGX_PROPERTY_STATIC() \
        symbol.v.u.c.modifier.is_static = 1;

#define LGX_PROPERTY_END \
        if (lgx_obj_add_property(obj, &symbol)) { \
            return 1; \
        } \
    } while (0);

#define LGX_FUNCTION_ARGS_INIT() \
    lgx_fun_t *_parent_func = vm->regs[0].v.fun; \
    lgx_val_t *_this_stack = &vm->regs[_parent_func->stack_size]; \
    lgx_fun_t *_this_func  = _this_stack->v.fun

#define LGX_FUNCTION_ARGS_GET(variable, position) \
    lgx_val_t *variable = _this_stack + position + 4; \
    if (variable->type == T_UNDEFINED && _this_func->args[position].u.c.init) { \
        *variable = _this_func->args[position]; \
    }

#define LGX_METHOD_ARGS_INIT() \
    lgx_fun_t *_parent_func = vm->regs[0].v.fun; \
    lgx_val_t *_this_stack = &vm->regs[_parent_func->stack_size]; \
    lgx_fun_t *_this_func = _this_stack->v.fun

#define LGX_METHOD_ARGS_THIS(variable) \
    lgx_val_t *variable = _this_stack + 4

#define LGX_METHOD_ARGS_GET(variable, position) \
    lgx_val_t *variable = _this_stack + position + 5; \
    if (variable->type == T_UNDEFINED && _this_func->args[position].u.c.init) { \
        *variable = _this_func->args[position]; \
    }

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