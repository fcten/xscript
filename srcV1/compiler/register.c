#include <memory.h>
#include <assert.h>
#include "register.h"

void lgx_reg_push(lgx_reg_t* r, unsigned char i) {
    assert(r->top < 256);

    r->regs[r->top] = i;
    ++ r->top;
}

int lgx_reg_pop(lgx_reg_t* r) {
    if (!r->top) {
        return -1;
    }

    -- r->top;
    unsigned char reg = r->regs[r->top];

    if (reg > r->max) {
        r->max = reg;
    }

    return reg;
}


int lgx_reg_init(lgx_reg_t* r) {
    r->max = 0;
    r->top = 0;

    int i;
    for(i = 255; i >= 0; --i) {
        lgx_reg_push(r, i);
    }

    return 0;
}

void lgx_reg_cleanup(lgx_reg_t* r) {
    memset(r, 0, sizeof(lgx_reg_t));
}