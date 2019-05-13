#ifndef LGX_COMPILER_H
#define LGX_COMPILER_H

#include "../common/ht.h"
#include "../parser/ast.h"
#include "../common/rb.h"
#include "register.h"

typedef struct {
    lgx_ast_t ast;

    lgx_reg_t reg;
    
    // 字节码
    unsigned *bc;
    unsigned bc_size;
    unsigned bc_top;

    // 常量表
    lgx_ht_t constant;

    // 全局变量
    lgx_ht_t global;

    // 异常处理
    wbt_rb_t exception;
} lgx_compiler_t;

int lgx_compiler_init(lgx_compiler_t* c);
int lgx_compiler_cleanup(lgx_compiler_t* c);

#endif // LGX_COMPILER_H