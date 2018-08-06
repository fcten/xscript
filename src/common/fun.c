#include "common.h"
#include "../interpreter/vm.h"
#include "fun.h"

lgx_fun_t* lgx_fun_new() {
    return xmalloc(sizeof(lgx_fun_t));
}

void lgx_fun_delete(lgx_fun_t *fun) {
    xfree(fun);
}