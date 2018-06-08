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

// 函数拥有一个隐藏参数用于异常处理
// 类中的方法拥有一个隐藏参数用于访问对象
lgx_fun_t* lgx_fun_new();

#endif // LGX_FUN_H