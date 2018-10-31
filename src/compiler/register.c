#include "../common/common.h"
#include "register.h"

lgx_reg_alloc_t* reg_allocator_new() {
    lgx_reg_alloc_t* a = (lgx_reg_alloc_t*)xmalloc(sizeof(lgx_reg_alloc_t));
    int i;
    a->max = 3;
    a->top = 0;
    for(i = 255; i >= 4; i--) {
        a->regs[a->top++] = i;
    }
    return a;
}

void reg_allocator_delete(lgx_reg_alloc_t* a) {
    xfree(a);
}

void reg_push(lgx_bc_t *bc, unsigned char i) {
    bc->reg->regs[bc->reg->top++] = i;
}

unsigned char reg_pop(lgx_bc_t *bc) {
    if (bc->reg->top > 0) {
        unsigned r = bc->reg->regs[--bc->reg->top];
        if (r > bc->reg->max) {
            bc->reg->max = r;
        }
        return r;
    } else {
        return -1;
    }
}

void reg_free(lgx_bc_t *bc, lgx_val_t *e) {
    if (e->u.c.type == R_TEMP) {
        reg_push(bc, e->u.c.reg);
        e->type = 0;
        e->v.l = 0;
        e->u.c.type = 0;
        e->u.c.reg = 0;
    }
}