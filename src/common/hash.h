#ifndef LGX_HASH_H
#define	LGX_HASH_H

#include "val.h"

typedef struct {
    // 该节点存储的 key 数量
    unsigned count;
    
    // 双向链表存储的 key
    lgx_val_list_t vl;
    
    // 红黑树存储的 key
    
} lgx_hash_key_t;

typedef struct {
    lgx_val_t k;
    lgx_val_t v;
} lgx_hash_node_t;

typedef struct lgx_hash_s {
    // GC 信息
    struct {
        // 引用计数
        unsigned ref_cnt;
        // GC 标记
        unsigned char color;
    } gc;

    // 总容量
    unsigned size;

    // 按 hash 结果存储 key
    lgx_hash_key_t *key;
    // TODO 暂时无用
    unsigned key_offset;

    // 按插入顺序存储
    lgx_hash_node_t *table;
    // table 的当前可用位置
    unsigned table_offset;
} lgx_hash_t;

int lgx_hash_init(lgx_hash_t *hash, unsigned size);
int lgx_hash_cleanup(lgx_hash_t *hash);

int lgx_hash_set(lgx_hash_t *hash, lgx_hash_node_t *node);
int lgx_hash_add(lgx_hash_t *hash, lgx_val_t *v);
int lgx_hash_get(lgx_hash_t *hash, lgx_val_t *k);

int lgx_hash_print(lgx_hash_t *hash);

#endif	/* LGX_HASH_H */