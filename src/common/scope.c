#include <stdlib.h>

#include "scope.h"

void lgx_scope_init(lgx_scope_t *root) {
    lgx_list_init(&root->head);
}

// 创建新的变量作用域
void lgx_scope_new(lgx_scope_t *root) {
    lgx_scope_t* s = malloc(sizeof(lgx_scope_t));
    lgx_list_init(&s->head);
    lgx_hash_init(&s->symbols, 32);

    lgx_list_add_tail(&s->head, &root->head);
}

// 删除当前变量作用域
void lgx_scope_delete(lgx_scope_t *root) {
    lgx_scope_t* current = lgx_list_last_entry(&root->head, lgx_scope_t, head);
    lgx_hash_cleanup(&current->symbols);

    lgx_list_del(&current->head);
}

// 在当前作用域上设置变量
void lgx_scope_val_set(lgx_scope_t *root, lgx_str_ref_t *s) {
    lgx_scope_t* current = lgx_list_last_entry(&root->head, lgx_scope_t, head);

    lgx_hash_node_t node;
    node.k.type = T_STRING;
    node.k.v.str = lgx_str_new(s->buffer,s->length);

    lgx_hash_set(&current->symbols, &node);
}

lgx_val_t* lgx_scope_val_get(lgx_scope_t *root, lgx_str_ref_t *s) {
    lgx_val_t v;
    v.type = T_STRING;
    v.v.str = lgx_str_new(s->buffer,s->length);

    lgx_scope_t* current = lgx_list_last_entry(&root->head, lgx_scope_t, head);
    while (current != root) {
        int i = lgx_hash_get(&current->symbols, &v);
        if (i>=0) {
            return &current->symbols.table[i].v;
        }
        current = lgx_list_prev_entry(current, lgx_scope_t, head);
    }
    return NULL;
}
