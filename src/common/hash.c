#include "common.h"
#include "hash.h"

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

// TODO 引用计数
int lgx_hash_delete(lgx_hash_t *hash) {
    if (hash->flag_non_basic_elements) {
        int i, length;
        if (hash->flag_non_compact_elements) {
            length = hash->size;
        } else {
            length = hash->length;
        }
        for (i = 0; i < length; i ++) {
            if (hash->table[i].k.type == T_UNDEFINED) {
                continue;
            }
            while (!lgx_list_empty(&hash->table[i].head)) {
                lgx_hash_node_t *n = lgx_list_first_entry(&hash->table[i].head, lgx_hash_node_t, head);
                lgx_list_del(&n->head);
                xfree(n);
            }
        }
    }

    xfree(hash);

    return 0;
}

// 把 src 的元素复制到 dst 中
// dst->size 应当大于等于 src->size
// TODO 不复制 T_UNDEFINED 类型的元素
static void hash_copy(lgx_hash_t *src, lgx_hash_t *dst) {
    int i;
    for (i = 0; i < src->size; i++) {
        if (src->table[i].k.type == T_UNDEFINED) {
            continue;
        }
        lgx_hash_set(dst, &src->table[i]);
        lgx_hash_node_t *p;
        lgx_list_for_each_entry(p, lgx_hash_node_t, &src->table[i].head, head) {
            lgx_hash_set(dst, p);
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

// TODO value 复制需要复制内部数据结构，否则会导致循环引用
// TODO 引用计数
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

    if (node->v.type > T_BOOL) {
        hash->flag_non_basic_elements = 1;
    }

    if (EXPECTED(hash->table[k].k.type == T_UNDEFINED)) {
        // 插入位置是空的
        hash->table[k] = *node;
        lgx_list_init(&hash->table[k].head);

        hash->length ++;
    } else {
        // 插入位置已经有元素了，遍历寻找
        // TODO 节点本身也包含一个 value
        lgx_hash_node_t *p;
        lgx_list_for_each_entry(p, lgx_hash_node_t, &hash->table[k].head, head) {
            if (lgx_val_cmp(&p->k, &node->k)) {
                // 覆盖旧元素
                p->v = node->v;
                return 0;
            }
        }

        // 没有找到，新插入一个元素
        p = xmalloc(sizeof(lgx_hash_node_t));
        if (UNEXPECTED(!p)) {
            return NULL;
        }
        *p = *node;
        lgx_list_add_tail(&p->head, &hash->table[k].head);

        hash->length ++;
    }
    
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

    if (hash->table[k].k.type == T_UNDEFINED) {
        return NULL;
    }

    if (lgx_val_cmp(&hash->table[k].k, key)) {
        return &hash->table[k];
    }

    lgx_hash_node_t *p;
    lgx_list_for_each_entry(p, lgx_hash_node_t, &hash->table[k].head, head) {
        if (lgx_val_cmp(&p->k, key)) {
            return p;
        }
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
        if (hash->table[i].k.type == T_UNDEFINED) {
            continue;
        }
        if (lgx_val_cmp(&hash->table[i].v, v)) {
            return &hash->table[i];
        }
        lgx_hash_node_t *p;
        lgx_list_for_each_entry(p, lgx_hash_node_t, &hash->table[i].head, head) {
            if (lgx_val_cmp(&p->v, v)) {
                return p;
            }
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