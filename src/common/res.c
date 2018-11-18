#include "common.h"
#include "res.h"

lgx_res_t* lgx_res_new(unsigned type, void *buf) {
    lgx_res_t *res = (lgx_res_t *)xcalloc(1, sizeof(lgx_res_t));
    if (!res) {
        return NULL;
    }

    res->type = type;
    res->buf = buf;

    wbt_list_init(&res->gc.head);

    return res;
}

int lgx_res_delete(lgx_res_t *res) {
    if (res->buf) {
        xfree(res->buf);
    }

    xfree(res);

    return 0;
}