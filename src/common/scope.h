#ifndef LGX_SCOPE_H
#define LGX_SCOPE_H

#include "list.h"
#include "hash.h"
#include "val.h"
#include "str.h"

typedef struct {
    lgx_list_t head;
    lgx_hash_t symbols;
} lgx_scope_t;

void lgx_scope_init(lgx_scope_t *root);
void lgx_scope_new(lgx_scope_t *root);
void lgx_scope_delete(lgx_scope_t *root);

void lgx_scope_val_set(lgx_scope_t *root, lgx_str_ref_t *s);
lgx_val_t* lgx_scope_val_get(lgx_scope_t *root, lgx_str_ref_t *s);

#endif // LGX_SCOPE_H