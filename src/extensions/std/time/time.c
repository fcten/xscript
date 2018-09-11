
#include "../../extension.h"

lgx_buildin_ext_t ext_std_time = {
    "std.time",
    std_time_init
};

int std_time_init() {
    lgx_fun_t *f = lgx_fun_new(0);

    lgx_ext_register_function(ext_std_time.package, "time", f);
}

int std_time(lgx_vm_t *vm) {



    return 0;
}