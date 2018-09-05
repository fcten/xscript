#include "common.h"
#include "../interpreter/vm.h"
#include "fun.h"

lgx_fun_t* lgx_fun_new(unsigned args_num) {
    unsigned size = sizeof(lgx_fun_t) + args_num * sizeof(lgx_val_t);

    lgx_fun_t *fun =  xcalloc(1, size);
    if (UNEXPECTED(!fun)) {
        return NULL;
    }

    fun->gc.size = size;
    fun->gc.type = T_FUNCTION;

    fun->args_num = args_num;

    return fun;
}

void lgx_fun_delete(lgx_fun_t *fun) {
    xfree(fun);
}