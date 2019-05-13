#ifndef LGX_RB_H
#define	LGX_RB_H

#include "str.h"

/* 定义红黑树结点颜色颜色类型 */
typedef enum { 
    LGX_RB_COLOR_RED = 0, 
    LGX_RB_COLOR_BLACK = 1 
} lgx_rb_color_t;

typedef enum { 
    LGX_RB_KEY_STRING = 0,
    LGX_RB_KEY_INTEGER = 1
} lgx_rb_key_type_t;

typedef struct lgx_rb_node_s {
    struct lgx_rb_node_s * left;
    struct lgx_rb_node_s * right;
    unsigned long long parent_color;
    lgx_str_t key;
    void* value;
} lgx_rb_node_t;

typedef struct lgx_rb_s {
    lgx_rb_node_t* root;
    unsigned int size;
    lgx_rb_key_type_t key_type;
} lgx_rb_t;

void lgx_rb_init(lgx_rb_t* rbt, lgx_rb_key_type_t type);
void lgx_rb_cleanup(lgx_rb_t *rbt);

lgx_rb_node_t* lgx_rb_set(lgx_rb_t* rbt, lgx_str_t* key);
void lgx_rb_del(lgx_rb_t* rbt, lgx_rb_node_t* node);
lgx_rb_node_t* lgx_rb_get(lgx_rb_t* rbt, lgx_str_t* key);
lgx_rb_node_t* lgx_rb_get_min(lgx_rb_t* rbt);
lgx_rb_node_t* lgx_rb_get_max(lgx_rb_t* rbt);
lgx_rb_node_t* lgx_rb_get_lesser(lgx_rb_t* rbt, lgx_str_t* key);
lgx_rb_node_t* lgx_rb_get_lesser_or_equal(lgx_rb_t* rbt, lgx_str_t* key);
lgx_rb_node_t* lgx_rb_get_greater(lgx_rb_t* rbt, lgx_str_t* key);
lgx_rb_node_t* lgx_rb_get_greater_or_equal(lgx_rb_t* rbt, lgx_str_t* key);

lgx_rb_node_t* lgx_rb_first(const lgx_rb_t* rbt);
lgx_rb_node_t* lgx_rb_next(const lgx_rb_node_t* node);

void lgx_rb_print(lgx_rb_t* rbt);

#endif	/* LGX_RB_H */

