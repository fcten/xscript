#ifndef LGX_RES_H
#define LGX_RES_H

#include "val.h"

#define LGX_CO_RES_TYPE 0x20181117

lgx_res_t* lgx_res_new(unsigned type, void *buf);
int lgx_res_delete(lgx_res_t *res);

#endif // LGX_RES_H