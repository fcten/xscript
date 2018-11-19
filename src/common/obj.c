#include "common.h"
#include "obj.h"
#include "hash.h"
#include "str.h"
#include "../interpreter/gc.h"

lgx_obj_t* lgx_obj_create(lgx_str_t *name) {
    lgx_obj_t *obj = (lgx_obj_t *)xcalloc(1, sizeof(lgx_obj_t));
    if (!obj) {
        return NULL;
    }

    obj->name = lgx_str_new(name->buffer, name->length);
    obj->properties = lgx_hash_new(4);
    obj->methods = lgx_hash_new(4);

    wbt_list_init(&obj->gc.head);

    if (!obj->name || !obj->properties || !obj->methods) {
        lgx_obj_delete(obj);
        return NULL;
    }

    return obj;
}

lgx_obj_t* lgx_obj_new(lgx_obj_t *obj) {
    lgx_obj_t *object = lgx_obj_create(obj->name);
    object->parent = obj;
    return object;
}

int lgx_obj_delete(lgx_obj_t *obj) {
    if (!wbt_list_empty(&obj->gc.head)) {
        wbt_list_del(&obj->gc.head);
    }

    if (obj->name) {
        lgx_str_delete(obj->name);
    }

    if (obj->properties) {
        lgx_hash_delete(obj->properties);
    }

    if (obj->methods) {
        lgx_hash_delete(obj->methods);
    }

    xfree(obj);

    return 0;
}

int lgx_obj_set(lgx_obj_t *obj, lgx_val_t *k, lgx_val_t *v) {
    lgx_val_t *val = lgx_obj_get(obj, k);
    if (!val || val->type == T_FUNCTION) {
        return 1;
    }

    lgx_hash_node_t node;
    node.k = *k;
    node.v = *v;

    return lgx_hash_set(obj->properties, &node);
}

lgx_val_t* lgx_obj_get(lgx_obj_t *obj, lgx_val_t *k) {
    lgx_hash_node_t *node = lgx_hash_get(obj->properties, k);
    if (node) {
        return &node->v;
    }

    node = lgx_hash_get(obj->methods, k);
    if (node) {
        return &node->v;
    }

    if (obj->parent) {
        return lgx_obj_get(obj->parent, k);
    } else {
        return NULL;
    }
}

int lgx_obj_add_property(lgx_obj_t *obj, lgx_hash_node_t *node) {
    if (lgx_hash_get(obj->properties, &node->k)) {
        // 属性已存在
        return 1;
    }

    return lgx_hash_set(obj->properties, node);
}

int lgx_obj_add_method(lgx_obj_t *obj, lgx_hash_node_t *node) {
    if (lgx_hash_get(obj->methods, &node->k)) {
        // 方法已存在
        return 1;
    }

    return lgx_hash_set(obj->methods, node);
}

lgx_str_t * lgx_obj_get_name(lgx_obj_t *obj) {
    while (obj->name == NULL) {
        obj = obj->parent;
        if (!obj) {
            return NULL;
        }
    }

    return obj->name;
}

int lgx_obj_is_same_class(lgx_obj_t *obj1, lgx_obj_t *obj2) {
    lgx_str_t *n1 = lgx_obj_get_name(obj1);
    lgx_str_t *n2 = lgx_obj_get_name(obj2);

    assert(n1 && n2);

    return lgx_str_cmp(n1, n2);
}

int lgx_obj_is_instanceof(lgx_obj_t *obj, lgx_str_t *name) {
    while (obj) {
        if (obj->name && lgx_str_cmp(obj->name, name) == 0) {
            return 1;
        } else {
            obj = obj->parent;
        }
    }

    return 0;
}

int lgx_obj_print(lgx_obj_t *obj) {
    printf("\n{\n\t\"properties\":");
    lgx_hash_print(obj->properties);
    printf(",\n\t\"methods\":");
    lgx_hash_print(obj->methods);
    printf(",\n\t\"parent\":");
    if (obj->parent) {
        lgx_obj_print(obj->parent);
    } else {
        printf("null");
    }
    printf(",\n}\n");
    return 0;
}