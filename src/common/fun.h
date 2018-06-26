#ifndef LGX_FUN_H
#define	LGX_FUN_H

typedef struct {
    
} lgx_fun_args_t;

typedef struct {
    // 参数列表
    lgx_fun_args_t args;
    // 入口地址
    unsigned addr;
} lgx_fun_t;

// [0] 返回值   <--- top
// [1] 返回地址
// [2] this 指针
// [3] 参数
// [n] 局部变量
// throw 和 return 的处理流程类似，区别在于 throw 能够跳出多层函数调用，直到被 catch。

lgx_fun_t* lgx_fun_new();
int lgx_fun_call(lgx_fun_t* fun);

#endif // LGX_FUN_H