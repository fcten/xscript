#ifndef LGX_CAST_H
#define LGX_CAST_H

#include "val.h"

// 自动类型转换
int lgx_cast_long(lgx_val_t *ret, lgx_val_t *src);
int lgx_cast_double(lgx_val_t *ret, lgx_val_t *src);
int lgx_cast_string(lgx_val_t *ret, lgx_val_t *src);
int lgx_cast_array(lgx_val_t *ret, lgx_val_t *src);
int lgx_cast_bool(lgx_val_t *ret, lgx_val_t *src);

// TODO 强制类型转换

#endif // LGX_CAST_H