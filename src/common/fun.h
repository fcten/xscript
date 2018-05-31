#ifndef LGX_FUN_H
#define	LGX_FUN_H

typedef struct {
    
} lgx_fun_args_t;

typedef struct {
    lgx_fun_args_t args;
    union {
        struct lgx_ast_node_s* ast;
    } u;
} lgx_fun_t;

lgx_fun_t* lgx_fun_new();

#endif // LGX_FUN_H