#include "../common/common.h"
#include "register.h"

void reg_push(lgx_bc_t *bc, unsigned char i) {
    bc->regs[bc->reg_top] = i;
    bc->reg_top ++;
}

unsigned char reg_pop(lgx_bc_t *bc) {
    // TODO 限制变量数量
    bc->reg_top --;
    return bc->regs[bc->reg_top];
}

void reg_free(lgx_bc_t *bc, lgx_val_t *e) {
    if (e->u.reg.type == R_TEMP) {
        reg_push(bc, e->u.reg.reg);
        e->type = 0;
        e->v.l = 0;
        e->u.reg.type = 0;
        e->u.reg.reg = 0;
    }
}