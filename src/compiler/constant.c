#include "../interpreter/value.h"
#include "constant.h"

int const_key(lgx_str_t* key, lgx_expr_result_t* e) {
    switch (e->v_type.type) {
        case T_LONG: {
            lgx_str_t prefix = lgx_str("int:");
            lgx_str_t name;
            name.buffer = (char *)&e->v.l;
            name.length = sizeof(e->v.l);
            name.size = 0;
            if (lgx_str_init(key, prefix.length + name.length)) {
                return 1;
            }
            lgx_str_concat(&prefix, key);
            lgx_str_concat(&name, key);
            return 0;
        }
        case T_DOUBLE: {
            lgx_str_t prefix = lgx_str("float:");
            lgx_str_t name;
            name.buffer = (char *)&e->v.d;
            name.length = sizeof(e->v.d);
            name.size = 0;
            if (lgx_str_init(key, prefix.length + name.length)) {
                return 1;
            }
            lgx_str_concat(&prefix, key);
            lgx_str_concat(&name, key);
            return 0;
        }
        case T_BOOL: {
            if (e->v.l) {
                lgx_str_t prefix = lgx_str("bool:true");
                if (lgx_str_init(key, prefix.length)) {
                    return 1;
                }
                lgx_str_dup(&prefix, key);
                return 0;
            } else {
                lgx_str_t prefix = lgx_str("bool:false");
                if (lgx_str_init(key, prefix.length)) {
                    return 1;
                }
                lgx_str_dup(&prefix, key);
                return 0;
            }
        }
        case T_STRING: {
            lgx_str_t prefix = lgx_str("string:");
            if (lgx_str_init(key, prefix.length + e->v.str.length)) {
                return 1;
            }
            lgx_str_concat(&prefix, key);
            lgx_str_concat(&e->v.str, key);
            return 0;
        }
        // TODO: case T_ARRAY: 
        /*
        case T_FUNCTION: {
            lgx_str_t prefix = lgx_str("function:");
            if (lgx_str_init(key, prefix.length + e->v.str.length)) {
                return 1;
            }
            lgx_str_concat(&prefix, key);
            lgx_str_concat(&e->v.str, key);
            return 0;
        }
        */
        default:
            return 1;
    }
}

int lgx_const_get(lgx_ht_t* ct, lgx_expr_result_t* e) {
    lgx_str_t key;
    lgx_str_init(&key, 0);
    if (const_key(&key, e)) {
        return -1;
    }

    lgx_ht_node_t* n = lgx_ht_get(ct, &key);
    if (n) {
        lgx_str_cleanup(&key);
        return ((lgx_const_t*)n->v)->num;
    }

    lgx_const_t* c = lgx_const_new(ct, e);
    if (!c) {
        return -1;
    }

    if (lgx_ht_set(ct, &key, c)) {
        lgx_str_cleanup(&key);
        return -1;
    }

    n = lgx_ht_get(ct, &key);
    assert(n);
    lgx_str_cleanup(&key);
    return ((lgx_const_t*)n->v)->num;
}

int lgx_const_get_function(lgx_ht_t* ct, lgx_str_t* fun_name) {
    lgx_str_t prefix = lgx_str("function:");
    lgx_str_t key;
    if (lgx_str_init(&key, prefix.length + fun_name->length)) {
        return -1;
    }
    lgx_str_concat(&prefix, &key);
    lgx_str_concat(fun_name, &key);

    lgx_ht_node_t* n = lgx_ht_get(ct, &key);
    if (n) {
        lgx_str_cleanup(&key);
        return ((lgx_const_t*)n->v)->num;
    }

    lgx_const_t* c = lgx_const_new(ct, NULL);
    if (!c) {
        return -1;
    }

    c->v.type = T_LONG;
    c->v.v.l = c->num;

    if (lgx_ht_set(ct, &key, c)) {
        lgx_str_cleanup(&key);
        return -1;
    }

    n = lgx_ht_get(ct, &key);
    assert(n);
    lgx_str_cleanup(&key);
    return ((lgx_const_t*)n->v)->num;
}

int lgx_const_update_function(lgx_ht_t* ct, lgx_function_t* fun) {
    lgx_str_t prefix = lgx_str("function:");
    lgx_str_t key;
    if (lgx_str_init(&key, prefix.length + fun->name.length)) {
        return 1;
    }
    lgx_str_concat(&prefix, &key);
    lgx_str_concat(&fun->name, &key);

    lgx_ht_node_t* n = lgx_ht_get(ct, &key);
    if (!n) {
        return 1;
    }
    
    lgx_const_t* c = (lgx_const_t*)n->v;
    c->v.type = T_FUNCTION;
    c->v.v.fun = fun;

    return 0;
}

lgx_const_t* lgx_const_new(lgx_ht_t* ct, lgx_expr_result_t* e) {
    lgx_const_t* c = xcalloc(1, sizeof(lgx_const_t));
    c->num = ct->length;

    if (e) {
        if (lgx_expr_to_value(e, &c->v)) {
            xfree(c);
            return NULL;
        }
    }

    return c;
}

void lgx_const_del(lgx_const_t* c) {
    xfree(c);
}

void lgx_const_print(lgx_ht_t* ct) {
    lgx_ht_node_t* n;
    printf("[");
    for (n = lgx_ht_first(ct); n; n = lgx_ht_next(n)) {
        lgx_value_print(&((lgx_const_t*)n->v)->v);
        printf(",");
    }
    printf("\b]");
}