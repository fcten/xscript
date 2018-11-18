#ifndef LGX_RES_H
#define LGX_RES_H

#include "val.h"

lgx_res_t* lgx_res_new(unsigned type, void *buf);
int lgx_res_delete(lgx_res_t *res);

#endif // LGX_RES_H