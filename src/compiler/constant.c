#include "constant.h"

int const_get(lgx_bc_t *bc, lgx_val_t *v) {
    int idx = lgx_hash_get(&bc->constant, v);
    if (idx >= 0) {
        return idx;
    } else {
        lgx_hash_node_t n;
        n.k = *v;
        n.v.type = T_UNDEFINED;
        if (lgx_hash_set(&bc->constant, &n) == 0) {
            return lgx_hash_get(&bc->constant, v);
        } else {
            return -1;
        }
    }
}