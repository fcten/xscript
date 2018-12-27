#include "common.h"
#include "str.h"
#include "hash.h"
#include "../interpreter/gc.h"

lgx_hash_t* _lgx_hash_new(unsigned size) {
    // 规范化 size 取值
    size = ALIGN(size);

    unsigned head_size = sizeof(lgx_hash_t);
    unsigned data_size = size * sizeof(lgx_hash_node_t);

    lgx_hash_t *hash = (lgx_hash_t *)xcalloc(1, head_size);
    if (UNEXPECTED(!hash)) {
        return NULL;
    }

    hash->table = (lgx_hash_node_t *)xcalloc(1, data_size);
    if (UNEXPECTED(!hash->table)) {
        xfree(hash);
        return NULL;
    }

    hash->size = size;

    hash->gc.size = head_size + data_size;
    hash->gc.type = T_ARRAY;
    wbt_list_init(&hash->gc.head);

    return hash;
}

// 清空哈希表
int lgx_hash_remove_all(lgx_hash_t *hash) {
    int i, length;
    if (hash->flag_non_compact_elements) {
        length = hash->size;
    } else {
        length = hash->length;
    }
    for (i = 0; i < length; i ++) {
        lgx_gc_ref_del(&hash->table[i].k);
        lgx_gc_ref_del(&hash->table[i].v);
        hash->table[i].k.type = T_UNDEFINED;
        hash->table[i].v.type = T_UNDEFINED;
        // 释放其它哈希值相同的元素
        lgx_hash_node_t *cur, *next = hash->table[i].next;
        while (next) {
            cur = next;
            next = cur->next;
            lgx_gc_ref_del(&cur->k);
            lgx_gc_ref_del(&cur->v);
            xfree(cur);
        }
        hash->table[i].next = NULL;
        hash->table[i].order = NULL;
    }

    hash->length = 0;
    hash->head = hash->tail = NULL;

    return 0;
}

// 删除哈希表，并删除有所引用计数小于等于 1 的子元素
int lgx_hash_delete(lgx_hash_t *hash) {
    if (!wbt_list_empty(&hash->gc.head)) {
        wbt_list_del(&hash->gc.head);
    }

    lgx_hash_remove_all(hash);

    xfree(hash->table);

    xfree(hash);

    return 0;
}

// 把 src 的元素复制到 dst 中
int lgx_hash_copy(lgx_hash_t *src, lgx_hash_t *dst) {
    lgx_hash_node_t *next = src->head;
    while (next) {
        if (lgx_hash_set(dst, next) != 0) {
            return 1;
        }
        next = next->order;
    }

    return 0;
}

// 将 hash table 扩容一倍
static int hash_resize(lgx_hash_t *hash) {
    lgx_hash_t *resize = lgx_hash_new(hash->size * 2);
    if (UNEXPECTED(!resize)) {
        return 1;
    }

    lgx_hash_copy(hash, resize);
    lgx_hash_remove_all(hash);

    xfree(hash->table);
    hash->table = resize->table;
    hash->size = resize->size;
    hash->length = resize->length;
    hash->gc.size = resize->gc.size;
    hash->head = resize->head;
    hash->tail = resize->tail;
    xfree(resize);

    return 0;
}

static unsigned hash_bkdr(lgx_hash_t *hash, lgx_val_t *k) {
    unsigned ret = 0;
    int i, length;
    
    switch (k->type) {
        case T_LONG:
        case T_DOUBLE:
            ret = (unsigned long long)k->v.l & (hash->size - 1);
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
            ret &= hash->size - 1;
            break;
        case T_FUNCTION:
            ret = k->v.fun->addr & (hash->size - 1);
            break;
        default:
            // error
            ret = 0;
    }
    
    return ret;
}

void lgx_hash_update_list(lgx_hash_t *hash, lgx_hash_node_t *node) {
    if (hash->head) {
        hash->tail->order = node;
        hash->tail = node;
    } else {
        hash->head = hash->tail = node;
    }
}

// TODO 正确处理循环引用
int lgx_hash_set(lgx_hash_t *hash, lgx_hash_node_t *node) {
    unsigned k = hash_bkdr(hash, &node->k);

    if (k > hash->length) {
        hash->flag_non_compact_elements = 1;
    }

    if (EXPECTED(hash->table[k].k.type == T_UNDEFINED)) {
        // 插入位置是空的
        lgx_gc_ref_add(&node->k);
        lgx_gc_ref_add(&node->v);
        hash->table[k].k = node->k;
        hash->table[k].v = node->v;

        lgx_hash_update_list(hash, &hash->table[k]);
    } else {
        // 插入位置已经有元素了，遍历寻找
        lgx_hash_node_t *next = &hash->table[k];
        while (next) {
            if (lgx_val_cmp(&next->k, &node->k)) {
                // 覆盖旧元素
                lgx_gc_ref_del(&next->v);
                lgx_gc_ref_add(&node->v);
                if (IS_GC_VALUE(&node->k) && node->k.v.gc->ref_cnt == 0) {
                    lgx_val_free(&node->k);
                }
                next->v = node->v;
                return 0;
            }
            next = next->next;
        }

        // 没有找到，新插入一个元素
        next = (lgx_hash_node_t *)xmalloc(sizeof(lgx_hash_node_t));
        if (UNEXPECTED(!next)) {
            return 1;
        }
        lgx_gc_ref_add(&node->k);
        lgx_gc_ref_add(&node->v);
        next->k = node->k;
        next->v = node->v;
        next->next = hash->table[k].next;
        hash->table[k].next = next;
        next->order = NULL;

        lgx_hash_update_list(hash, next);
    }
    
    hash->length ++;

    if (EXPECTED(hash->size > hash->length * (hash->flag_non_compact_elements ? 2 : 1))) {
        return 0;
    } else {
        return hash_resize(hash);
    }
}

int lgx_hash_add(lgx_hash_t *hash, lgx_val_t *v) {
    lgx_hash_node_t node;

    node.v = *v;

    // TODO 检查该 key 是否存在，确保没有 key 会被覆盖
    node.k.type = T_LONG;
    node.k.v.l = hash->length;

    return lgx_hash_set(hash, &node);
}

lgx_hash_node_t* lgx_hash_get(lgx_hash_t *hash, lgx_val_t *key) {
    unsigned k = hash_bkdr(hash, key);

    if (UNEXPECTED(key->type == T_UNDEFINED)) {
        return NULL;
    }

    lgx_hash_node_t *next = &hash->table[k];
    while (next) {
        if (lgx_val_cmp(&next->k, key)) {
            return next;
        }
        next = next->next;
    }

    return NULL;
}

lgx_hash_node_t* lgx_hash_find(lgx_hash_t *hash, lgx_val_t *v) {
    lgx_hash_node_t *next = hash->head;

    while (next) {
        if (lgx_val_cmp(&next->v, v)) {
            return next;
        }
        next = next->order;
    }

    return NULL;
}

int lgx_hash_cmp(lgx_hash_t *hash1, lgx_hash_t *hash2) {
    if (hash1 == hash2) {
        return 0;
    }

    if (hash1->length != hash2->length) {
        return 1;
    }

    lgx_hash_node_t *next1 = hash1->head, *next2 = hash2->head;
    while (next1 && next2) {
        if (!lgx_val_cmp(&next1->k, &next2->k) || !lgx_val_cmp(&next1->v, &next2->v)) {
            return 1;
        }
        next1 = next1->order;
        next2 = next2->order;
    }

    return 0;
}

void lgx_hash_print(lgx_hash_t *hash, int deep) {
    if (deep > 9) {
        printf("[\" ** excessive nesting ** \"]");
        return;
    }

    if (hash->flag_non_compact_elements) {
        printf("{");
    } else {
        printf("[");
    }

    lgx_hash_node_t *next = hash->head;
    int count = 0;
    while (next) {
        if (hash->flag_non_compact_elements) {
            lgx_val_print(&next->k, deep + 1);
            printf(":");
        }
        lgx_val_print(&next->v, deep + 1);
        if (++count < hash->length) {
            printf(",");
        }
        next = next->order;
    }

    if (hash->flag_non_compact_elements) {
        printf("}");
    } else {
        printf("]");
    }
}

lgx_val_t* lgx_hash_get_s(lgx_hash_t *hash, char *k) {
    lgx_str_t s;
    s.buffer = k;
    s.length = strlen(k);
    lgx_val_t key;
    key.type = T_STRING;
    key.v.str = &s;

    lgx_hash_node_t *n = lgx_hash_get(hash, &key);

    if (n == NULL) {
        return NULL;
    } else {
        return &n->v;
    }
}