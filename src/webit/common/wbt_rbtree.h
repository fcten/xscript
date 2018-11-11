/* 
 * File:   wbt_rbtree.h
 * Author: Fcten
 *
 * Created on 2014年11月12日, 下午2:58
 */

#ifndef __WBT_RBTREE_H__
#define	__WBT_RBTREE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "wbt_memory.h"
#include "wbt_string.h"

/* 定义红黑树结点颜色颜色类型 */
typedef enum { 
    WBT_RB_COLOR_RED = 0, 
    WBT_RB_COLOR_BLACK = 1 
} wbt_rb_color_t;

typedef enum { 
    WBT_RB_KEY_STRING = 0, 
    WBT_RB_KEY_INTEGER = 1,
    WBT_RB_KEY_LONGLONG = 2
} wbt_rb_key_type_t;

typedef struct wbt_rb_key_s {
    int len;
    union {
        char *s;
        unsigned int *i;
        unsigned long long int *l;
    } str;
} wbt_rb_key_t;

typedef struct wbt_rb_node_s {
    struct wbt_rb_node_s * left;
    struct wbt_rb_node_s * right;
    // 在 gcc 中，unsigned long int 总是和指针长度一致
    unsigned long int parent_color;
    wbt_rb_key_t key;
    wbt_str_t value;
} wbt_rb_node_t;

#define wbt_rb_parent(r)   ((wbt_rb_node_t *)((r)->parent_color & ~3))
#define wbt_rb_color(r)    ((wbt_rb_color_t)((r)->parent_color & 1))
#define wbt_rb_is_red(r)   (!wbt_rb_color(r))
#define wbt_rb_is_black(r) wbt_rb_color(r)
#define wbt_rb_set_red(r)  do { (r)->parent_color &= ~1; } while (0)
#define wbt_rb_set_black(r)  do { (r)->parent_color |= 1; } while (0)

typedef struct wbt_rb_s {
    wbt_rb_node_t * root;
    unsigned int size;
    wbt_rb_key_type_t key_type;
} wbt_rb_t;

void wbt_rb_init(wbt_rb_t *rbt, wbt_rb_key_type_t type);

void wbt_rb_print(wbt_rb_node_t *node);

wbt_rb_node_t * wbt_rb_insert(wbt_rb_t *rbt, wbt_str_t *key);

void wbt_rb_delete(wbt_rb_t *rbt, wbt_rb_node_t *node);

wbt_rb_node_t * wbt_rb_get(wbt_rb_t *rbt, wbt_str_t *key);
void * wbt_rb_get_value(wbt_rb_t *rbt, wbt_str_t *key);
wbt_rb_node_t * wbt_rb_get_min(wbt_rb_t *rbt);
wbt_rb_node_t * wbt_rb_get_max(wbt_rb_t *rbt);
wbt_rb_node_t * wbt_rb_get_lesser(wbt_rb_t *rbt, wbt_str_t *key);
wbt_rb_node_t * wbt_rb_get_lesser_or_equal(wbt_rb_t *rbt, wbt_str_t *key);
wbt_rb_node_t * wbt_rb_get_greater(wbt_rb_t *rbt, wbt_str_t *key);
wbt_rb_node_t * wbt_rb_get_greater_or_equal(wbt_rb_t *rbt, wbt_str_t *key);

void wbt_rb_destroy(wbt_rb_t *rbt);
void wbt_rb_destroy_ignore_value(wbt_rb_t *rbt);

wbt_rb_node_t * wbt_rb_first(const wbt_rb_t *rbt);
wbt_rb_node_t * wbt_rb_next(const wbt_rb_node_t *node);

#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_RBTREE_H__ */

