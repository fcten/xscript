#ifndef LGX_FUN_H
#define	LGX_FUN_H

typedef struct {
    
} lgx_fun_args_t;

typedef struct {
    // 参数列表
    lgx_fun_args_t args;
    // 需求的堆栈空间
    unsigned stack_size;
    // 入口地址
    unsigned addr;
} lgx_fun_t;

// [0] 当前正在执行的函数
// [1] 返回值
// [2] 返回地址
// [3] 堆栈地址
// [4] 参数
// [5] 局部变量
// throw 和 return 的处理流程类似，区别在于 throw 能够跳出多层函数调用，直到被 catch。

lgx_fun_t* lgx_fun_new();
void lgx_fun_delete(lgx_fun_t *fun);

int lgx_fun_call(lgx_fun_t *fun);

#endif // LGX_FUN_H