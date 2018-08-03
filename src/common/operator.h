#ifndef LGX_OPERATOR_H
#define LGX_OPERATOR_H

#include "val.h"

int lgx_op_add(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_sub(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_mul(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_div(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_mod(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_shl(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_shr(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_gt(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_lt(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_ge(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_le(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_eq(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_ne(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_and(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_xor(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_or(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_land(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_lor(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);

int lgx_op_lnot(lgx_val_t *ret, lgx_val_t *right);
int lgx_op_not(lgx_val_t *ret, lgx_val_t *right);
int lgx_op_neg(lgx_val_t *ret, lgx_val_t *right);

int lgx_op_index(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);

int lgx_op_binary(int op, lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right);
int lgx_op_unary(int op, lgx_val_t *ret, lgx_val_t *right);

#endif // LGX_OPERATOR_H