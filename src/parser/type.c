#include "type.h"

const lgx_type_t lgx_type_basic[] = {
    {T_UNKNOWN, lgx_str("unknown")},
    {T_NULL,    lgx_str("null")},
    {T_CUSTOM,  lgx_str("custom")},
    {T_LONG,    lgx_str("integer")},
    {T_DOUBLE,  lgx_str("float")},
    {T_BOOL,    lgx_str("bool")},
    {T_STRING,  lgx_str("string")}
};

char* lgx_type_to_string(lgx_type_t* type) {
    switch (type->type) {
        case T_UNKNOWN:
            return "unknown";
        case T_LONG:
            return "integer";
        case T_DOUBLE:
            return "float";
        case T_BOOL:
            return "boolean";
        case T_STRING:
            return "string";
        case T_ARRAY:
            return "array";
        case T_MAP:
            return "map";
        case T_STRUCT:
            return "struct";
        case T_INTERFACE:
            return "interface";
        case T_FUNCTION:
            return "function";
        default:
            return "invalid";
    }
}

int lgx_type_cmp(lgx_type_t* t1, lgx_type_t* t2) {
    if (t1->type != t2->type) {
        return 1;
    }

    switch (t1->type) {
        case T_ARRAY:
            // TODO
            break;
        case T_MAP:
            // TODO
            break;
        case T_STRUCT:
            // TODO
            break;
        case T_INTERFACE:
            // TODO
            break;
        case T_FUNCTION:
            // TODO
            break;
        default:
            break;
    }

    return 0;
}

int lgx_type_function_dup(lgx_type_function_t* src, lgx_type_function_t* dst) {
    // 函数接收者类型
    if (lgx_type_dup(&src->receiver, &dst->receiver)) {
        return 1;
    }

    // 返回值类型
    if (lgx_type_dup(&src->ret, &dst->ret)) {
        return 1;
    }

    // 参数列表
    dst->arg_len = src->arg_len;
    dst->args = xcalloc(dst->arg_len, sizeof(lgx_type_t));
    int i;
    for (i = 0; i < dst->arg_len; ++i) {
        if (lgx_type_dup(&src->args[i], &dst->args[i])) {
            return 1;
        }
    }

    return 0;
}

int lgx_type_dup(lgx_type_t* src, lgx_type_t* dst) {
    assert(dst->type == T_UNKNOWN);
    assert(dst->u.arr == NULL);

    if (lgx_type_init(dst, src->type)) {
        return 1;
    }

    switch (dst->type) {
        case T_FUNCTION:
            if (lgx_type_function_dup(src->u.fun, dst->u.fun)) {
                return 1;
            }
            break;            
        default:
            break;
    }

    return 0;
}

static int check_unknown(lgx_type_t* type) {
    switch (type->type) {
        case T_UNKNOWN:
            return 1;
        case T_ARRAY:
            return check_unknown(&type->u.arr->value);
        case T_MAP:
            if (check_unknown(&type->u.map->key)) {
                return 1;
            }
            return check_unknown(&type->u.map->value);
        default:
            return 0;
    }
}

int lgx_type_inference(lgx_type_t* src, lgx_type_t* dst) {
    if (check_unknown(src)) {
        return 1;
    }
    return lgx_type_dup(src, dst);
}

int lgx_type_init(lgx_type_t* type, lgx_val_type_t t) {
    memset(type, 0, sizeof(lgx_type_t));

    type->type = t;

    switch (type->type) {
        case T_CUSTOM:
        case T_LONG:
        case T_DOUBLE:
        case T_BOOL:
        case T_STRING:
        case T_UNKNOWN:
            // 简单类型
            break;
        case T_ARRAY:
            type->u.arr = xcalloc(1, sizeof(lgx_type_array_t));
            if (!type->u.arr) {
                return 1;
            }
            break;
        case T_MAP:
            type->u.map = xcalloc(1, sizeof(lgx_type_map_t));
            if (!type->u.map) {
                return 1;
            }
            break;
        case T_STRUCT:
            type->u.sru = xcalloc(1, sizeof(lgx_type_struct_t));
            if (!type->u.sru) {
                return 1;
            }
            break;
        case T_INTERFACE:
            type->u.itf = xcalloc(1, sizeof(lgx_type_interface_t));
            if (!type->u.itf) {
                return 1;
            }
            break;
        case T_FUNCTION:
            type->u.fun = xcalloc(1, sizeof(lgx_type_function_t));
            if (!type->u.fun) {
                return 1;
            }
            break;
        default:
            return 1;
    }

    return 0;
}

void lgx_type_array_cleanup(lgx_type_array_t* arr) {
    memset(arr, 0, sizeof(lgx_type_array_t));
}

void lgx_type_map_cleanup(lgx_type_map_t* map) {
    memset(map, 0, sizeof(lgx_type_map_t));
}

void lgx_type_struct_cleanup(lgx_type_struct_t* sru) {
    memset(sru, 0, sizeof(lgx_type_struct_t));
}

void lgx_type_interface_cleanup(lgx_type_interface_t* itf) {
    memset(itf, 0, sizeof(lgx_type_interface_t));
}

void lgx_type_function_cleanup(lgx_type_function_t* fun) {
    lgx_type_cleanup(&fun->receiver);
    lgx_type_cleanup(&fun->ret);

    int i;
    for (i = 0; i < fun->arg_len; ++i) {
        lgx_type_cleanup(&fun->args[i]);
    }

    xfree(fun->args);

    memset(fun, 0, sizeof(lgx_type_function_t));
}

void lgx_type_cleanup(lgx_type_t* type) {
    switch (type->type) {
        case T_ARRAY:
            if (type->u.arr) {
                lgx_type_array_cleanup(type->u.arr);
                xfree(type->u.arr);
            }
            break;
        case T_MAP:
            if (type->u.map) {
                lgx_type_map_cleanup(type->u.map);
                xfree(type->u.map);
            }
            break;
        case T_STRUCT:
            if (type->u.sru) {
                lgx_type_struct_cleanup(type->u.sru);
                xfree(type->u.sru);
            }
            break;
        case T_INTERFACE:
            if (type->u.itf) {
                lgx_type_interface_cleanup(type->u.itf);
                xfree(type->u.itf);
            }
            break;
        case T_FUNCTION:
            if (type->u.fun) {
                lgx_type_function_cleanup(type->u.fun);
                xfree(type->u.fun);
            }
            break;
        case T_UNKNOWN:
        case T_LONG:
        case T_DOUBLE:
        case T_BOOL:
        case T_STRING:
        default:
            break;
    }

    memset(type, 0, sizeof(lgx_type_t));
}