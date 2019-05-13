#ifndef LGX_HT_H
#define	LGX_HT_H

#include "str.h"

#define LGX_HT_MIN_SIZE 8

typedef struct lgx_ht_node_s {
    // 下一个相同哈希值的元素
    struct lgx_ht_node_s* next;

    // 下一个插入的元素
    struct lgx_ht_node_s* order;

	// 当前哈希节点的内容
    lgx_str_t k;
    void* v;

    // 当前节点的哈希值
    unsigned hash;
} lgx_ht_node_t;

typedef struct lgx_ht_s {
    // 总容量
    unsigned size;

    // 已使用的容量
    unsigned length;

    // 存储数据的结构
    lgx_ht_node_t** table;

    // 依据插入顺序保存的链表，以便遍历
    lgx_ht_node_t* head;
    lgx_ht_node_t* tail;
} lgx_ht_t;

int lgx_ht_init(lgx_ht_t* ht, unsigned size);
int lgx_ht_cleanup(lgx_ht_t* ht);

int lgx_ht_set(lgx_ht_t *ht, lgx_str_t* k, void* v);
lgx_ht_node_t* lgx_ht_get(lgx_ht_t *ht, lgx_str_t* k);
int lgx_ht_del(lgx_ht_t *ht, lgx_str_t* k);

lgx_ht_node_t* lgx_ht_first(const lgx_ht_t* ht);
lgx_ht_node_t* lgx_ht_next(const lgx_ht_node_t* node);

#endif	/* LGX_HT_H */