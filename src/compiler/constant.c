#include <stdint.h>
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
            if (lgx_str_init(key, prefix.length + v->v.fun->name.length)) {
                return 1;
            }
            lgx_str_concat(&prefix, key);
            lgx_str_concat(&v->v.fun->name, key);
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
        return 1;
    }

    lgx_ht_node_t* n = lgx_ht_get(ct, &key);
    if (n) {
        lgx_str_cleanup(&key);
        return (intptr_t)n->v;
    }

    if (lgx_ht_set(ct, &key, (void *)(intptr_t)ct->length)) {
        lgx_str_cleanup(&key);
        return -1;
    }

    n = lgx_ht_get(ct, &key);
    assert(n);
    lgx_str_cleanup(&key);
    return (intptr_t)n->v;
}