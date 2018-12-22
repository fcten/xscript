#ifndef LGX_HASH_H
#define	LGX_HASH_H

#include "typedef.h"
#include "val.h"

#define HASH_HAS_NON_BASIC_ELEMENTS    1
#define HASH_HAS_NON_COMPACT_ELEMENTS  2

#define LGX_HASH_MIN_SIZE 8

#if defined(__GNUC__)
#define lgx_hash_new(size) \
	(__builtin_constant_p(size) ? \
		((((unsigned)(size)) <= LGX_HASH_MIN_SIZE) ? \
			_lgx_hash_new(LGX_HASH_MIN_SIZE) \
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

int lgx_hash_set(lgx_hash_t *hash, lgx_hash_node_t *node);
int lgx_hash_add(lgx_hash_t *hash, lgx_val_t *v);
lgx_hash_node_t* lgx_hash_get(lgx_hash_t *hash, lgx_val_t *k);
lgx_val_t* lgx_hash_get_s(lgx_hash_t *hash, char *k);
lgx_hash_node_t* lgx_hash_find(lgx_hash_t *hash, lgx_val_t *v);

int lgx_hash_copy(lgx_hash_t *src, lgx_hash_t *dst);

int lgx_hash_cmp(lgx_hash_t *hash1, lgx_hash_t *hash2);

int lgx_hash_print(lgx_hash_t *hash);

#endif	/* LGX_HASH_H */