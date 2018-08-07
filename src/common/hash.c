#include "common.h"
#include "hash.h"
#include "../interpreter/gc.h"

lgx_hash_t* _lgx_hash_new_const();
int lgx_hash_delete(lgx_hash_t *hash);

lgx_hash_t* _lgx_hash_new(unsigned size) {
    // 规范化 size 取值
    size = ALIGN(size);

    unsigned mem_size = sizeof(lgx_hash_t) + size * sizeof(lgx_hash_node_t);

    lgx_hash_t *hash = xcalloc(1, mem_size);
    if (UNEXPECTED(!hash)) {
        return NULL;
    }

    hash->size = size;

    hash->gc.size = mem_size;
    hash->gc.type = T_ARRAY;

    return hash;
}

lgx_hash_t* _lgx_hash_new_const() {
    static lgx_hash_t *hash = NULL;

    if (UNEXPECTED(!hash)) {
        hash = _lgx_hash_new(LGX_HASH_MIN_SIZE);
        if (UNEXPECTED(!hash)) {
            return NULL;
        }
        hash->gc.ref_cnt = 1;
    }

    return hash;
}

// 删除哈希表，并删除有所引用计数小于等于 1 的子元素
int lgx_hash_delete(lgx_hash_t *hash) {
    if (hash->flag_non_basic_elements) {
        int i, length;
        if (hash->flag_non_compact_elements) {
            length = hash->size;
        } else {
            length = hash->length;
        }
        for (i = 0; i < length; i ++) {
            lgx_val_free(&hash->table[i].k);
            lgx_val_free(&hash->table[i].v);
            lgx_hash_node_t *cur, *next = hash->table[i].next;
            while (next) {
                cur = next;
                next = cur->next;
                lgx_val_free(&cur->k);
                lgx_val_free(&cur->v);
                xfree(cur);
            }
        }
    }

    xfree(hash);

    return 0;
}

// 把 src 的元素复制到 dst 中，忽略 undefined 类型的 key
// dst->size 应当大于等于 src->size
static void hash_copy(lgx_hash_t *src, lgx_hash_t *dst) {
    int i;
    for (i = 0; i < src->size; i++) {
        if (src->table[i].k.type != T_UNDEFINED) {
            lgx_hash_set(dst, &src->table[i]);
        }
        lgx_hash_node_t *next = src->table[i].next;
        while (next) {
            if (next->k.type != T_UNDEFINED) {
                lgx_hash_set(dst, next);
            }
            next = next->next;
        }
    }
}

// 将 hash table 扩容一倍
// TODO 引用计数
static lgx_hash_t* hash_resize(lgx_hash_t *hash) {
    lgx_hash_t *resize = lgx_hash_new(hash->size * 2);
    if (UNEXPECTED(!resize)) {
        return NULL;
    }

    hash_copy(hash, resize);

    // 更新 GC 信息
    hash->gc.size = resize->gc.size;
    resize->gc = hash->gc;

    resize->gc.head.prev->next = &resize->gc.head;
    resize->gc.head.next->prev = &resize->gc.head;

    lgx_hash_delete(hash);

    return resize;
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

// TODO 正确处理循环引用
lgx_hash_t* lgx_hash_set(lgx_hash_t *hash, lgx_hash_node_t *node) {
    // 如果引用计数大于 1，则进行复制
    if (UNEXPECTED(hash->gc.ref_cnt > 1)) {
        hash->gc.ref_cnt --;
        lgx_hash_t *dst = _lgx_hash_new(hash->size);
        if (hash->length) {
            hash_copy(hash, dst);
        }
        hash = dst;
        hash->gc.ref_cnt = 1;
    }

    unsigned k = hash_bkdr(hash, &node->k);

    if (k > hash->length) {
        hash->flag_non_compact_elements = 1;
    }

    if (IS_GC_VALUE(&node->v) || IS_GC_VALUE(&node->k)) {
        hash->flag_non_basic_elements = 1;
    }

    if (EXPECTED(hash->table[k].k.type == T_UNDEFINED)) {
        // 插入位置是空的
        lgx_gc_ref_add(&node->k);
        lgx_gc_ref_add(&node->v);
        hash->table[k].k = node->k;
        hash->table[k].v = node->v;
    } else {
        // 插入位置已经有元素了，遍历寻找
        lgx_hash_node_t *next = hash->table[k].next;
        while (next) {
            if (lgx_val_cmp(&next->k, &node->k)) {
                // 覆盖旧元素
                lgx_gc_ref_del(&next->v);
                lgx_gc_ref_add(&node->v);
                next->v = node->v;
                return 0;
            }
            next = next->next;
        }

        // 没有找到，新插入一个元素
        next = xmalloc(sizeof(lgx_hash_node_t));
        if (UNEXPECTED(!next)) {
            return NULL;
        }
        lgx_gc_ref_add(&node->k);
        lgx_gc_ref_add(&node->v);
        next->k = node->k;
        next->v = node->v;
        next->next = hash->table[k].next;
        hash->table[k].next = next;
    }
    
    hash->length ++;

    if (EXPECTED(hash->size > hash->length)) {
        return hash;
    } else {
        return hash_resize(hash);
    }
}

lgx_hash_t* lgx_hash_add(lgx_hash_t *hash, lgx_val_t *v) {
    lgx_hash_node_t node;

    node.v = *v;

    node.k.type = T_LONG;
    node.k.v.l = hash->length;

    return lgx_hash_set(hash, &node);
}

lgx_hash_node_t* lgx_hash_get(lgx_hash_t *hash, lgx_val_t *key) {
    unsigned k = hash_bkdr(hash, key);

    if (UNEXPECTED(key->type == T_UNDEFINED)) {
        return NULL;
    }

    if (lgx_val_cmp(&hash->table[k].k, key)) {
        return &hash->table[k];
    }

    lgx_hash_node_t *next = hash->table[k].next;
    while (next) {
        if (lgx_val_cmp(&next->k, key)) {
            return next;
        }
        next = next->next;
    }

    return NULL;
}

lgx_hash_node_t* lgx_hash_find(lgx_hash_t *hash, lgx_val_t *v) {
    int i, length;

    if (hash->flag_non_compact_elements) {
        length = hash->size;
    } else {
        length = hash->length;
    }

    for (i = 0; i < length; i++) {
        if (lgx_val_cmp(&hash->table[i].v, v)) {
            return &hash->table[i];
        }
        lgx_hash_node_t *next = hash->table[i].next;
        while (next) {
            if (lgx_val_cmp(&next->v, v)) {
                return next;
            }
            next = next->next;
        }
    }

    return NULL;
}

int lgx_hash_print(lgx_hash_t *hash) {
    int i, length, count = 0;

    if (hash->flag_non_compact_elements) {
        length = hash->size;
    } else {
        length = hash->length;
    }

    printf("[");

    for (i = 0 ; i < length ; i ++) {
        if (hash->table[i].k.type == T_UNDEFINED) {
            continue;
        }
        //lgx_val_print(&hash->table[i].k);
        //printf("=>");
        lgx_val_print(&hash->table[i].v);
        if (++count < hash->length) {
            printf(",");
        }
        //printf("\n");
    }

    printf("]");

    return 0;
}