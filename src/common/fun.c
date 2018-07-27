#include <stdlib.h>
#include <memory.h>

#include "../interpreter/vm.h"
#include "fun.h"

lgx_fun_t* lgx_fun_new() {
    return malloc(sizeof(lgx_fun_t));
}
