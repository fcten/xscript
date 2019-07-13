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

int lgx_type_to_string(lgx_type_t* type, lgx_str_t* str) {
    lgx_str_t append;

    switch (type->type) {
        case T_UNKNOWN:
            lgx_str_set(append, "unknown");
            return lgx_str_append(&append, str);
        case T_NULL:
            lgx_str_set(append, "null");
            return lgx_str_append(&append, str);
        case T_LONG:
            lgx_str_set(append, "int");
            return lgx_str_append(&append, str);
        case T_DOUBLE:
            lgx_str_set(append, "float");
            return lgx_str_append(&append, str);
        case T_BOOL:
            lgx_str_set(append, "bool");
            return lgx_str_append(&append, str);
        case T_STRING:
            lgx_str_set(append, "string");
            return lgx_str_append(&append, str);
        case T_ARRAY:
            lgx_str_set(append, "[]");
            if (lgx_str_append(&append, str)) {
                return 1;
            }
            return lgx_type_to_string(&type->u.arr->value, str);
        case T_MAP:
            lgx_str_set(append, "[");
            if (lgx_str_append(&append, str)) {
                return 1;
            }
            if (lgx_type_to_string(&type->u.map->key, str)) {
                return 1;
            }
            lgx_str_set(append, "]");
            if (lgx_str_append(&append, str)) {
                return 1;
            }
            return lgx_type_to_string(&type->u.map->value, str);
        case T_STRUCT:
            lgx_str_set(append, "struct");
            return lgx_str_append(&append, str);
        case T_INTERFACE:
            lgx_str_set(append, "interface");
            return lgx_str_append(&append, str);
        case T_FUNCTION:{
            if (type->u.fun->receiver.type != T_UNKNOWN) {
                lgx_str_set(append, "(");
                if (lgx_str_append(&append, str)) {
                    return 1;
                }
                if (lgx_type_to_string(&type->u.fun->receiver, str)) {
                    return 1;
                }
                lgx_str_set(append, ") ");
                if (lgx_str_append(&append, str)) {
                    return 1;
                }
            }
            lgx_str_set(append, "func(");
            if (lgx_str_append(&append, str)) {
                return 1;
            }
            int i;
            for (i = 0; i < type->u.fun->arg_len; ++i) {
                if (lgx_type_to_string(&type->u.fun->args[i], str)) {
                    return 1;
                }
                lgx_str_set(append, ",");
                if (lgx_str_append(&append, str)) {
                    return 1;
                }
            }
            if (type->u.fun->arg_len) {
                str->length --;
            }
            lgx_str_set(append, ")");
            if (lgx_str_append(&append, str)) {
                return 1;
            }
            if (type->u.fun->ret.type != T_UNKNOWN) {
                if (lgx_type_to_string(&type->u.fun->ret, str)) {
                    return 1;
                }
            }
            return 0;
        }
        default:
            lgx_str_set(append, "invalid");
            return lgx_str_append(&append, str);
    }
}

// 相同返回 0，不同返回 1
int lgx_type_cmp_function(lgx_type_function_t* t1, lgx_type_function_t* t2) {
    if (t1->receiver.type == T_UNKNOWN) {
        if (t2->receiver.type != T_UNKNOWN) {
            return 1;
        }
    } else {
        if (lgx_type_cmp(&t1->receiver, &t2->receiver)) {
            return 1;
        }
    }

    if (t1->ret.type == T_UNKNOWN) {
        if (t2->ret.type != T_UNKNOWN) {
            return 1;
        }
    } else {
        if (lgx_type_cmp(&t1->ret, &t2->ret)) {
            return 1;
        }
    }

    if (t1->arg_len != t2->arg_len) {
        return 1;
    }

    int i;
    for (i = 0; i < t1->arg_len; ++i) {
        if (lgx_type_cmp(&t1->args[i], &t2->args[i])) {
            return 1;
        }
    }

    return 0;
}

// 相同返回 0，不同返回 1
int lgx_type_cmp(lgx_type_t* t1, lgx_type_t* t2) {
    if (t1->type != t2->type) {
        return 1;
    }

    switch (t1->type) {
        case T_UNKNOWN:
            // unknown != unknown
            return 1;
        case T_ARRAY:
            return lgx_type_cmp(&t1->u.arr->value, &t2->u.arr->value);
        case T_MAP:
            if (lgx_type_cmp(&t1->u.map->key, &t2->u.map->key)) {
                return 1;
            }
            return lgx_type_cmp(&t1->u.map->value, &t2->u.map->value);
        case T_STRUCT:
            // TODO
            break;
        case T_INTERFACE:
            // TODO
            break;
        case T_FUNCTION:
            return lgx_type_cmp_function(t1->u.fun, t2->u.fun);
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

int lgx_type_array_dup(lgx_type_array_t* src, lgx_type_array_t* dst) {
    return lgx_type_dup(&src->value, &dst->value);
}

int lgx_type_map_dup(lgx_type_map_t* src, lgx_type_map_t* dst) {
    if (lgx_type_dup(&src->key, &dst->key)) {
        return 1;
    }

    return lgx_type_dup(&src->value, &dst->value);
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
        case T_ARRAY:
            if (lgx_type_array_dup(src->u.arr, dst->u.arr)) {
                return 1;
            }
            break;
        case T_MAP:
            if (lgx_type_map_dup(src->u.map, dst->u.map)) {
                return 1;
            }
            break;
        case T_STRUCT:
        case T_INTERFACE:
            // TODO
            break;
        default:
            break;
    }

    return 0;
}

// 检查类型是否已经完全确定
int lgx_type_is_definite(lgx_type_t* type) {
    switch (type->type) {
        case T_UNKNOWN:
            return 0;
        case T_ARRAY:
            return lgx_type_is_definite(&type->u.arr->value);
        case T_MAP:
            if (!lgx_type_is_definite(&type->u.map->key)) {
                return 0;
            }
            return lgx_type_is_definite(&type->u.map->value);
        default:
            return 1;
    }
}

// 根据 src 推断 dst 的类型。
// 只有未确定的类型才需要进行类型推断，确定的类型则进行类型比较
int lgx_type_inference(lgx_type_t* src, lgx_type_t* dst) {
    switch (dst->type) {
        case T_UNKNOWN:
            return lgx_type_dup(src, dst);
        case T_ARRAY:
            if (src->type != T_ARRAY) {
                return 1;
            }
            return lgx_type_inference(&src->u.arr->value, &dst->u.arr->value);
        case T_MAP:
            if (src->type != T_MAP) {
                return 1;
            }
            if (lgx_type_inference(&src->u.map->key, &dst->u.map->key)) {
                return 1;
            }
            return lgx_type_inference(&src->u.map->value, &dst->u.map->value);
        default:
            return lgx_type_cmp(src, dst);
    }
}

// 判断类型 src 是否可以被用于赋值给类型 dst
// 能则返回 1，不能则返回 0
// 注意：src 只能是字面量。如果 src 是变量或常量，应该使用 lgx_type_cmp 判断。
int lgx_type_is_fit(lgx_type_t* src, lgx_type_t* dst) {
    switch (src->type) {
        case T_UNKNOWN:
            return 1;
        case T_ARRAY:
            if (dst->type != T_ARRAY) {
                return 0;
            }
            return lgx_type_is_fit(&src->u.arr->value, &dst->u.arr->value);
        case T_MAP:
            if (dst->type != T_MAP) {
                return 0;
            }
            if (!lgx_type_is_fit(&src->u.map->key, &dst->u.map->key)) {
                return 0;
            }
            return lgx_type_is_fit(&src->u.map->value, &dst->u.map->value);
        case T_STRUCT:
        case T_INTERFACE:
        case T_FUNCTION:
            // TODO
            return 0;
        default:
            return !lgx_type_cmp(src, dst);
    }
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
    lgx_type_cleanup(&arr->value);
    memset(arr, 0, sizeof(lgx_type_array_t));
}

void lgx_type_map_cleanup(lgx_type_map_t* map) {
    // TODO
    memset(map, 0, sizeof(lgx_type_map_t));
}

void lgx_type_struct_cleanup(lgx_type_struct_t* sru) {
    // TODO
    memset(sru, 0, sizeof(lgx_type_struct_t));
}

void lgx_type_interface_cleanup(lgx_type_interface_t* itf) {
    // TODO
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