#include "common.h"
#include "../interpreter/vm.h"
#include "fun.h"

lgx_fun_t* lgx_fun_new() {
    return xmalloc(sizeof(lgx_fun_t));
}
