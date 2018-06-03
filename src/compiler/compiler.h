#ifndef LGX_COMPILER_H
#define LGX_COMPILER_H

#include "../parser/ast.h"

typedef struct {
    unsigned char regs[256];
    unsigned char reg_top;
    
    unsigned *bc;
    unsigned bc_size;
    unsigned bc_top;
} lgx_bc_t;

int lgx_bc_compile(lgx_ast_t *ast, lgx_bc_t *bc);

#endif // LGX_COMPILER_H