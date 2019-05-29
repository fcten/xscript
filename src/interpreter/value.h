#ifndef LGX_VALUE_H
#define LGX_VALUE_H

#include "../parser/type.h"

int lgx_value_dup(lgx_value_t* src, lgx_value_t* dst);

void lgx_value_print(lgx_value_t* v);

char* lgx_value_typeof(lgx_value_t* v);

int lgx_value_cmp(lgx_value_t* v1, lgx_value_t* v2);

#endif // LGX_VALUE_H