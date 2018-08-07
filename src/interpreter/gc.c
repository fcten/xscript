#include "../common/common.h"
#include "gc.h"

void lgx_gc_enable(lgx_vm_t *vm) {
    vm->gc_enable = 1;
}

void lgx_gc_disable(lgx_vm_t *vm) {
    vm->gc_enable = 0;
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
                if (((lgx_gc_t*)list)->ref_cnt == 0) {
                    vm->heap.young_size -= ((lgx_gc_t*)list)->size;
                    xfree(list);
                } else {
                    lgx_list_add_tail(list, &vm->heap.old);

                    vm->heap.young_size -= ((lgx_gc_t*)list)->size;
                    vm->heap.old_size += ((lgx_gc_t*)list)->size;

                    cnt ++;
                }
                break;
            }
            case T_ARRAY: {
                if (((lgx_gc_t*)list)->ref_cnt == 0) {
                    vm->heap.young_size -= ((lgx_gc_t*)list)->size;
                    lgx_hash_delete((lgx_hash_t *)list);
                } else {
                    lgx_list_add_tail(list, &vm->heap.old);

                    vm->heap.young_size -= ((lgx_gc_t*)list)->size;
                    vm->heap.old_size += ((lgx_gc_t*)list)->size;

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
    if (IS_BASIC_VALUE(v)) {
        return 0;
    }

    lgx_list_add_tail(&v->v.gc->head ,&vm->heap.young);
    vm->heap.young_size += v->v.gc->size;

    if (vm->heap.young_size >= 4 * 1024 * 1024) {
        if (minor_gc(vm)) {
            // Out Of Memory
            return 1;
        }

    }

    return 0;
}
