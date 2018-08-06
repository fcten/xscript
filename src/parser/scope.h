#ifndef LGX_SCOPE_H
#define LGX_SCOPE_H

#include "../common/val.h"
#include "../common/str.h"
#include "../parser/ast.h"

void lgx_scope_val_add(lgx_ast_node_t *node, lgx_str_t *s);

lgx_val_t* lgx_scope_local_val_get(lgx_ast_node_t *node, lgx_str_t *s);
lgx_val_t* lgx_scope_global_val_get(lgx_ast_node_t *node, lgx_str_t *s);

#endif // LGX_SCOPE_H