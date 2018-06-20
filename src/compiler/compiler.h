#ifndef LGX_COMPILER_H
#define LGX_COMPILER_H

typedef struct {
    unsigned char regs[256];
    unsigned char reg_top;
    
    // 字节码
    unsigned *bc;
    unsigned bc_size;
    unsigned bc_top;

    // 常量表
    lgx_hash_t constant;

    // 全局变量
    lgx_hash_t global;

    int errno;
    char *err_info;
    int err_len;
} lgx_bc_t;

int lgx_bc_compile(lgx_ast_t *ast, lgx_bc_t *bc);

#endif // LGX_COMPILER_H