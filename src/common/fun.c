#include "common.h"
#include "../interpreter/vm.h"
#include "fun.h"

lgx_fun_t* lgx_fun_new(unsigned args_num) {
    unsigned size = sizeof(lgx_fun_t) + args_num * sizeof(lgx_val_t);

    lgx_fun_t *fun =  (lgx_fun_t *)xcalloc(1, size);
    if (UNEXPECTED(!fun)) {
        return NULL;
    }

    fun->gc.size = size;
    fun->gc.type = T_FUNCTION;
    wbt_list_init(&fun->gc.head);

    fun->args_num = args_num;

    // 不含任何局部变量和临时变量情况下所需要的最小栈长度
    fun->stack_size = 4 + args_num;
    // 如果是 method，则还要额外 + 1 以容纳隐藏的 this 参数
    // TODO 区分是否为 method
    fun->stack_size ++;

    return fun;
}

void lgx_fun_delete(lgx_fun_t *fun) {
    if (!wbt_list_empty(&fun->gc.head)) {
        wbt_list_del(&fun->gc.head);
    }

    xfree(fun);
}