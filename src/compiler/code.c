#include "../common/common.h"
#include "../common/bytecode.h"
#include "constant.h"
#include "register.h"
#include "code.h"

static int bc_append(lgx_bc_t *bc, unsigned i) {
    if (bc->bc_top >= bc->bc_size) {
        bc->bc_size *= 2;
        bc->bc = xrealloc(bc->bc, bc->bc_size);
    }

    bc->bc[bc->bc_top] = i;
    bc->bc_top ++;

    return 0;
}

void bc_set(lgx_bc_t *bc, unsigned pos, unsigned i) {
    bc->bc[pos] = i;
}

void bc_set_pa(lgx_bc_t *bc, unsigned pos, unsigned pa) {
    bc->bc[pos] &= 0xFFFF00FF;
    bc->bc[pos] |= pa << 8;
}

void bc_set_pd(lgx_bc_t *bc, unsigned pos, unsigned pd) {
    bc->bc[pos] &= 0x0000FFFF;
    bc->bc[pos] |= pd << 16;
}

void bc_set_pe(lgx_bc_t *bc, unsigned pos, unsigned pe) {
    bc->bc[pos] &= 0x000000FF;
    bc->bc[pos] |= pe << 8;
}

void bc_nop(lgx_bc_t *bc) {
    bc_append(bc, I0(OP_NOP));
}

void bc_load(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b) {
    a->type = b->type;

    bc_append(bc, I2(OP_LOAD, a->u.reg.reg, const_get(bc, b)));
}

static void bc_load_to_reg(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b) {
    a->u.reg.type = R_TEMP;
    a->u.reg.reg = reg_pop(bc);
    
    bc_load(bc, a, b);
}

void bc_mov(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b) {
    a->type = b->type;

    // 在前一条指令为 mov、add 等指定指令时，直接复用
    if (b->u.reg.type == R_TEMP) {
        unsigned i = bc->bc[bc->bc_top-1];
        if ( (OP(i) >= OP_ADD && OP(i) <= OP_DIVI) ||
            (OP(i) >= OP_SHL && OP(i) <= OP_XOR) ||
            (OP(i) >= OP_EQ && OP(i) <= OP_LTI) ||
            (OP(i) == OP_ARRAY_NEW) ) {
            reg_free(bc, b);
            bc_set_pa(bc, bc->bc_top-1, a->u.reg.reg);
            return;
        }
    }

    if (!is_register(b)) {
        if (is_instant16(b)) {
            bc_append(bc, I2(OP_MOVI, a->u.reg.reg, b->v.l));
        } else {
            bc_load(bc, a, b);
        }
        return;
    }

    bc_append(bc, I2(OP_MOV, a->u.reg.reg, b->u.reg.reg));
}

void bc_add(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    if (is_number(b) && is_number(c)) {
        if (b->type == T_LONG && c->type == T_LONG) {
            a->type = T_LONG;
        } else {
            a->type = T_DOUBLE;
        }
    }

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_ADDI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_ADD, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        if (is_instant8(b)) {
            bc_append(bc, I3(OP_ADDI, a->u.reg.reg, c->u.reg.reg, b->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, b);
            bc_append(bc, I3(OP_ADD, a->u.reg.reg, r.u.reg.reg, c->u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    bc_append(bc, I3(OP_ADD, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_sub(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    if (is_number(b) && is_number(c)) {
        if (b->type == T_LONG && c->type == T_LONG) {
            a->type = T_LONG;
        } else {
            a->type = T_DOUBLE;
        }
    }

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_SUBI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_SUB, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        lgx_val_t r;
        bc_load_to_reg(bc, &r, b);
        bc_append(bc, I3(OP_SUB, a->u.reg.reg, r.u.reg.reg, c->u.reg.reg));
        reg_free(bc, &r);
        return;
    }

    bc_append(bc, I3(OP_SUB, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_mul(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    if (is_number(b) && is_number(c)) {
        if (b->type == T_LONG && c->type == T_LONG) {
            a->type = T_LONG;
        } else {
            a->type = T_DOUBLE;
        }
    }

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_MULI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_MUL, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        if (is_instant8(b)) {
            bc_append(bc, I3(OP_MULI, a->u.reg.reg, c->u.reg.reg, b->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, b);
            bc_append(bc, I3(OP_MUL, a->u.reg.reg, r.u.reg.reg, c->u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    bc_append(bc, I3(OP_MUL, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_div(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    if (is_number(b) && is_number(c)) {
        if (b->type == T_LONG && c->type == T_LONG) {
            a->type = T_LONG;
        } else {
            a->type = T_DOUBLE;
        }
    }

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_DIVI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_DIV, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        lgx_val_t r;
        bc_load_to_reg(bc, &r, b);
        bc_append(bc, I3(OP_DIV, a->u.reg.reg, r.u.reg.reg, c->u.reg.reg));
        reg_free(bc, &r);
        return;
    }

    bc_append(bc, I3(OP_DIV, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_neg(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b) {
    a->type = b->type;

    bc_append(bc, I2(OP_NEG, a->u.reg.reg, b->u.reg.reg));
}

void bc_shl(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    a->type = T_LONG;

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_SHLI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_SHL, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        lgx_val_t r;
        bc_load_to_reg(bc, &r, b);
        bc_append(bc, I3(OP_SHL, a->u.reg.reg, r.u.reg.reg, c->u.reg.reg));
        reg_free(bc, &r);
        return;
    }

    bc_append(bc, I3(OP_SHL, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_shr(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    a->type = T_LONG;

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_SHRI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_SHRI, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        lgx_val_t r;
        bc_load_to_reg(bc, &r, b);
        bc_append(bc, I3(OP_SHR, a->u.reg.reg, r.u.reg.reg, c->u.reg.reg));
        reg_free(bc, &r);
        return;
    }

    bc_append(bc, I3(OP_SHR, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_and(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    bc_append(bc, I3(OP_AND, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_or(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    bc_append(bc, I3(OP_OR, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_xor(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    bc_append(bc, I3(OP_XOR, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_not(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b) {
    bc_append(bc, I2(OP_NOT, a->u.reg.reg, b->u.reg.reg));
}

void bc_lnot(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b) {
    a->type = T_BOOL;

    bc_append(bc, I2(OP_LNOT, a->u.reg.reg, b->u.reg.reg));
}

void bc_eq(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    a->type = T_BOOL;

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_EQI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_EQ, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        if (is_instant8(b)) {
            bc_append(bc, I3(OP_EQI, a->u.reg.reg, c->u.reg.reg, b->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, b);
            bc_append(bc, I3(OP_EQ, a->u.reg.reg, r.u.reg.reg, c->u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    bc_append(bc, I3(OP_EQ, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_ne(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    bc_eq(bc, a, b, c);
    bc_lnot(bc, a, a);
}

void bc_lt(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    a->type = T_BOOL;

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_LTI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_LT, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        if (is_instant8(b)) {
            bc_append(bc, I3(OP_GTI, a->u.reg.reg, c->u.reg.reg, b->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, b);
            bc_append(bc, I3(OP_LT, a->u.reg.reg, r.u.reg.reg, c->u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    bc_append(bc, I3(OP_LT, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_le(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    a->type = T_BOOL;

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_LEI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_LE, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        if (is_instant8(b)) {
            bc_append(bc, I3(OP_GEI, a->u.reg.reg, c->u.reg.reg, b->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, b);
            bc_append(bc, I3(OP_LE, a->u.reg.reg, r.u.reg.reg, c->u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    bc_append(bc, I3(OP_LE, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_gt(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    a->type = T_BOOL;

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_GTI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_LT, a->u.reg.reg, r.u.reg.reg, b->u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        if (is_instant8(b)) {
            bc_append(bc, I3(OP_LTI, a->u.reg.reg, c->u.reg.reg, b->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, b);
            bc_append(bc, I3(OP_LT, a->u.reg.reg, c->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    bc_append(bc, I3(OP_LT, a->u.reg.reg, c->u.reg.reg, b->u.reg.reg));
}

void bc_ge(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    a->type = T_BOOL;

    if (!is_register(c)) {
        if (is_instant8(c)) {
            bc_append(bc, I3(OP_GEI, a->u.reg.reg, b->u.reg.reg, c->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, c);
            bc_append(bc, I3(OP_LE, a->u.reg.reg, r.u.reg.reg, b->u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    if (!is_register(b)) {
        if (is_instant8(b)) {
            bc_append(bc, I3(OP_LEI, a->u.reg.reg, c->u.reg.reg, b->v.l));
        } else {
            lgx_val_t r;
            bc_load_to_reg(bc, &r, b);
            bc_append(bc, I3(OP_LE, a->u.reg.reg, c->u.reg.reg, r.u.reg.reg));
            reg_free(bc, &r);
        }
        return;
    }

    bc_append(bc, I3(OP_LE, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
}

void bc_call_new(lgx_bc_t *bc, lgx_val_t *a) {
    bc_append(bc, I1(OP_CALL_NEW, a->u.reg.reg));
}

void bc_call_set(lgx_bc_t *bc, unsigned char i, lgx_val_t *b) {
    if (!is_register(b)) {
        lgx_val_t r;
        bc_load_to_reg(bc, &r, b);
        bc_append(bc, I2(OP_CALL_SET, i, r.u.reg.reg));
        reg_free(bc, &r);
    } else {
        bc_append(bc, I2(OP_CALL_SET, i, b->u.reg.reg));
    }
}

void bc_call(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b) {
    bc_append(bc, I2(OP_CALL, a->u.reg.reg, b->u.reg.reg));
}

void bc_ret(lgx_bc_t *bc, lgx_val_t *a) {
    if (!is_register(a)) {
        lgx_val_t r;
        bc_load_to_reg(bc, &r, a);
        bc_append(bc, I1(OP_RET, r.u.reg.reg));
        reg_free(bc, &r);
    } else {
        bc_append(bc, I1(OP_RET, a->u.reg.reg));
    }
}

void bc_test(lgx_bc_t *bc, lgx_val_t *a, unsigned pos) {
    bc_append(bc, I2(OP_TEST, a->u.reg.reg, pos));
}

void bc_jmp(lgx_bc_t *bc, lgx_val_t *a) {
    bc_append(bc, I1(OP_JMP, a->u.reg.reg));
}

void bc_jmpi(lgx_bc_t *bc, unsigned pos) {
    bc_append(bc, I1(OP_JMPI, pos));
}

void bc_echo(lgx_bc_t *bc, lgx_val_t *a) {
    if (!is_register(a)) {
        lgx_val_t r;
        bc_load_to_reg(bc, &r, a);
        bc_append(bc, I1(OP_ECHO, r.u.reg.reg));
        reg_free(bc, &r);
    } else {
        bc_append(bc, I1(OP_ECHO, a->u.reg.reg));
    }
}

void bc_hlt(lgx_bc_t *bc) {
    bc_append(bc, I0(OP_HLT));
}

void bc_array_new(lgx_bc_t *bc, lgx_val_t *a) {
    bc_append(bc, I1(OP_ARRAY_NEW, a->u.reg.reg));
}

void bc_array_add(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b) {
    if (!is_register(b)) {
        lgx_val_t r;
        bc_load_to_reg(bc, &r, b);
        bc_append(bc, I2(OP_ARRAY_ADD, a->u.reg.reg, r.u.reg.reg));
        reg_free(bc, &r);
    } else {
        bc_append(bc, I2(OP_ARRAY_ADD, a->u.reg.reg, b->u.reg.reg));
    }
}

void bc_array_get(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    if (!is_register(c)) {
        lgx_val_t r;
        bc_load_to_reg(bc, &r, c);
        bc_append(bc, I3(OP_ARRAY_GET, a->u.reg.reg, b->u.reg.reg, r.u.reg.reg));
        reg_free(bc, &r);
    } else {
        bc_append(bc, I3(OP_ARRAY_GET, a->u.reg.reg, b->u.reg.reg, c->u.reg.reg));
    }
}

void bc_array_set(lgx_bc_t *bc, lgx_val_t *a, lgx_val_t *b, lgx_val_t *c) {
    lgx_val_t rb, rc;
    int fb = 0, fc = 0;

    rb = *b;
    rc = *c;

    if (!is_register(b)) {
        bc_load_to_reg(bc, &rb, b);
        fb = 1;
    }

    if (!is_register(c)) {
        bc_load_to_reg(bc, &rc, c);
        fc = 1;
    }

    bc_append(bc, I3(OP_ARRAY_SET, a->u.reg.reg, rb.u.reg.reg, rc.u.reg.reg));

    if (fb) {
        reg_free(bc, &rb);
    }

    if (fc) {
        reg_free(bc, &rc);
    }
}