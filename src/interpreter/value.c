#include "gc.h"
#include "value.h"

int lgx_value_init(lgx_value_t* v) {
    memset(v, 0, sizeof(lgx_value_t));
    return 0;
}

// TODO
void lgx_value_cleanup(lgx_value_t* v) {
    switch (v->type) {
        case T_FUNCTION:
            lgx_function_cleanup(v->v.fun);
            xfree(v->v.fun);
            break;
        case T_ARRAY:
            lgx_array_cleanup(v->v.arr);
            xfree(v->v.arr);
            break;
        case T_STRING:
            lgx_string_cleanup(v->v.str);
            xfree(v->v.str);
        default:
            break;
    }
}

void lgx_string_cleanup(lgx_string_t* str) {
    lgx_str_cleanup(&str->string);
    lgx_type_cleanup(&str->gc.type);
}

void lgx_array_cleanup(lgx_array_t* arr) {
    lgx_ht_node_t* n;
    for (n = lgx_ht_first(&arr->table); n; n = lgx_ht_next(n)) {
        lgx_value_cleanup((lgx_value_t*)n->v);
        xfree(n->v);
        n->v = NULL;
    }

    lgx_ht_cleanup(&arr->table);
    lgx_type_cleanup(&arr->gc.type);
}

void lgx_function_cleanup(lgx_function_t* fun) {
    lgx_str_cleanup(&fun->name);
    lgx_type_cleanup(&fun->gc.type);
}

lgx_string_t* lgx_string_new() {
    lgx_string_t* str = xcalloc(1, sizeof(lgx_string_t));
    return str;
}

lgx_array_t* lgx_array_new() {
    lgx_array_t* arr = xcalloc(1, sizeof(lgx_array_t));
    return arr;
}

lgx_function_t* lgx_fucntion_new() {
    lgx_function_t* fun = xcalloc(1, sizeof(lgx_function_t));
    return fun;
}

void lgx_array_value_cleanup(lgx_ht_t* arr) {
    lgx_ht_node_t* n;
    for (n = lgx_ht_first(arr); n; n = lgx_ht_next(n)) {
        lgx_value_cleanup((lgx_value_t*)n->v);
        xfree(n->v);
        n->v = NULL;
    }
}

void lgx_array_print(lgx_array_t* arr) {
    printf("[");
    lgx_ht_node_t* n = NULL;
    for (n = lgx_ht_first(&arr->table); n; n = lgx_ht_next(n)) {
        lgx_value_print((lgx_value_t*)n->v);
        printf(",");
    }
    if (arr->table.length) {
        printf("\b]");
    } else {
        printf("]");
    }
}

void lgx_value_type_print(lgx_value_t* v) {
    switch (v->type) {
        case T_UNKNOWN:
            printf("unknown");
            break;
        case T_LONG:
            printf("int");
            break;
        case T_DOUBLE:
            printf("float");
            break;
        case T_BOOL:
            printf("bool");
            break;
        case T_STRING:
            printf("string");
            break;
        case T_NULL:
            printf("null");
            break;
        case T_ARRAY:{
            lgx_str_t type;
            lgx_str_set_null(type);
            lgx_type_to_string(&v->v.arr->gc.type, &type);
            printf("%.*s", type.length, type.buffer);
            lgx_str_cleanup(&type);
            break;
        }
        case T_CUSTOM:
        case T_MAP:
        case T_STRUCT:
        case T_INTERFACE:
        case T_FUNCTION:
            // TODO
            break;
        
    }
}

void lgx_value_print(lgx_value_t* v) {
   switch (v->type) {
        case T_LONG:
            printf("%lld", v->v.l);
            break;
        case T_DOUBLE:
            printf("%lg", v->v.d);
            break;
        case T_BOOL:
            if (v->v.l) {
                printf("true");
            } else {
                printf("false");
            }
            break;
        case T_STRING:
            printf("\"");
            lgx_str_print(&v->v.str->string);
            printf("\"");
            break;
        case T_FUNCTION:
            lgx_str_print(&v->v.fun->name);
            printf("()");
            break;
        case T_ARRAY:
            lgx_array_print(v->v.arr);
            break;
        case T_NULL:
            printf("null");
            break;
        case T_UNKNOWN:
            printf("unknwon");
            break;
        case T_MAP:
        case T_STRUCT:
        case T_INTERFACE:
        case T_CUSTOM:
            // TODO
            break;
    }
}

int lgx_value_typeof(lgx_value_t* v, lgx_str_t* str) {
    lgx_str_t append;

    switch (v->type) {
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
        default:
            return lgx_type_to_string(&v->v.gc->type, str);
    }
}

int lgx_value_cmp(lgx_value_t* v1, lgx_value_t* v2) {
    return 0;
}

int lgx_string_dup(lgx_string_t* src, lgx_string_t* dst) {
    return 0;
}

int lgx_array_dup(lgx_array_t* src, lgx_array_t* dst) {
    return 0;
}

int lgx_value_dup(lgx_value_t* src, lgx_value_t* dst) {
    dst->type = src->type;
    dst->v = src->v;

    // TODO
    switch (src->type) {
        case T_STRING:
            dst->v.str = lgx_string_new();
            if (!dst->v.str) {
                return 1;
            }
            if (lgx_type_dup(&src->v.str->gc.type, &dst->v.str->gc.type)) {
                xfree(dst->v.str);
                dst->v.str = NULL;
                return 1;
            }
            if (lgx_str_init(&dst->v.str->string, src->v.str->string.length)) {
                lgx_type_cleanup(&dst->v.str->gc.type);
                xfree(dst->v.str);
                dst->v.str = NULL;
                return 1;
            }
            lgx_str_dup(&src->v.str->string, &dst->v.str->string);
            break;
        case T_ARRAY:
            dst->v.arr = lgx_array_new();
            if (!dst->v.arr) {
                return 1;
            }
            if (lgx_type_dup(&src->v.arr->gc.type, &dst->v.arr->gc.type)) {
                xfree(dst->v.arr);
                dst->v.arr = NULL;
                return 1;
            }
            if (lgx_ht_init(&dst->v.arr->table, src->v.arr->table.length)) {
                lgx_type_cleanup(&dst->v.arr->gc.type);
                xfree(dst->v.arr);
                dst->v.arr = NULL;
                return 1;
            }
            lgx_ht_node_t* n;
            for (n = lgx_ht_first(&src->v.arr->table); n; n = lgx_ht_next(n)) {
                lgx_value_t* val = xcalloc(1, sizeof(lgx_value_t));
                if (lgx_value_dup((lgx_value_t*)n->v, val)) {
                    xfree(val);
                    lgx_value_cleanup(dst);
                    return 1;
                }
                if (lgx_ht_set(&dst->v.arr->table, &n->k, val)) {
                    lgx_value_cleanup(val);
                    xfree(val);
                    lgx_value_cleanup(dst);
                    return 1;
                }
            }
            break;
        case T_FUNCTION:
            dst->v.fun = xcalloc(1, sizeof(lgx_function_t));
            if (!dst->v.fun) {
                return 1;
            }
            if (lgx_type_dup(&src->v.fun->gc.type, &dst->v.fun->gc.type)) {
                xfree(dst->v.fun);
                dst->v.fun = NULL;
                return 1;
            }
            if (lgx_str_init(&dst->v.fun->name, src->v.fun->name.length)) {
                lgx_type_cleanup(&dst->v.fun->gc.type);
                xfree(dst->v.fun);
                dst->v.fun = NULL;
                return 1;
            }
            lgx_str_dup(&src->v.fun->name, &dst->v.fun->name);
            dst->v.fun->addr = src->v.fun->addr;
            dst->v.fun->end = src->v.fun->end;
            dst->v.fun->buildin = src->v.fun->buildin;
            dst->v.fun->stack_size = src->v.fun->stack_size;
            break;
        default:
            break;
    }

    return 0;
}