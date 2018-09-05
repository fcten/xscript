#include "../common/hash.h"
#include "extension.h"

lgx_hash_t *lgx_extensions;

int lgx_ext_init() {
    lgx_extensions = lgx_hash_new(32);

    return 0;
}

int lgx_ext_add_function() {

}

int lgx_ext_add_const() {
    
}

int lgx_ext_add_class() {
    
}