#include "../common/common.h"
#include "scope.h"

static lgx_ast_node_t* find_scope(lgx_ast_node_t *node) {
    while (node && node->type != BLOCK_STATEMENT) {
        node = node->parent;
    }
    return node;
}

// 在当前作用域上添加一个变量
lgx_val_t* lgx_scope_val_add(lgx_ast_node_t *node, lgx_str_t *s) {
    lgx_ast_node_t *cur = find_scope(node);

    lgx_hash_node_t n;
    memset(&n, 0, sizeof(lgx_hash_node_t));
    n.k.type = T_STRING;
    n.k.v.str = lgx_str_new_ref(s->buffer, s->length);

    if (lgx_hash_get(cur->u.symbols, &n.k)) {
        // 已经存在同名变量
        lgx_str_delete(n.k.v.str);
        return NULL;
    } else {
        lgx_hash_set(cur->u.symbols, &n);
        return &(lgx_hash_get(cur->u.symbols, &n.k))->v;
    }
}

// 查找局部变量
lgx_val_t* lgx_scope_local_val_get(lgx_ast_node_t *node, lgx_str_t *s) {
    lgx_val_t v;
    v.type = T_STRING;
    v.v.str = s;

    lgx_ast_node_t *cur = find_scope(node);
    while (cur->parent) {
        lgx_hash_node_t *n = lgx_hash_get(cur->u.symbols, &v);
        if (n) {
            n->v.u.c.type = R_LOCAL;
            return &n->v;
        } else {
            if (cur->parent->type == FUNCTION_DECLARATION) {
                break;
            } else {
                cur = find_scope(cur->parent);
            }
        }
    }

    return NULL;
}

// 查找全局变量
lgx_val_t* lgx_scope_global_val_get(lgx_ast_node_t *node, lgx_str_t *s) {
    lgx_val_t v;
    v.type = T_STRING;
    v.v.str = s;

    int is_global = 0;
    lgx_ast_node_t *cur = find_scope(node);
    while (cur->parent) {
        cur = cur->parent;
        if (cur->type == FUNCTION_DECLARATION) {
            // 在函数作用域中访问全局变量，则将变量标识为 global
            is_global = 1;
        }
    }

    lgx_hash_node_t *n = lgx_hash_get(cur->u.symbols, &v);
    if (n) {
        n->v.u.c.type = is_global ? R_GLOBAL : R_LOCAL;
        return &n->v;
    } else {
        return NULL;
    }
}

lgx_val_t* lgx_scope_val_get(lgx_ast_node_t *node, lgx_str_t *s) {
    lgx_val_t *v = lgx_scope_local_val_get(node, s);
    if (v) {
        return v;
    }

    v = lgx_scope_global_val_get(node, s);
    if (v) {
        return v;
    }

    return NULL;
}
