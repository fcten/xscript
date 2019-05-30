#include "common.h"
#include "ht.h"

int lgx_ht_init(lgx_ht_t* ht, unsigned size) {
    memset(ht, 0, sizeof(lgx_ht_t));
    
    // 规范化 size 取值
    size = ALIGN(size);
    if (size < LGX_HT_MIN_SIZE) {
        size = LGX_HT_MIN_SIZE;
    }

    ht->table = (lgx_ht_node_t **)xcalloc(size, sizeof(lgx_ht_node_t*));
    if (UNEXPECTED(!ht->table)) {
        return 1;
    }

    ht->length = 0;
    ht->size = size;

    return 0;
}

static unsigned ht_bkdr(lgx_ht_t* ht, lgx_str_t* k) {
    unsigned ret = 0;
    int i;
    
    for (i = 0; i < k->length; ++i) {
        ret = (ret << 5) - ret + k->buffer[i];
    }
    
    return ret;
}

static int ht_set(lgx_ht_t* ht, lgx_ht_node_t* node) {
    unsigned pos = node->hash % ht->size;

    // 插入位置是空的
    if (!ht->table[pos]) {
        node->next = NULL;
        ht->table[pos] = node;
        return 0;
    }

    lgx_ht_node_t* next = ht->table[pos];
    while (next) {
        if (lgx_str_cmp(&node->k, &next->k) == 0) {
            // 键已存在
            return 1;
        }
        next = next->next;
    }

    // 插入到链表头部
    node->next = ht->table[pos];
    ht->table[pos] = node;

    return 0;
}

static lgx_ht_node_t* ht_node_new(lgx_str_t* k, void* v) {
    lgx_ht_node_t* node = (lgx_ht_node_t*)xcalloc(1, sizeof(lgx_ht_node_t));
    if (!node) {
        return NULL;
    }

    if (lgx_str_init(&node->k, k->length)) {
        xfree(node);
        return NULL;
    }
    
    lgx_str_dup(k, &node->k);
    node->v = v;

    return node;
}

static void ht_node_del(lgx_ht_node_t* node) {
    assert(node->v == NULL);

    lgx_str_cleanup(&node->k);
    xfree(node);
}

void lgx_ht_cleanup(lgx_ht_t* ht) {
    int i;
    lgx_ht_node_t *node, *next;
    for (i = 0; i < ht->size; i++) {
        node = ht->table[i];
        while (node) {
            next = node->next;
            ht_node_del(node);
            node = next;
        }
    }

    xfree(ht->table);

    memset(ht, 0, sizeof(lgx_ht_t));
}

// 将 hash table 扩容一倍
static int ht_resize(lgx_ht_t* ht) {
    lgx_ht_t resize;
    if (UNEXPECTED(lgx_ht_init(&resize, ht->size * 2))) {
        return 1;
    }

    lgx_ht_node_t *n = ht->head, *next;
    while (n) {
        next = n->order;
        ht_set(&resize, n);
        n = next;
    }

    xfree(ht->table);

    ht->size = resize.size;
    ht->table = resize.table;
    ht->head = resize.head;
    ht->tail = resize.tail;

    return 0;
}

// 注意：键 k 会在插入哈希表时被自动复制，而值 v 不会。
// 因此，v 必须指向堆内存。而 k 如果指向堆内存，则必须在调用 lgx_ht_set 后手动释放。
// 如果键已存在，则会返回失败。
// 成功返回 0，失败返回 1
int lgx_ht_set(lgx_ht_t *ht, lgx_str_t* k, void* v) {
    // 键不能为空
    assert(k->length && k->buffer);

    lgx_ht_node_t* node = ht_node_new(k, v);
    if (!node) {
        return 1;
    }
    node->hash = ht_bkdr(ht, &node->k);

    if (ht_set(ht, node)) {
        // 键已存在
        node->v = NULL;
        ht_node_del(node);
        return 1;
    }

    ++ ht->length;

    if (!ht->tail) {
        ht->head = node;
        ht->tail = node;
    } else {
        ht->tail->order = node;
        ht->tail = node;
    }

    if (UNEXPECTED(ht->size < ht->length * 2)) {
        ht_resize(ht);
    }

    return 0;
}

lgx_ht_node_t* lgx_ht_get(lgx_ht_t *ht, lgx_str_t* k) {
    // 键不能为空
    assert(k->length && k->buffer);

    unsigned pos = ht_bkdr(ht, k) % ht->size;

    if (!ht->table[pos]) {
        return NULL;
    }

    lgx_ht_node_t *next = ht->table[pos];
    while (next) {
        if (lgx_str_cmp(&next->k, k) == 0) {
            return next;
        }
        next = next->next;
    }

    return NULL;
}

// 注意：lgx_ht_del 不会自动释放 v 所指向的内存，需要调用者自行处理。
int lgx_ht_del(lgx_ht_t *ht, lgx_str_t* k) {
    // 键不能为空
    assert(k->length && k->buffer);

    unsigned pos = ht_bkdr(ht, k) % ht->size;

    if (!ht->table[pos]) {
        return 0;
    }

    lgx_ht_node_t *next;

    if (lgx_str_cmp(&ht->table[pos]->k, k) == 0) {
        next = ht->table[pos]->next;
        ht_node_del(ht->table[pos]);
        ht->table[pos] = next;
        return 0;
    }

    lgx_ht_node_t* prev = next = ht->table[pos];
    next = next->next;
    while (next) {
        if (lgx_str_cmp(&next->k, k) == 0) {
            prev->next = next->next;
            ht_node_del(next);
            return 0;
        }
        prev = next;
        next = next->next;
    }

    return 0;
}

lgx_ht_node_t* lgx_ht_first(const lgx_ht_t* ht) {
    return ht->head;
}

lgx_ht_node_t* lgx_ht_next(const lgx_ht_node_t* node) {
    return node->order;
}
