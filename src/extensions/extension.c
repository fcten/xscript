#include "../common/hash.h"
#include "extension.h"

int lgx_ext_init(lgx_vm_t *vm) {
//    lgx_extensions = lgx_hash_new(32);

    return 0;
}

int lgx_ext_register_function(const char *package, const char *name, lgx_fun_t *fun) {
    fun->is_buildin = 1;


}

int lgx_ext_register_const(lgx_vm_t *vm) {
    
}

int lgx_ext_register_class(lgx_vm_t *vm) {
    
}