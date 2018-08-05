#include "../common/common.h"
#include "gc.h"

void lgx_gc_enable(lgx_vm_t *vm) {
    vm->gc_enable = 1;
}

void lgx_gc_disable(lgx_vm_t *vm) {
    vm->gc_enable = 0;
}

void lgx_gc_free(lgx_val_t *val) {
    switch (val->type) {
        case T_STRING:
            if (val->v.str->gc.ref_cnt) {
                return;
            }
            xfree(val->v.str);
            val->type = T_UNDEFINED;
            break;
        case T_ARRAY:
            if (val->v.arr->gc.ref_cnt) {
                return;
            }
            lgx_hash_delete(val->v.arr);
            val->type = T_UNDEFINED;
            break;
        case T_OBJECT:
            // TODO
            break;
    }
}

static int full_gc(lgx_vm_t *vm) {
    // TODO
    printf("[full gc]\n");

    // TODO 遍历新生代、老年代，进行一遍标记

    // TODO 遍历新生代，执行清除

    // TODO 遍历老年代，执行清除、压缩

    return 1;
}

static int minor_gc(lgx_vm_t *vm) {
    int cnt = 0;

    lgx_list_t *list = vm->heap.young.next;
    while(list != &vm->heap.young) {
        lgx_list_t *next = list->next;
        lgx_list_del(list);
        
        switch (((lgx_gc_t*)list)->type) {
            case T_STRING: {
                unsigned size = sizeof(lgx_str_t) + ((lgx_str_t*)list)->size;
                if (((lgx_gc_t*)list)->ref_cnt == 0) {
                    xfree(list);
                } else {
                    lgx_list_add_tail(list, &vm->heap.old);

                    vm->heap.old_size += size;
                    cnt ++;
                }
                vm->heap.young_size -= size;
                break;
            }
            case T_ARRAY: {
                unsigned size = sizeof(lgx_hash_t) + ((lgx_str_t*)list)->size * sizeof(lgx_hash_node_t);
                if (((lgx_gc_t*)list)->ref_cnt == 0) {
                    lgx_hash_delete((lgx_hash_t *)list);
                    vm->heap.young_size -= size;
                } else {
                    /*
                    lgx_list_add_tail(list, &vm->heap.old);

                    vm->heap.young_size -= size;
                    vm->heap.old_size += size;
                    */
                    cnt ++;
                }
                break;
            }
        }

        list = next;
    }

    // TODO
    //printf("[minor gc] %d remain\n", cnt);

    if (vm->heap.old_size >= 4 * 1024 * 1024) {
        if (full_gc(vm)) {
            // Out Of Memory
            return 1;
        }

    }

    return 0;
}

int lgx_gc_trace(lgx_vm_t *vm, lgx_val_t*v) {
    switch (v->type) {
        case T_STRING:
            lgx_list_add_tail(&v->v.str->gc.head ,&vm->heap.young);
            vm->heap.young_size += sizeof(lgx_str_t) + v->v.str->size;
            break;
        case T_ARRAY:
            lgx_list_add_tail(&v->v.arr->gc.head ,&vm->heap.young);
            vm->heap.young_size += sizeof(lgx_hash_t) + v->v.arr->size * sizeof(lgx_hash_node_t);
            break;
        default:
            return 0;
    }

    if (vm->heap.young_size >= 4 * 1024 * 1024) {
        if (minor_gc(vm)) {
            // Out Of Memory
            return 1;
        }

    }

    return 0;
}
