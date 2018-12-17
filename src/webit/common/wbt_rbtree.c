/* 
 * File:   wbt_rbtree.c
 * Author: Fcten
 *
 * Created on 2014年11月12日, 下午2:58
 */

#include <stdio.h>
#include "../webit.h"
#include "wbt_rbtree.h"
#include "wbt_memory.h"
#include "wbt_string.h"

/* 1）每个结点要么是红的，要么是黑的。
 * 2）根结点是黑的。
 * 3）每个叶结点，即空结点（NIL）是黑的。
 * 4）如果一个结点是红的，那么它的俩个儿子都是黑的。
 * 5）对每个结点，从该结点到其子孙结点的所有路径上包含相同数目的黑结点。
 */

static wbt_inline void wbt_rbtree_set_parent(wbt_rb_node_t *node, wbt_rb_node_t *parent) {  
    node->parent_color = (node->parent_color & 3) | (unsigned long int)parent;  
}

static wbt_inline void wbt_rbtree_set_color(wbt_rb_node_t *node, wbt_rb_color_t color) {
    node->parent_color = (node->parent_color & ~1) | color;
}

//static wbt_inline void wbt_rbtree_set_parent_color(wbt_rb_node_t *node, wbt_rb_node_t *parent, wbt_rb_color_t color) {
//    node->parent_color = (unsigned long int)parent | color;
//}

static long long wbt_rbtree_compare(wbt_rb_t *rbt, wbt_rb_key_t *key1, wbt_rb_key_t *key2) {
    switch( rbt->key_type ) {
        case WBT_RB_KEY_INTEGER:
            if( *key1->str.i > *key2->str.i ) {
                return *key1->str.i - *key2->str.i;
            } else {
                return -(*key2->str.i - *key1->str.i);
            }
        case WBT_RB_KEY_LONGLONG:
            if( *key1->str.l > *key2->str.l ) {
                return *key1->str.l - *key2->str.l;
            } else {
                return -(*key2->str.l - *key1->str.l);
            }
        default:
            return wbt_strcmp((wbt_str_t *)key1, (wbt_str_t *)key2);
    }
}

static wbt_inline void wbt_rbtree_rotate_left(wbt_rb_t *rbt, wbt_rb_node_t *node) {
    wbt_rb_node_t  *temp;

    temp = node->right;
    node->right = temp->left;

    if (temp->left) {
        wbt_rbtree_set_parent(temp->left, node);
    }

    wbt_rbtree_set_parent(temp, wbt_rb_parent(node));

    if (node == rbt->root) {
        rbt->root = temp;
    } else if (node == wbt_rb_parent(node)->left) {
        wbt_rb_parent(node)->left = temp;
    } else {
        wbt_rb_parent(node)->right = temp;
    }

    temp->left = node;
    wbt_rbtree_set_parent(node, temp);
}

static wbt_inline void wbt_rbtree_rotate_right(wbt_rb_t *rbt, wbt_rb_node_t *node) {
    wbt_rb_node_t  *temp;

    temp = node->left;
    node->left = temp->right;

    if (temp->right) {
        wbt_rbtree_set_parent(temp->right, node);
    }

    wbt_rbtree_set_parent(temp, wbt_rb_parent(node));

    if (node == rbt->root) {
        rbt->root = temp;
    } else if (node == wbt_rb_parent(node)->right) {
        wbt_rb_parent(node)->right = temp;
    } else {
        wbt_rb_parent(node)->left = temp;
    }

    temp->right = node;
    wbt_rbtree_set_parent(node, temp);
}

void wbt_rbtree_insert_fixup(wbt_rb_t *rbt, wbt_rb_node_t *node) {  
    wbt_rb_node_t *parent, *gparent;  
  
    while ((parent = wbt_rb_parent(node)) && wbt_rb_is_red(parent)) {
        gparent = wbt_rb_parent(parent); 
  
        if (parent == gparent->left) {  
            {
                register wbt_rb_node_t *uncle = gparent->right; 
                if (uncle && wbt_rb_is_red(uncle)) {  
                    wbt_rb_set_black(uncle);
                    wbt_rb_set_black(parent);
                    wbt_rb_set_red(gparent);
                    node = gparent;
                    continue;
                }  
            }
  
            if (parent->right == node) {  
                register wbt_rb_node_t *tmp;  
                wbt_rbtree_rotate_left(rbt, parent);
                tmp = parent;
                parent = node;  
                node = tmp;  
            }  
  
            wbt_rb_set_black(parent);
            wbt_rb_set_red(gparent);
            wbt_rbtree_rotate_right(rbt, gparent);
        } else {
            {
                register wbt_rb_node_t *uncle = gparent->left;  
                if (uncle && wbt_rb_is_red(uncle)) {  
                    wbt_rb_set_black(uncle);  
                    wbt_rb_set_black(parent);  
                    wbt_rb_set_red(gparent);  
                    node = gparent;  
                    continue;  
                }
            }
  
            if (parent->left == node) {  
                register wbt_rb_node_t *tmp;  
                wbt_rbtree_rotate_right(rbt, parent);  
                tmp = parent;  
                parent = node;  
                node = tmp;  
            }  
  
            wbt_rb_set_black(parent);  
            wbt_rb_set_red(gparent);  
            wbt_rbtree_rotate_left(rbt, gparent);  
        }
    }
  
    wbt_rb_set_black(rbt->root);  
}

void wbt_rb_init(wbt_rb_t *rbt, wbt_rb_key_type_t type) {
    /*rbt->max = 1024;*/
    rbt->size = 0;
    rbt->root = NULL;
    rbt->key_type = type;
}

wbt_rb_node_t * wbt_rb_insert(wbt_rb_t *rbt, wbt_str_t *key) {
    wbt_rb_node_t *tmp_node, *tail_node;
    long long ret;

    tmp_node = (wbt_rb_node_t *)wbt_malloc(sizeof(wbt_rb_node_t));
    if( tmp_node == NULL ) {
        return NULL;
    }

    tmp_node->key.str.s = (char *)wbt_strdup(key->str, key->len);
    tmp_node->key.len = key->len;
    if( tmp_node->key.str.s == NULL ) {
        wbt_free(tmp_node);
        return NULL;
    }

    tmp_node->left = tmp_node->right = NULL;
    wbt_rbtree_set_parent(tmp_node, NULL);
    wbt_rb_set_red(tmp_node);

    if( rbt->root == NULL ) {
        rbt->root = tmp_node;
    } else {
        tail_node = rbt->root;
        while(1) {
            ret = wbt_rbtree_compare(rbt, (wbt_rb_key_t *)key, &tail_node->key);
            if( ret == 0 ) {
                /* 键值已经存在 */
                wbt_free(tmp_node->key.str.s);
                wbt_free(tmp_node);
                return NULL;
            } else if( ret > 0 ) {
                if( tail_node->right ) {
                    tail_node = tail_node->right;
                } else {
                    /* 找到了插入位置 */
                    tail_node->right = tmp_node;
                    wbt_rbtree_set_parent(tmp_node, tail_node);
                    break;
                }
            } else {
                if( tail_node->left ) {
                    tail_node = tail_node->left;
                } else {
                    /* 找到了插入位置 */
                    tail_node->left = tmp_node;
                    wbt_rbtree_set_parent(tmp_node, tail_node);
                    break;
                }
            }
        }
    }
    
    /* 插入节点后需要调整 */
    wbt_rbtree_insert_fixup( rbt, tmp_node );
    
    rbt->size ++;

    //wbt_log_debug("rbtree insert\n");
    
    return tmp_node;
}

// 在红黑树 rbt 中删除结点 node
void wbt_rb_delete(wbt_rb_t * rbt, wbt_rb_node_t * node) {
    wbt_rb_node_t * y = node; // 将要被删除的结点 
    wbt_rb_node_t * x = NULL; // y 的唯一子节点
    wbt_rb_node_t * x_parent = NULL;

    // 这里我们确定了要删除的节点 y，并确定了其唯一字节点 x，注意 x 可能为 NULL
    if (y->left == 0) {             // node has at most one non-null child. y == node.  
        x = y->right;               // x might be null.  
    } else {  
        if (y->right == 0) {        // node has exactly one non-null child.  y == node.  
            x = y->left;            // x is not null.  
        } else {                    // node has two non-null children.  Set y to  
            y = y->right;           //   node's successor.  x might be null.  
            while (y->left != 0) {
                y = y->left;
            }
            x = y->right;
        }
    }

    if (y != node) {                 // relink y in place of z.  y is z's successor  
        // 移动 node 的左子节点，使其成为 y 的左子节点
        wbt_rbtree_set_parent(node->left, y);
        y->left = node->left;

        if (y != node->right) {
            x_parent = wbt_rb_parent(y);
            if (x) {
                wbt_rbtree_set_parent(x, wbt_rb_parent(y));
            }
            wbt_rb_parent(y)->left = x;      // y must be a left child

            y->right = node->right;
            wbt_rbtree_set_parent(node->right, y);
        } else {
            x_parent = y;
        }

        if (rbt->root == node) {
            rbt->root = y;
        } else if (wbt_rb_parent(node)->left == node) {
            wbt_rb_parent(node)->left = y;
        } else {
            wbt_rb_parent(node)->right = y;
        }
        wbt_rbtree_set_parent(y, wbt_rb_parent(node));

        // 交换 y 与 node 的颜色
        wbt_rb_color_t tmp = wbt_rb_color(y);
        wbt_rbtree_set_color(y, wbt_rb_color(node));
        wbt_rbtree_set_color(node, tmp);

        y = node;  
        // y now points to node to be actually deleted  
    } else {                        // y == node
        x_parent = wbt_rb_parent(y);  
        if (x) {
            wbt_rbtree_set_parent(x, wbt_rb_parent(y));
        }
        if (rbt->root == node) {
            rbt->root = x;  
        } else {
            if (wbt_rb_parent(node)->left == node) {
                wbt_rb_parent(node)->left = x;  
            } else {
                wbt_rb_parent(node)->right = x;
            }
        }
    }

    // 删除黑色结点后，导致黑色缺失，违背性质4,故对树进行调整 
    if( wbt_rb_is_black(y) ) {
        while (x != rbt->root && (!x || wbt_rb_is_black(x))) {
            if (x == x_parent->left) {
                wbt_rb_node_t* uncle = x_parent->right;
                if(wbt_rb_is_red(uncle)) {
                    wbt_rb_set_black(uncle);
                    wbt_rb_set_red(x_parent);
                    wbt_rbtree_rotate_left(rbt, x_parent);  
                    uncle = x_parent->right;  
                }  
                if ((!uncle->left  || wbt_rb_is_black(uncle->left)) &&
                    (!uncle->right || wbt_rb_is_black(uncle->right))) {
                    wbt_rb_set_red(uncle);
                    x = x_parent;
                    x_parent = wbt_rb_parent(x_parent);
                } else {  
                    if (!uncle->right || wbt_rb_is_black(uncle->right)) {  
                        if (uncle->left) {
                            wbt_rb_set_black(uncle->left);
                        }
                        wbt_rb_set_red(uncle);
                        wbt_rbtree_rotate_right(rbt, uncle);
                        uncle = x_parent->right;
                    }
                    wbt_rbtree_set_color(uncle, wbt_rb_color(x_parent));
                    wbt_rb_set_black(x_parent);
                    if (uncle->right) {
                        wbt_rb_set_black(uncle->right);
                    }
                    wbt_rbtree_rotate_left(rbt, x_parent);  
                    break;
                }
            } else {                  // same as above, with right <-> left.  
                wbt_rb_node_t* uncle = x_parent->left;
                if (wbt_rb_is_red(uncle)) {
                    wbt_rb_set_black(uncle);
                    wbt_rb_set_red(x_parent);
                    wbt_rbtree_rotate_right(rbt, x_parent);  
                    uncle = x_parent->left;  
                }
                if ((!uncle->right || wbt_rb_is_black(uncle->right)) &&  
                    (!uncle->left  || wbt_rb_is_black(uncle->left))) {  
                    wbt_rb_set_red(uncle);
                    x = x_parent;  
                    x_parent = wbt_rb_parent(x_parent);
                } else {  
                    if (!uncle->left || wbt_rb_is_black(uncle->left)) {  
                        if (uncle->right) {
                            wbt_rb_set_black(uncle->right);
                        }
                        wbt_rb_set_red(uncle);
                        wbt_rbtree_rotate_left(rbt, uncle);  
                        uncle = x_parent->left;  
                    }  
                    wbt_rbtree_set_color(uncle, wbt_rb_color(x_parent));
                    wbt_rb_set_black(x_parent);
                    if (uncle->left) {
                        wbt_rb_set_black(uncle->left);
                    }
                    wbt_rbtree_rotate_right(rbt, x_parent);  
                    break;  
                }  
            }
        }
        if (x) {
            wbt_rb_set_black(x);
        }
    }

    /* 删除 y */
    wbt_free(y->key.str.s);
    wbt_free(y->value.str);
    wbt_free(y);

    rbt->size --;
    
    //wbt_rbtree_print(rbt->root);
    //wbt_log_debug("rbtree delete\n");
} 

wbt_rb_node_t * wbt_rb_get(wbt_rb_t *rbt, wbt_str_t *key) {
    wbt_rb_node_t *node;
    long long ret;
    
    node = rbt->root;
    
    while(node) {
        ret = wbt_rbtree_compare(rbt, (wbt_rb_key_t *)key, &node->key);
        if(ret == 0) {
            return node;
        } else if( ret > 0 ) {
            node = node->right;
        } else {
            node = node->left;
        }
    }
    
    return NULL;
}

void * wbt_rb_get_value(wbt_rb_t *rbt, wbt_str_t *key) {
    wbt_rb_node_t * node = wbt_rb_get(rbt, key);
    if( node == NULL ) {
        return NULL;
    } else {
        return node->value.str;
    }
}

// 获取一个小于 key 的最大的值
wbt_rb_node_t * wbt_rb_get_lesser(wbt_rb_t *rbt, wbt_str_t *key) {
    wbt_rb_node_t *node, *ret = NULL;
    
    node = rbt->root;
    
    while(node) {
        if(wbt_rbtree_compare(rbt, &node->key, (wbt_rb_key_t *)key) >= 0) {
            node = node->left;
        } else {
            ret = node;
            node = node->right;
        }
    }
    
    return ret;
}

// 获取一个小于等于 key 的最大的值
wbt_rb_node_t * wbt_rb_get_lesser_or_equal(wbt_rb_t *rbt, wbt_str_t *key) {
    wbt_rb_node_t *node, *ret = NULL;
    
    node = rbt->root;
    
    while(node) {
        if(wbt_rbtree_compare(rbt, &node->key, (wbt_rb_key_t *)key) > 0) {
            node = node->left;
        } else {
            ret = node;
            node = node->right;
        }
    }
    
    return ret;
}

// 获取一个大于 key 的最小的值
wbt_rb_node_t * wbt_rb_get_greater(wbt_rb_t *rbt, wbt_str_t *key) {
    wbt_rb_node_t *node, *ret = NULL;
    
    node = rbt->root;
    
    while(node) {
        if(wbt_rbtree_compare(rbt, &node->key, (wbt_rb_key_t *)key) > 0) {
            //wbt_log_debug("%lld > %lld\n", *node->key.str.l, (*(unsigned long long int *)key->str));
            ret = node;
            node = node->left;
        } else {
            //wbt_log_debug("%lld <= %lld\n", *node->key.str.l, (*(unsigned long long int *)key->str));
            node = node->right;
        }
    }
    
//    wbt_log_debug("============\n");
//    wbt_log_debug("bigger than %lld\n", (*(unsigned long long int *)key->str));
//    wbt_rbtree_print(rbt->root);
//    if(ret) {
//        wbt_log_debug("return %lld\n", (*(unsigned long long int *)ret->key.str));
//    }
//    wbt_log_debug("============\n");
    
    return ret;
}

// 获取一个大于等于 key 的最小的值
wbt_rb_node_t * wbt_rb_get_greater_or_equal(wbt_rb_t *rbt, wbt_str_t *key) {
    wbt_rb_node_t *node, *ret = NULL;
    
    node = rbt->root;
    
    while(node) {
        if(wbt_rbtree_compare(rbt, &node->key, (wbt_rb_key_t *)key) >= 0) {
            ret = node;
            node = node->left;
        } else {
            node = node->right;
        }
    }
    
    return ret;
}

wbt_rb_node_t * wbt_rb_get_min(wbt_rb_t *rbt) {
    wbt_rb_node_t *node;
    
    node = rbt->root;
    
    while(node) {
        if(node->left) {
            node = node->left;
        } else {
            return node;
        }
    }
    
    return node;
}

wbt_rb_node_t * wbt_rb_get_max(wbt_rb_t *rbt) {
    wbt_rb_node_t *node;
    
    node = rbt->root;
    
    while(node) {
        if(node->right) {
            node = node->right;
        } else {
            return node;
        }
    }
    
    return node;
}

wbt_rb_node_t * wbt_rb_first(const wbt_rb_t *rbt) {
    wbt_rb_node_t *n = rbt->root;  

    if(n == NULL) {
        return NULL;
    }

    while (n->left) { 
        n = n->left;
    }

    return n;  
}  
  
wbt_rb_node_t * wbt_rb_next(const wbt_rb_node_t *node) {
    wbt_rb_node_t *parent;  
  
    if (wbt_rb_parent(node) == node) {
        return NULL;
    }
  
    /* If we have a right-hand child, go down and then left as far 
       as we can. */  
    if (node->right) {  
        node = node->right;   
        while (node->left) {  
            node = node->left;
        }
        return (wbt_rb_node_t *)node;
    }  
  
    /* No right-hand children.  Everything down and left is 
       smaller than us, so any 'next' node must be in the general 
       direction of our parent. Go up the tree; any time the 
       ancestor is a right-hand child of its parent, keep going 
       up. First time it's a left-hand child of its parent, said 
       parent is our 'next' node. */  
    while ((parent = wbt_rb_parent(node)) && node == parent->right) {
        node = parent;
    }
  
    return parent;  
}

void wbt_rb_print(wbt_rb_node_t *node) {
    if(node) {
        wbt_rb_print(node->left);
        if( node->key.len == 4 ) {
            printf("%u\n", (*node->key.str.i));
        } else if( node->key.len == 8 ) {
            printf("%lld\n", (*node->key.str.l));
        } else {
            printf("%.*s\n", node->key.len, node->key.str.s);
        }
        wbt_rb_print(node->right);
    }
}

void wbt_rbtree_destroy_recursive(wbt_rb_node_t *node) {
    if(node) {
        wbt_rbtree_destroy_recursive(node->left);
        wbt_rbtree_destroy_recursive(node->right);
        
        wbt_free(node->key.str.s);
        wbt_free(node->value.str);
        wbt_free(node);
    }
}

void wbt_rb_destroy(wbt_rb_t *rbt) {
    wbt_rbtree_destroy_recursive(rbt->root);

    rbt->size = 0;
}

void wbt_rbtree_destroy_recursive_ignore_value(wbt_rb_node_t *node) {
    if(node) {
        wbt_rbtree_destroy_recursive_ignore_value(node->left);
        wbt_rbtree_destroy_recursive_ignore_value(node->right);
        
        wbt_free(node->key.str.s);
        //wbt_free(node->value.str);
        wbt_free(node);
    }
}

void wbt_rb_destroy_ignore_value(wbt_rb_t *rbt) {
    wbt_rbtree_destroy_recursive_ignore_value(rbt->root);
    
    rbt->size = 0;
}