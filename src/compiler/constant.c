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

lgx_const_t* lgx_const_new(lgx_ht_t* ct, lgx_expr_result_t* e) {
    lgx_const_t* c = xcalloc(1, sizeof(lgx_const_t));
    c->num = ct->length;
    c->v.type = e->v_type.type;

    switch (e->v_type.type) {
    case T_LONG:
        c->v.v.l = e->v.l;
        break;
    case T_DOUBLE:
        c->v.v.d = e->v.d;
        break;
    case T_STRING:
        c->v.v.str = lgx_string_new();
        assert(c->v.v.str);
        lgx_str_init(&c->v.v.str->string, e->v.str.length);
        lgx_str_dup(&e->v.str, &c->v.v.str->string);
        break;
    // case T_ARRAY:
    // TODO 其他类型
    default:
        break;
    }

    return c;
}

void lgx_const_del(lgx_const_t* c) {
    xfree(c);
}