#ifndef LGX_CODE_H
#define LGX_CODE_H

#include "../parser/ast.h"
#include "compiler.h"

#define is_instant8(e)  ((e)->type == T_LONG && (e)->v.l >= 0 && (e)->v.l <= 255)
#define is_instant16(e) ((e)->type == T_LONG && (e)->v.l >= 0 && (e)->v.l <= 65535)

#define is_number(e)    ((e)->type == T_LONG || (e)->type == T_DOUBLE)
#define is_bool(e)      ((e)->type == T_BOOL)

#define is_register(e)  ((e)->u.c.type)
#define is_global(e)    ((e)->u.c.type == R_GLOBAL)
#define is_local(e)     ((e)->u.c.type == R_LOCAL)
#define is_temp(e)      ((e)->u.c.type == R_TEMP)

void bc_nop(lgx_bc_t *bc);

void bc_load(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);
void bc_mov(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);

void bc_add(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_sub(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_mul(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_div(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_neg(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);

void bc_shl(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_shr(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_and(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_or(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_xor(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_not(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);
void bc_lnot(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);

void bc_eq(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_ne(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_lt(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_le(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_gt(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_ge(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);

void bc_call_new(lgx_bc_t *bc, unsigned char i);
void bc_call_set(lgx_bc_t *bc, unsigned char i, lgx_val_t *b);
void bc_call(lgx_bc_t *bc, lgx_val_t *a, unsigned char i);
void bc_ret(lgx_bc_t *bc, lgx_val_t *a);

void bc_test(lgx_bc_t *bc, lgx_val_t *a, unsigned pos);
void bc_jmp(lgx_bc_t *bc, lgx_val_t *a);
void bc_jmpi(lgx_bc_t *bc, unsigned pos);

void bc_echo(lgx_bc_t *bc, lgx_val_t *a);
void bc_hlt(lgx_bc_t *bc);

void bc_array_new(lgx_bc_t *bc, lgx_val_t *a);
void bc_array_add(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);
void bc_array_get(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_array_set(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);

void bc_global_get(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);
void bc_global_set(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);

void bc_object_new(lgx_bc_t *bc, lgx_val_t *a, unsigned char i);
void bc_object_get(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);
void bc_object_set(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c);

void bc_typeof(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);

void bc_set(lgx_bc_t *bc, unsigned pos, unsigned i);
void bc_set_pa(lgx_bc_t *bc, unsigned pos, unsigned pa);
void bc_set_pd(lgx_bc_t *bc, unsigned pos, unsigned pd);
void bc_set_pe(lgx_bc_t *bc, unsigned pos, unsigned pe);

void bc_throw(lgx_bc_t *bc, lgx_val_t *a);

void bc_await(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b);

#endif // LGX_CODE_H