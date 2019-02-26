#include "../common/fun.h"
#include "../common/str.h"
#include "std_type.h"

LGX_FUNCTION(to_float) {
    LGX_FUNCTION_ARGS_INIT();
    LGX_FUNCTION_ARGS_GET(from, 0, T_UNDEFINED);

    double ret = 0;
    
    switch (from->type) {
    case T_UNDEFINED:
        ret = 0;
        break;
    case T_LONG:
        ret = from->v.l;
        break;
    case T_DOUBLE:
        ret = from->v.d;
        break;
    case T_BOOL:
        if (from->v.l) {
            ret = 1;
        } else {
            ret = 0;
        }
        break;
    case T_STRING:{
        char *buff = xmalloc(from->v.str->length + 1);
        memcpy(buff, from->v.str->buffer, from->v.str->length);
        buff[from->v.str->length] = '\0';

        sscanf(buff, "%lf", &ret);
        break;
    }
    default:
        lgx_vm_throw_s(vm, "invalid param type for to_int");
        return 1;
    }

    LGX_RETURN_DOUBLE(ret);
    
    return 0;
}

LGX_FUNCTION(to_int) {
    LGX_FUNCTION_ARGS_INIT();
    LGX_FUNCTION_ARGS_GET(from, 0, T_UNDEFINED);

    long long ret = 0;
    
    switch (from->type) {
    case T_UNDEFINED:
        ret = 0;
        break;
    case T_LONG:
        ret = from->v.l;
        break;
    case T_DOUBLE:
        ret = from->v.d;
        break;
    case T_BOOL:
        if (from->v.l) {
            ret = 1;
        } else {
            ret = 0;
        }
        break;
    case T_STRING:{
        char *buff = xmalloc(from->v.str->length + 1);
        memcpy(buff, from->v.str->buffer, from->v.str->length);
        buff[from->v.str->length] = '\0';

        sscanf(buff, "%lld", &ret);
        break;
    }
    default:
        lgx_vm_throw_s(vm, "invalid param type for to_int");
        return 1;
    }

    LGX_RETURN_LONG(ret);
    
    return 0;
}

LGX_FUNCTION(to_string) {
    LGX_FUNCTION_ARGS_INIT();
    LGX_FUNCTION_ARGS_GET(from, 0, T_UNDEFINED);

    lgx_str_t *ret;
    // 50 字节应当足够保存任意 64 位数字类型
    char buff[50];
    
    switch (from->type) {
    case T_UNDEFINED:
        ret = lgx_str_new("undefined", sizeof("undefined") - 1);
        break;
    case T_LONG:
        sprintf(buff, "%lld", from->v.l);
        ret = lgx_str_new(buff, strlen(buff));
        break;
    case T_DOUBLE:
        sprintf(buff, "%lg", from->v.d);
        ret = lgx_str_new(buff, strlen(buff));
        break;
    case T_BOOL:
        if (from->v.l) {
            ret = lgx_str_new("true", sizeof("true") - 1);
        } else {
            ret = lgx_str_new("false", sizeof("false") - 1);
        }
        break;
    case T_STRING:
        ret = lgx_str_new(from->v.str->buffer, from->v.str->length);
        break;
    default:
        lgx_vm_throw_s(vm, "invalid param type for to_string");
        return 1;
    }

    LGX_RETURN_STRING(ret);
    
    return 0;
}

int std_type_load_symbols(lgx_hash_t *hash) {
    LGX_FUNCTION_BEGIN(to_int, 1)
        LGX_FUNCTION_RET(T_LONG)
        LGX_FUNCTION_ARG(0, T_UNDEFINED)
    LGX_FUNCTION_END

    LGX_FUNCTION_BEGIN(to_float, 1)
        LGX_FUNCTION_RET(T_DOUBLE)
        LGX_FUNCTION_ARG(0, T_UNDEFINED)
    LGX_FUNCTION_END

    LGX_FUNCTION_BEGIN(to_string, 1)
        LGX_FUNCTION_RET(T_STRING)
        LGX_FUNCTION_ARG(0, T_UNDEFINED)
    LGX_FUNCTION_END

    return 0;
}

lgx_buildin_ext_t ext_std_type_ctx = {
    wbt_string("std.type"),
    std_type_load_symbols
};