#include <stdlib.h>
#include <memory.h>

#include "../interpreter/vm.h"
#include "fun.h"

lgx_fun_t* lgx_fun_new() {
    return malloc(sizeof(lgx_fun_t));
}

int lgx_fun_stack_init(lgx_fun_t* fun) {
    // TODO 根据需要决定栈大小
    int size = 256;

    if (fun->stack) {
        //memset(fun->stack, 0 , sizeof(lgx_stack_t) + size * sizeof(lgx_val_t*));
    } else {
        fun->stack = malloc(sizeof(lgx_stack_t) + size * sizeof(lgx_val_t*));
        if (!fun->stack) {
            return 1;
        }
    }

    fun->stack->size = size;
    fun->stack->fun = fun;
    fun->stack->ret_idx = -1;
    lgx_list_init(&fun->stack->head);

    return 0;
}

lgx_fun_t* lgx_fun_copy(lgx_fun_t* fun) {
    lgx_fun_t* ret = lgx_fun_new();

    if (ret) {
        memcpy(ret, fun, sizeof(lgx_fun_t));
        ret->stack = NULL;
    }

    return ret;
}
