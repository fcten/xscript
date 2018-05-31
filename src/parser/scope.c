#include <stdlib.h>

#include "scope.h"

static lgx_ast_node_t* find_scope(lgx_ast_node_t *node) {
    while (node->type != BLOCK_STATEMENT) {
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

    lgx_hash_set(cur->u.symbols, &n);
}

// 查找作用域链上的所有变量
lgx_val_t* lgx_scope_val_get(lgx_ast_node_t *node, lgx_str_ref_t *s) {
    lgx_val_t v;
    v.type = T_STRING;
    v.v.str = lgx_str_new(s->buffer,s->length);

    lgx_ast_node_t *cur = find_scope(node);
    while (1) {
        int i = lgx_hash_get(cur->u.symbols, &v);
        if (i >= 0) {
            return &cur->u.symbols->table[i].v;
        } else {
            if (cur->parent) {
                cur = find_scope(cur->parent);
            } else {
                return NULL;
            }
        }
    }
}

// 查找当前作用域上的变量
lgx_val_t* lgx_scope_local_val_get(lgx_ast_node_t *node, lgx_str_ref_t *s) {
    lgx_val_t v;
    v.type = T_STRING;
    v.v.str = lgx_str_new(s->buffer,s->length);

    lgx_ast_node_t *cur = find_scope(node);
    int i = lgx_hash_get(cur->u.symbols, &v);
    if (i >= 0) {
        return &cur->u.symbols->table[i].v;
    } else {
        return NULL;
    }
}
