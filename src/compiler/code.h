#ifndef LGX_CODE_H
#define LGX_CODE_H

#include "../parser/ast.h"

typedef struct {
    unsigned char regs[256];
    unsigned char reg_top;
    
    // 字节码
    unsigned *bc;
    unsigned bc_size;
    unsigned bc_top;

    // 常量表
    lgx_hash_t constant;

    int errno;
    char *err_info;
    int err_len;
} lgx_bc_t;

#define is_instant8(e)  ((e)->type == T_LONG && (e)->v.l >= 0 && (e)->v.l <= 255)
#define is_instant16(e) ((e)->type == T_LONG && (e)->v.l >= 0 && (e)->v.l <= 65535)

#define is_number(e)    ((e)->type == T_LONG || (e)->type == T_DOUBLE)

#define is_register(e)  ((e)->u.reg.type)

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

void bc_test(lgx_bc_t *bc, lgx_val_t *a, unsigned pos);
void bc_jmp(lgx_bc_t *bc, unsigned pos);

void bc_echo(lgx_bc_t *bc, lgx_val_t *a);
void bc_hlt(lgx_bc_t *bc);

#define bc_eq(a, b, c)   bc_append(bc, I3(OP_EQ, a, b, c))
#define bc_le(a, b, c)   bc_append(bc, I3(OP_LE, a, b, c))
#define bc_lt(a, b, c)   bc_append(bc, I3(OP_LT, a, b, c))
#define bc_eqi(a, b, c)  bc_append(bc, I3(OP_EQI, a, b, c))
#define bc_gti(a, b, c)  bc_append(bc, I3(OP_GTI, a, b, c))
#define bc_gei(a, b, c)  bc_append(bc, I3(OP_GEI, a, b, c))
#define bc_lti(a, b, c)  bc_append(bc, I3(OP_LTI, a, b, c))
#define bc_lei(a, b, c)  bc_append(bc, I3(OP_LEI, a, b, c))

void bc_set(lgx_bc_t *bc, unsigned pos, unsigned i);
void bc_set_pa(lgx_bc_t *bc, unsigned pos, unsigned pa);
void bc_set_pd(lgx_bc_t *bc, unsigned pos, unsigned pd);
void bc_set_pe(lgx_bc_t *bc, unsigned pos, unsigned pe);

#endif // LGX_CODE_H