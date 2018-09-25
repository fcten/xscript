#ifndef LGX_FUN_H
#define	LGX_FUN_H

#include "val.h"

// [0] 当前正在执行的函数
// [1] 返回值地址
// [2] 返回地址
// [3] 堆栈地址
// [4] 参数
// [5] 局部变量
// throw 和 return 的处理流程类似，区别在于 throw 能够跳出多层函数调用，直到被 catch。

lgx_fun_t* lgx_fun_new(unsigned args_num);
void lgx_fun_delete(lgx_fun_t *fun);

int lgx_fun_call(lgx_fun_t *fun);

#endif // LGX_FUN_H