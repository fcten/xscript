#include <stdio.h>
#include <stdlib.h>

#include "hash.h"

int lgx_hash_init(lgx_hash_t *hash, unsigned size) {
    hash->size = size;
    hash->key_offset = 0;
    hash->table_offset = 0;
    
    hash->key = calloc(hash->size, sizeof(lgx_hash_key_t));
    if (!hash->key) {
        hash->size = 0;
        return 1;
    }
    
    hash->table = malloc(hash->size * sizeof(lgx_hash_node_t));
    if (!hash->table) {
        hash->size = 0;
        free(hash->key);
        return 2;
    }
    
    int i;
    for (i = 0; i < hash->size; i++) {
        lgx_list_init(&hash->key[i].vl.head);
    }
    
    return 0;
}

int lgx_hash_cleanup(lgx_hash_t *hash) {
    int i;
    for (i = 0; i < hash->size; i ++) {
        while (!lgx_list_empty(&hash->key[i].vl.head)) {
            lgx_val_list_t *list = lgx_list_first_entry(&hash->key[i].vl.head, lgx_val_list_t, head);
            lgx_list_del(&list->head);
            free(list);
        }
    }

    hash->size = 0;

    free(hash->key);
    hash->key = NULL;
    hash->key_offset = 0;

    free(hash->table);
    hash->table = NULL;
    hash->table_offset = 0;

    return 0;
}

// 将 hash table 扩容一倍
static int hash_resize(lgx_hash_t *hash) {
    if (hash->table_offset < hash->size) {
        return 0;
    }
    
    // resize
    lgx_hash_t new;
    lgx_hash_init(&new, hash->size*2);

    for (int i = 0; i < hash->table_offset; i++) {
        lgx_hash_set(&new, &hash->table[i]);
    }

    hash->size = new.size;

    free(hash->key);
    hash->key = new.key;

    free(hash->table);
    hash->table = new.table;

    return 0;
}

static unsigned hash_bkdr(lgx_hash_t *hash, lgx_val_t *k) {
    unsigned ret = 0;
    int i, length;
    
    switch (k->type) {
        case T_LONG:
        case T_DOUBLE:
            ret = (unsigned long long)k->v.l % hash->size;
            break;
        case T_BOOL:
            if (k->v.l) {
                return 1;
            } else {
                return 0;
            }
            break;
        case T_STRING:
            length = k->v.str->length;
            for (i = 0 ; i < length ; i ++) {
                ret = (ret << 5) - ret + k->v.str->buffer[i];
            }
            ret %= hash->size;
            break;
        case T_FUNCTION:
            ret = k->v.fun->addr % hash->size;
            break;
        default:
            // error
            ret = 0;
    }
    
    return ret;
}

// TODO value 复制需要复制内部数据结构，否则会导致循环引用
int lgx_hash_set(lgx_hash_t *hash, lgx_hash_node_t *node) {
    unsigned k = hash_bkdr(hash, &node->k);
    lgx_val_list_t *p;
    lgx_hash_node_t *n;
    lgx_list_for_each_entry(p, lgx_val_list_t, &hash->key[k].vl.head, head) {
        if (p->v->type == node->k.type) {
            switch (p->v->type) {
                case T_LONG:
                case T_DOUBLE:
                    if (p->v->v.l == node->k.v.l) {
                        // key 已存在
                        n = (lgx_hash_node_t *)p->v;
                        n->v = node->v;
                        return 0;
                    }
                    break;
                case T_BOOL:
                    if ( (p->v->v.l && node->k.v.l) || (!p->v->v.l && !node->k.v.l) ) {
                        // key 已存在
                        n = (lgx_hash_node_t *)p->v;
                        n->v = node->v;
                        return 0;
                    }
                    break;
                case T_STRING:
                    if (lgx_str_cmp(p->v->v.str, node->k.v.str) == 0) {
                        // key 已存在
                        n = (lgx_hash_node_t *)p->v;
                        n->v = node->v;
                        return 0;
                    }
                    break;
                case T_FUNCTION:
                    if (p->v->v.fun == node->k.v.fun) {
                        // key 已存在
                        n = (lgx_hash_node_t *)p->v;
                        n->v = node->v;
                        return 0;
                    }
                default:
                    // key 类型不合法
                    return 1;
            }
        }
    }

    // key 不存在
    hash->table[hash->table_offset++] = *node;
    
    lgx_val_list_t* vl = malloc(sizeof(lgx_val_list_t));
    if (!vl) {
        hash->table_offset--;
        return 2;
    }
    vl->v = &hash->table[hash->table_offset-1].k;
    lgx_list_init(&vl->head);

    lgx_list_add_tail(&vl->head, &hash->key[k].vl.head);
    hash->key[k].count ++;
    
    return hash_resize(hash);
}

int lgx_hash_add(lgx_hash_t *hash, lgx_val_t *v) {
    lgx_hash_node_t node;

    node.v = *v;

    node.k.type = T_LONG;
    node.k.v.l = hash->table_offset;

    return lgx_hash_set(hash, &node);
}

int lgx_hash_get(lgx_hash_t *hash, lgx_val_t *key) {
    unsigned k = hash_bkdr(hash, key);
    lgx_val_list_t *p;
    lgx_list_for_each_entry(p, lgx_val_list_t, &hash->key[k].vl.head, head) {
        if (p->v->type == key->type) {
            switch (p->v->type) {
                case T_LONG:
                case T_DOUBLE:
                    if (p->v->v.l == key->v.l) {
                        // key 存在
                        return (lgx_hash_node_t *)p->v - hash->table;
                    }
                    break;
                case T_BOOL:
                    if ( (p->v->v.l && key->v.l) || (!p->v->v.l && !key->v.l) ) {
                        // key 已存在
                        return (lgx_hash_node_t *)p->v - hash->table;
                    }
                    break;
                case T_STRING:
                    if (lgx_str_cmp(p->v->v.str, key->v.str) == 0) {
                        // key 存在
                        return (lgx_hash_node_t *)p->v - hash->table;
                    }
                    break;
                case T_FUNCTION:
                    if (p->v->v.fun == key->v.fun) {
                        // key 存在
                        return (lgx_hash_node_t *)p->v - hash->table;
                    }
                    break;
                default:
                    // key 类型不合法
                    return -1;
            }
        }
    }

    return -1;
}

int lgx_hash_print(lgx_hash_t *hash) {
    printf("[");
    int i;
    for (i = 0 ; i < hash->table_offset ; i ++) {
        //lgx_val_print(&hash->table[i].k);
        //printf("=");
        lgx_val_print(&hash->table[i].v);
        if (i < hash->table_offset - 1) {
            printf(",");
        }
        //printf("\n");
    }
    printf("]");

    return 0;
}