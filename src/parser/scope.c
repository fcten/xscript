#include <stdlib.h>

#include "scope.h"

static lgx_ast_node_t* find_scope(lgx_ast_node_t *node) {
    while (node && node->type != BLOCK_STATEMENT) {
        node = node->parent;
    }
    return node;
}

// 在当前作用域上添加一个变量
void lgx_scope_val_add(lgx_ast_node_t *node, lgx_str_ref_t *s) {
    lgx_ast_node_t *cur = find_scope(node);

    lgx_hash_node_t n;
    n.k.type = T_STRING;
    n.k.v.str = lgx_str_new(s->buffer,s->length);
    n.v.type = T_UNDEFINED;

    cur->u.symbols = lgx_hash_set(cur->u.symbols, &n);
}

// 查找局部变量
lgx_val_t* lgx_scope_local_val_get(lgx_ast_node_t *node, lgx_str_ref_t *s) {
    lgx_val_t v;
    v.type = T_STRING;
    v.v.str = lgx_str_new(s->buffer,s->length);

    lgx_ast_node_t *cur = find_scope(node);
    while (cur->parent) {
        lgx_hash_node_t *n = lgx_hash_get(cur->u.symbols, &v);
        if (n) {
            return &n->v;
        } else {
            cur = find_scope(cur->parent);
        }
    }

    return NULL;
}

// 查找全局变量
lgx_val_t* lgx_scope_global_val_get(lgx_ast_node_t *node, lgx_str_ref_t *s) {
    lgx_val_t v;
    v.type = T_STRING;
    v.v.str = lgx_str_new(s->buffer,s->length);

    lgx_ast_node_t *cur = find_scope(node);
    while (cur->parent) {
        cur = find_scope(cur->parent);
    }

    lgx_hash_node_t *n = lgx_hash_get(cur->u.symbols, &v);
    if (n) {
        return &n->v;
    } else {
        return NULL;
    }
}
