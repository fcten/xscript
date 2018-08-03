#include "../common/common.h"
#include "constant.h"

int const_get(lgx_bc_t *bc, lgx_val_t *v) {
    lgx_hash_node_t *n = lgx_hash_find(bc->constant, v);
    if (!n) {
        bc->constant = lgx_hash_add(bc->constant, v);
        if (UNEXPECTED(!bc->constant)) {
            return -1;
        }
        n = lgx_hash_find(bc->constant, v);
    }

    return n->k.v.l;
}