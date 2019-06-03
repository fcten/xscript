#ifndef LGX_CONSTANT_H
#define LGX_CONSTANT_H

// 常量表

#include "../common/ht.h"
#include "../parser/type.h"
#include "expr_result.h"

typedef struct {
    unsigned num;
    lgx_value_t v;
} lgx_const_t;

int lgx_const_get(lgx_ht_t* ct, lgx_expr_result_t* e);
int lgx_const_get_function(lgx_ht_t* ct, lgx_str_t* fun_name);
int lgx_const_update_function(lgx_ht_t* ct, lgx_function_t* fun);

lgx_const_t* lgx_const_new(lgx_ht_t* ct, lgx_expr_result_t* e);
void lgx_const_del(lgx_const_t* c);

void lgx_const_print(lgx_ht_t* ct);

#endif // LGX_CONSTANT_H