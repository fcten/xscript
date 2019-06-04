#include "value.h"

int lgx_value_init(lgx_value_t* v) {
    memset(v, 0, sizeof(lgx_value_t));
    return 0;
}

// TODO
void lgx_value_cleanup(lgx_value_t* v) {
    switch (v->type) {
        case T_FUNCTION:
            lgx_str_cleanup(&v->v.fun->name);
            lgx_type_cleanup(&v->v.fun->gc.type);
            xfree(v->v.fun);
            break;
        case T_ARRAY:
            lgx_array_cleanup(&v->v.arr->table);
            lgx_ht_cleanup(&v->v.arr->table);
            lgx_type_cleanup(&v->v.arr->gc.type);
            xfree(v->v.arr);
            break;
        default:
            break;
    }
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

void lgx_array_cleanup(lgx_ht_t* arr) {
    lgx_ht_node_t* n;
    for (n = lgx_ht_first(arr); n; n = lgx_ht_next(n)) {
        lgx_value_cleanup((lgx_value_t*)n->v);
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
        default:
            printf("unknwon");
            break;
    }
}

char* lgx_value_typeof(lgx_value_t* v) {
    switch (v->type) {
        case T_UNKNOWN:
            return "unknown";
        case T_LONG:
            return "integer";
        case T_DOUBLE:
            return "float";
        case T_BOOL:
            return "boolean";
        default:
            return lgx_type_to_string(&v->v.gc->type);
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
        case T_ARRAY:

            break;
        default:
            break;
    }

    return 0;
}