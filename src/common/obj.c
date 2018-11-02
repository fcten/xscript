#include "obj.h"
#include "hash.h"
#include "str.h"
#include "../interpreter/gc.h"

lgx_obj_t* lgx_obj_create(lgx_str_t *name) {
    lgx_obj_t *obj = xcalloc(1, sizeof(lgx_obj_t));
    if (!obj) {
        return NULL;
    }

    obj->name = lgx_str_new(name->buffer, name->length);
    obj->properties = lgx_hash_new(4);
    obj->methods = lgx_hash_new(4);

    if (!obj->name || !obj->properties || !obj->methods) {
        lgx_obj_delete(obj);
        return NULL;
    }

    return obj;
}

lgx_obj_t* lgx_obj_new(lgx_obj_t *obj) {
    return lgx_obj_create(obj->name);
}

int lgx_obj_delete(lgx_obj_t *obj) {
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
    lgx_hash_node_t *node = lgx_hash_get(obj->properties, k);
    if (!node) {
        return 1;
    }

    lgx_gc_ref_del(&node->v);
    node->v = *v;
    lgx_gc_ref_add(&node->v);

    return 0;
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

int lgx_obj_print(lgx_obj_t *obj) {
    return 0;
}