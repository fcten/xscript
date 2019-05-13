#ifndef LGX_CONSTANT_H
#define LGX_CONSTANT_H

// 常量表

#include "../common/ht.h"
#include "../parser/type.h"

int lgx_const_set(lgx_ht_t* ct, lgx_str_t* k, lgx_value_t* v);
int lgx_const_get(lgx_ht_t* ct, lgx_str_t* k);

#endif // LGX_CONSTANT_H