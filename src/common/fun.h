#ifndef LGX_FUN_H
#define	LGX_FUN_H

#include "../parser/ast.h"

typedef struct {
    
} lgx_fun_args_t;

typedef struct {
    lgx_func_args_t args;
    lgx_ast_node_t* block;
} lgx_fun_t;

#endif // LGX_FUN_H