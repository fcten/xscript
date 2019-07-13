#include "common.h"
#include "rb.h"

/* 1）每个结点要么是红的，要么是黑的。
 * 2）根结点是黑的。
 * 3）每个叶结点，即空结点（NIL）是黑的。
 * 4）如果一个结点是红的，那么它的俩个儿子都是黑的。
 * 5）对每个结点，从该结点到其子孙结点的所有路径上包含相同数目的黑结点。
 */

#define rb_parent(r)     ((lgx_rb_node_t *)((r)->parent_color & ~3))
#define rb_color(r)      ((lgx_rb_color_t)((r)->parent_color & 1))
#define rb_is_red(r)     (!rb_color(r))
#define rb_is_black(r)   rb_color(r)
#define rb_set_red(r)    do { (r)->parent_color &= ~1; } while (0)
#define rb_set_black(r)  do { (r)->parent_color |= 1; } while (0)

static lgx_inline void rb_set_parent(lgx_rb_node_t *node, lgx_rb_node_t *parent) {  
    node->parent_color = (node->parent_color & 3) | (uintptr_t)parent;  
}

static lgx_inline void rb_set_color(lgx_rb_node_t *node, lgx_rb_color_t color) {
    node->parent_color = (node->parent_color & ~1) | color;
}

//static lgx_inline void rb_set_parent_color(lgx_rb_node_t *node, lgx_rb_node_t *parent, lgx_rb_color_t color) {
//    node->parent_color = (uintptr_t)parent | color;
//}

static long long rb_compare(lgx_rb_t *rbt, lgx_str_t *key1, lgx_str_t *key2) {
    switch (rbt->key_type) {
        case LGX_RB_KEY_INTEGER: {
            long long k1 = *(long long *)key1->buffer;
            long long k2 = *(long long *)key2->buffer;

            return k1 - k2;
        }
        case LGX_RB_KEY_STRING:
        default:
            return lgx_str_cmp(key1, key2);
    }
}

static lgx_inline void rb_rotate_left(lgx_rb_t *rbt, lgx_rb_node_t *node) {
    lgx_rb_node_t  *temp;

    temp = node->right;
    node->right = temp->left;

    if (temp->left) {
        rb_set_parent(temp->left, node);
    }

    rb_set_parent(temp, rb_parent(node));

    if (node == rbt->root) {
        rbt->root = temp;
    } else if (node == rb_parent(node)->left) {
        rb_parent(node)->left = temp;
    } else {
        rb_parent(node)->right = temp;
    }

    temp->left = node;
    rb_set_parent(node, temp);
}

static lgx_inline void rb_rotate_right(lgx_rb_t *rbt, lgx_rb_node_t *node) {
    lgx_rb_node_t  *temp;

    temp = node->left;
    node->left = temp->right;

    if (temp->right) {
        rb_set_parent(temp->right, node);
    }

    rb_set_parent(temp, rb_parent(node));

    if (node == rbt->root) {
        rbt->root = temp;
    } else if (node == rb_parent(node)->right) {
        rb_parent(node)->right = temp;
    } else {
        rb_parent(node)->left = temp;
    }

    temp->right = node;
    rb_set_parent(node, temp);
}

void rb_insert_fixup(lgx_rb_t *rbt, lgx_rb_node_t *node) {  
    lgx_rb_node_t *parent, *gparent;  
  
    while ((parent = rb_parent(node)) && rb_is_red(parent)) {
        gparent = rb_parent(parent); 
  
        if (parent == gparent->left) {  
            {
                register lgx_rb_node_t *uncle = gparent->right; 
                if (uncle && rb_is_red(uncle)) {  
                    rb_set_black(uncle);
                    rb_set_black(parent);
                    rb_set_red(gparent);
                    node = gparent;
                    continue;
                }  
            }
  
            if (parent->right == node) {  
                register lgx_rb_node_t *tmp;  
                rb_rotate_left(rbt, parent);
                tmp = parent;
                parent = node;  
                node = tmp;  
            }  
  
            rb_set_black(parent);
            rb_set_red(gparent);
            rb_rotate_right(rbt, gparent);
        } else {
            {
                register lgx_rb_node_t *uncle = gparent->left;  
                if (uncle && rb_is_red(uncle)) {  
                    rb_set_black(uncle);  
                    rb_set_black(parent);  
                    rb_set_red(gparent);  
                    node = gparent;  
                    continue;  
                }
            }
  
            if (parent->left == node) {  
                register lgx_rb_node_t *tmp;  
                rb_rotate_right(rbt, parent);  
                tmp = parent;  
                parent = node;  
                node = tmp;  
            }  
  
            rb_set_black(parent);  
            rb_set_red(gparent);  
            rb_rotate_left(rbt, gparent);  
        }
    }
  
    rb_set_black(rbt->root);  
}

int lgx_rb_init(lgx_rb_t *rbt, lgx_rb_key_type_t type) {
    rbt->size = 0;
    rbt->root = NULL;
    rbt->key_type = type;

    return 0;
}

lgx_rb_node_t * lgx_rb_set(lgx_rb_t *rbt, lgx_str_t *key) {
    lgx_rb_node_t *tmp_node, *tail_node;
    long long ret;

    tmp_node = (lgx_rb_node_t *)xcalloc(1, sizeof(lgx_rb_node_t));
    if (tmp_node == NULL) {
        return NULL;
    }

    if (lgx_str_init(&tmp_node->key, key->length)) {
        xfree(tmp_node);
        return NULL;
    }
    lgx_str_dup(key, &tmp_node->key);

    tmp_node->left = tmp_node->right = NULL;
    rb_set_parent(tmp_node, NULL);
    rb_set_red(tmp_node);

    if (rbt->root == NULL) {
        rbt->root = tmp_node;
    } else {
        tail_node = rbt->root;
        while (1) {
            ret = rb_compare(rbt, key, &tail_node->key);
            if (ret == 0) {
                /* 键值已经存在 */
                lgx_str_cleanup(&tmp_node->key);
                xfree(tmp_node);
                return NULL;
            } else if (ret > 0) {
                if (tail_node->right) {
                    tail_node = tail_node->right;
                } else {
                    /* 找到了插入位置 */
                    tail_node->right = tmp_node;
                    rb_set_parent(tmp_node, tail_node);
                    break;
                }
            } else {
                if (tail_node->left) {
                    tail_node = tail_node->left;
                } else {
                    /* 找到了插入位置 */
                    tail_node->left = tmp_node;
                    rb_set_parent(tmp_node, tail_node);
                    break;
                }
            }
        }
    }
    
    /* 插入节点后需要调整 */
    rb_insert_fixup( rbt, tmp_node );
    
    rbt->size ++;

    //lgx_log_debug("rbtree insert\n");
    
    return tmp_node;
}

// 在红黑树 rbt 中删除结点 node
void lgx_rb_del(lgx_rb_t * rbt, lgx_rb_node_t * node) {
    lgx_rb_node_t * y = node; // 将要被删除的结点 
    lgx_rb_node_t * x = NULL; // y 的唯一子节点
    lgx_rb_node_t * x_parent = NULL;

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
        rb_set_parent(node->left, y);
        y->left = node->left;

        if (y != node->right) {
            x_parent = rb_parent(y);
            if (x) {
                rb_set_parent(x, rb_parent(y));
            }
            rb_parent(y)->left = x;      // y must be a left child

            y->right = node->right;
            rb_set_parent(node->right, y);
        } else {
            x_parent = y;
        }

        if (rbt->root == node) {
            rbt->root = y;
        } else if (rb_parent(node)->left == node) {
            rb_parent(node)->left = y;
        } else {
            rb_parent(node)->right = y;
        }
        rb_set_parent(y, rb_parent(node));

        // 交换 y 与 node 的颜色
        lgx_rb_color_t tmp = rb_color(y);
        rb_set_color(y, rb_color(node));
        rb_set_color(node, tmp);

        y = node;  
        // y now points to node to be actually deleted  
    } else {                        // y == node
        x_parent = rb_parent(y);  
        if (x) {
            rb_set_parent(x, rb_parent(y));
        }
        if (rbt->root == node) {
            rbt->root = x;  
        } else {
            if (rb_parent(node)->left == node) {
                rb_parent(node)->left = x;  
            } else {
                rb_parent(node)->right = x;
            }
        }
    }

    // 删除黑色结点后，导致黑色缺失，违背性质4,故对树进行调整 
    if( rb_is_black(y) ) {
        while (x != rbt->root && (!x || rb_is_black(x))) {
            if (x == x_parent->left) {
                lgx_rb_node_t* uncle = x_parent->right;
                if(rb_is_red(uncle)) {
                    rb_set_black(uncle);
                    rb_set_red(x_parent);
                    rb_rotate_left(rbt, x_parent);  
                    uncle = x_parent->right;  
                }  
                if ((!uncle->left  || rb_is_black(uncle->left)) &&
                    (!uncle->right || rb_is_black(uncle->right))) {
                    rb_set_red(uncle);
                    x = x_parent;
                    x_parent = rb_parent(x_parent);
                } else {  
                    if (!uncle->right || rb_is_black(uncle->right)) {  
                        if (uncle->left) {
                            rb_set_black(uncle->left);
                        }
                        rb_set_red(uncle);
                        rb_rotate_right(rbt, uncle);
                        uncle = x_parent->right;
                    }
                    rb_set_color(uncle, rb_color(x_parent));
                    rb_set_black(x_parent);
                    if (uncle->right) {
                        rb_set_black(uncle->right);
                    }
                    rb_rotate_left(rbt, x_parent);  
                    break;
                }
            } else {                  // same as above, with right <-> left.  
                lgx_rb_node_t* uncle = x_parent->left;
                if (rb_is_red(uncle)) {
                    rb_set_black(uncle);
                    rb_set_red(x_parent);
                    rb_rotate_right(rbt, x_parent);  
                    uncle = x_parent->left;  
                }
                if ((!uncle->right || rb_is_black(uncle->right)) &&  
                    (!uncle->left  || rb_is_black(uncle->left))) {  
                    rb_set_red(uncle);
                    x = x_parent;  
                    x_parent = rb_parent(x_parent);
                } else {  
                    if (!uncle->left || rb_is_black(uncle->left)) {  
                        if (uncle->right) {
                            rb_set_black(uncle->right);
                        }
                        rb_set_red(uncle);
                        rb_rotate_left(rbt, uncle);  
                        uncle = x_parent->left;  
                    }  
                    rb_set_color(uncle, rb_color(x_parent));
                    rb_set_black(x_parent);
                    if (uncle->left) {
                        rb_set_black(uncle->left);
                    }
                    rb_rotate_right(rbt, x_parent);  
                    break;  
                }  
            }
        }
        if (x) {
            rb_set_black(x);
        }
    }

    /* 删除 y */
    lgx_str_cleanup(&y->key);
    xfree(y);

    rbt->size --;
    
    //lgx_rbtree_print(rbt->root);
    //lgx_log_debug("rbtree delete\n");
} 

lgx_rb_node_t * lgx_rb_get(lgx_rb_t *rbt, lgx_str_t *key) {
    lgx_rb_node_t *node;
    long long ret;
    
    node = rbt->root;
    
    while(node) {
        ret = rb_compare(rbt, key, &node->key);
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

// 获取一个小于 key 的最大的值
lgx_rb_node_t * lgx_rb_get_lesser(lgx_rb_t *rbt, lgx_str_t *key) {
    lgx_rb_node_t *node, *ret = NULL;
    
    node = rbt->root;
    
    while(node) {
        if(rb_compare(rbt, &node->key, key) >= 0) {
            node = node->left;
        } else {
            ret = node;
            node = node->right;
        }
    }
    
    return ret;
}

// 获取一个小于等于 key 的最大的值
lgx_rb_node_t * lgx_rb_get_lesser_or_equal(lgx_rb_t *rbt, lgx_str_t *key) {
    lgx_rb_node_t *node, *ret = NULL;
    
    node = rbt->root;
    
    while(node) {
        if(rb_compare(rbt, &node->key, key) > 0) {
            node = node->left;
        } else {
            ret = node;
            node = node->right;
        }
    }
    
    return ret;
}

// 获取一个大于 key 的最小的值
lgx_rb_node_t * lgx_rb_get_greater(lgx_rb_t *rbt, lgx_str_t *key) {
    lgx_rb_node_t *node, *ret = NULL;
    
    node = rbt->root;
    
    while(node) {
        if(rb_compare(rbt, &node->key, key) > 0) {
            ret = node;
            node = node->left;
        } else {
            node = node->right;
        }
    }
    
    return ret;
}

// 获取一个大于等于 key 的最小的值
lgx_rb_node_t * lgx_rb_get_greater_or_equal(lgx_rb_t *rbt, lgx_str_t *key) {
    lgx_rb_node_t *node, *ret = NULL;
    
    node = rbt->root;
    
    while(node) {
        if(rb_compare(rbt, &node->key, key) >= 0) {
            ret = node;
            node = node->left;
        } else {
            node = node->right;
        }
    }
    
    return ret;
}

lgx_rb_node_t * lgx_rb_get_min(lgx_rb_t *rbt) {
    lgx_rb_node_t *node;
    
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

lgx_rb_node_t * lgx_rb_get_max(lgx_rb_t *rbt) {
    lgx_rb_node_t *node;
    
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

lgx_rb_node_t * lgx_rb_first(const lgx_rb_t *rbt) {
    lgx_rb_node_t *n = rbt->root;  

    if(n == NULL) {
        return NULL;
    }

    while (n->left) { 
        n = n->left;
    }

    return n;  
}  
  
lgx_rb_node_t * lgx_rb_next(const lgx_rb_node_t *node) {
    lgx_rb_node_t *parent;  
  
    if (rb_parent(node) == node) {
        return NULL;
    }
  
    /* If we have a right-hand child, go down and then left as far 
       as we can. */  
    if (node->right) {  
        node = node->right;   
        while (node->left) {  
            node = node->left;
        }
        return (lgx_rb_node_t *)node;
    }  
  
    /* No right-hand children.  Everything down and left is 
       smaller than us, so any 'next' node must be in the general 
       direction of our parent. Go up the tree; any time the 
       ancestor is a right-hand child of its parent, keep going 
       up. First time it's a left-hand child of its parent, said 
       parent is our 'next' node. */  
    while ((parent = rb_parent(node)) && node == parent->right) {
        node = parent;
    }
  
    return parent;  
}

static void rb_print(lgx_rb_t* rbt, lgx_rb_node_t *node) {
    if (node) {
        rb_print(rbt, node->left);
        switch (rbt->key_type) {
            case LGX_RB_KEY_INTEGER:
                printf("%lld\n", *(long long*)node->key.buffer);
            case LGX_RB_KEY_STRING:
            default:
                printf("%.*s\n", node->key.length, node->key.buffer);
        }
        rb_print(rbt, node->right);
    }
}

void lgx_rb_print(lgx_rb_t* rbt) {
    rb_print(rbt, rbt->root);
}

static void rb_cleanup(lgx_rb_node_t *node) {
    if (node) {
        rb_cleanup(node->left);
        rb_cleanup(node->right);
        
        lgx_str_cleanup(&node->key);
        xfree(node);
    }
}

void lgx_rb_cleanup(lgx_rb_t *rbt) {
    rb_cleanup(rbt->root);
    rbt->size = 0;
}
