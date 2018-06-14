#ifndef LGX_REGISTER_H
#define LGX_REGISTER_H

#include "code.h"

void reg_push(lgx_bc_t *bc, unsigned char i);
unsigned char reg_pop(lgx_bc_t *bc);
void reg_free(lgx_bc_t *bc, lgx_val_t *e);

#endif // LGX_REGISTER_H