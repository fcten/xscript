#ifndef LGX_REGISTER_H
#define LGX_REGISTER_H

// 寄存器分配

typedef struct {
    unsigned top;
    unsigned max;
    unsigned char regs[256];
} lgx_reg_t;

int lgx_reg_init(lgx_reg_t* r);
void lgx_reg_cleanup(lgx_reg_t* r);

void lgx_reg_push(lgx_reg_t* r, unsigned char i);
int lgx_reg_pop(lgx_reg_t* r);

#endif // LGX_REGISTER_H