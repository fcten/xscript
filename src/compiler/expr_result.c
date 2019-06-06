#include "expr_result.h"

int lgx_expr_to_value(lgx_expr_result_t* e, lgx_value_t* v) {
    v->type = e->v_type.type;

    switch (v->type) {
    case T_LONG:
        v->v.l = e->v.l;
        break;
    case T_DOUBLE:
        v->v.d = e->v.d;
        break;
    case T_BOOL:
        v->v.l = e->v.l;
        break;
    case T_STRING:
        v->v.str = lgx_string_new();
        if (!v->v.str) {
            return 1;
        }
        if (lgx_type_dup(&e->v_type, &v->v.str->gc.type)) {
            xfree(v->v.str);
            v->v.str = NULL;
            return 1;
        }
        if (lgx_str_init(&v->v.str->string, e->v.str.length)) {
            lgx_type_cleanup(&v->v.str->gc.type);
            xfree(v->v.str);
            v->v.str = NULL;
            return 1;
        }
        lgx_str_dup(&e->v.str, &v->v.str->string);
        break;
    case T_ARRAY: {
        v->v.arr = lgx_array_new();
        if (!v->v.arr) {
            return 1;
        }
        if (lgx_type_dup(&e->v_type, &v->v.arr->gc.type)) {
            xfree(v->v.arr);
            v->v.arr = NULL;
            return 1;
        }
        if (lgx_ht_init(&v->v.arr->table, e->v.arr.length)) {
            lgx_type_cleanup(&v->v.arr->gc.type);
            xfree(v->v.arr);
            v->v.arr = NULL;
            return 1;
        }
        lgx_ht_node_t* n;
        for (n = lgx_ht_first(&e->v.arr); n; n = lgx_ht_next(n)) {
            lgx_value_t* val = xcalloc(1, sizeof(lgx_value_t));
            if (!val) {
                return 1;
            }
            if (lgx_value_dup((lgx_value_t*)n->v, val)) {
                xfree(v);
                return 1;
            }
            if (lgx_ht_set(&v->v.arr->table, &n->k, val)) {
                lgx_value_cleanup(v);
                xfree(v);
                return 1;
            }
        }
        break;
    }
    // TODO 其他类型
    default:
        return 1;
    }

    return 0;
}

int lgx_value_to_expr(lgx_value_t* v, lgx_expr_result_t* e) {
    e->type = EXPR_LITERAL;
    
    switch (v->type) {
        case T_LONG:
            e->v_type.type = T_LONG;
            e->v.l = v->v.l;
            break;
        case T_DOUBLE:
            e->v_type.type = T_DOUBLE;
            e->v.d = v->v.d;
            break;
        case T_BOOL:
            e->v_type.type = T_BOOL;
            e->v.l = v->v.l;
            break;
        case T_STRING:
            e->v_type.type = T_STRING;
            if (lgx_str_init(&e->v.str, v->v.str->string.length)) {
                return 1;
            }
            lgx_str_dup(&v->v.str->string, &e->v.str);
            break;
        case T_ARRAY:
            if (lgx_type_dup(&v->v.arr->gc.type, &e->v_type)) {
                return 1;
            }
            if (lgx_ht_init(&e->v.arr, v->v.arr->table.length)) {
                lgx_type_cleanup(&e->v_type);
                return 1;
            }
            lgx_ht_node_t* n;
            for (n = lgx_ht_first(&v->v.arr->table); n; n = lgx_ht_next(n)) {
                lgx_value_t* val = xcalloc(1, sizeof(lgx_value_t));
                if (!val) {
                    return 1;
                }
                if (lgx_value_dup((lgx_value_t*)n->v, val)) {
                    xfree(v);
                    return 1;
                }
                if (lgx_ht_set(&e->v.arr, &n->k, val)) {
                    lgx_value_cleanup(v);
                    xfree(v);
                    return 1;
                }
            }
            break;
        default:
            return 1;
    }

    return 0;
}