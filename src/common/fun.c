#include "common.h"
#include "../interpreter/vm.h"
#include "fun.h"

lgx_fun_t* lgx_fun_new() {
    lgx_fun_t *fun =  xmalloc(sizeof(lgx_fun_t));
    if (UNEXPECTED(!fun)) {
        return NULL;
    }

    fun->gc.ref_cnt = 0;
    fun->gc.size = sizeof(lgx_fun_t);
    fun->gc.type = T_FUNCTION;

    return fun;
}

void lgx_fun_delete(lgx_fun_t *fun) {
    xfree(fun);
}