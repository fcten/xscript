#ifndef LGX_HASH_H
#define	LGX_HASH_H

#include "list.h"
#include "val.h"

typedef struct {
    lgx_list_t head;
    lgx_val_t k;
    lgx_val_t v;
} lgx_hash_node_t;

typedef struct lgx_hash_s {
    // GC 信息
    struct {
        lgx_list_t head;
        // 该 gc 所管理的类型
        unsigned char type;
        // GC 标记
        unsigned char color;
        // 引用计数
        unsigned ref_cnt;
    } gc;

    // 总容量
    unsigned size;

    // 已使用的容量
    unsigned length;

    // 存储数据的结构
    lgx_hash_node_t table[];
} lgx_hash_t;

#define LGX_HASH_MIN_SIZE 8

#if defined(__GNUC__)
#define lgx_hash_new(size) \
	(__builtin_constant_p(size) ? \
		((((unsigned)(size)) <= LGX_HASH_MIN_SIZE) ? \
			_lgx_hash_new_const() \
		: \
			_lgx_hash_new((size)) \
		) \
	: \
		_lgx_hash_new((size)) \
	)
#else
#define lgx_hash_new(size) _lgx_hash_new(size)
#endif

lgx_hash_t* _lgx_hash_new(unsigned size);
lgx_hash_t* _lgx_hash_new_const();
int lgx_hash_delete(lgx_hash_t *hash);

lgx_hash_t* lgx_hash_set(lgx_hash_t *hash, lgx_hash_node_t *node);
lgx_hash_t* lgx_hash_add(lgx_hash_t *hash, lgx_val_t *v);
lgx_hash_node_t* lgx_hash_get(lgx_hash_t *hash, lgx_val_t *k);
lgx_hash_node_t* lgx_hash_find(lgx_hash_t *hash, lgx_val_t *v);

int lgx_hash_print(lgx_hash_t *hash);

#endif	/* LGX_HASH_H */