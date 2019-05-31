#ifndef LGX_COMPILER_H
#define LGX_COMPILER_H

#include <stdint.h>
#include "../common/str.h"
#include "../common/ht.h"
#include "../parser/ast.h"
#include "../common/rb.h"
#include "register.h"

typedef struct {
    // 当前解析的 ast 结构
    lgx_ast_t* ast;

    // 字节码
    struct {
        unsigned length;
        unsigned size;
        uint32_t* buffer;
    } bc;

    // 常量
    lgx_ht_t constant;

    // 全局变量
    lgx_ht_t global;

    // 异常处理信息
    lgx_rb_t exception;
} lgx_compiler_t;

int lgx_compiler_init(lgx_compiler_t* c);
int lgx_compiler_generate(lgx_compiler_t* c, lgx_ast_t *ast);
void lgx_compiler_cleanup(lgx_compiler_t* c);

#endif // LGX_COMPILER_H