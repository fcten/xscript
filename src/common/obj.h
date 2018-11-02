#ifndef LGX_OBJ_H
#define	LGX_OBJ_H

#include "val.h"

lgx_obj_t* lgx_obj_create(lgx_str_t *name);
int lgx_obj_delete(lgx_obj_t *obj);

lgx_obj_t* lgx_obj_new(lgx_obj_t *obj);

int lgx_obj_set(lgx_obj_t *obj, lgx_val_t *k, lgx_val_t *v);
lgx_val_t* lgx_obj_get(lgx_obj_t *obj, lgx_val_t *k);

int lgx_obj_add_property(lgx_obj_t *obj, lgx_hash_node_t *node);
int lgx_obj_add_method(lgx_obj_t *obj, lgx_hash_node_t *node);

int lgx_obj_print(lgx_obj_t *obj);

#endif // LGX_OBJ_H