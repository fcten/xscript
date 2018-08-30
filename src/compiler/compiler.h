#ifndef LGX_COMPILER_H
#define LGX_COMPILER_H

#include "../common/hash.h"
#include "../common/bytecode.h"
#include "../parser/ast.h"

typedef struct {
    unsigned char top;
    unsigned char max;
    unsigned char regs[256];
} lgx_reg_alloc_t;

typedef struct {
    lgx_ast_t *ast;

    lgx_reg_alloc_t *reg;
    
    // 字节码
    unsigned *bc;
    unsigned bc_size;
    unsigned bc_top;

    // 常量表
    lgx_hash_t* constant;

    // 全局变量
    lgx_hash_t* global;

    int errno;
    char *err_info;
    int err_len;
} lgx_bc_t;

int lgx_bc_compile(lgx_ast_t *ast, lgx_bc_t *bc);
int lgx_bc_cleanup(lgx_bc_t *bc);

#endif // LGX_COMPILER_H