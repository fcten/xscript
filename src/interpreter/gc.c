#include <stdlib.h>

#include "gc.h"

void lgx_gc_enable(lgx_vm_t *vm) {
    vm->gc_enable = 1;
}

void lgx_gc_disable(lgx_vm_t *vm) {
    vm->gc_enable = 0;
}

void lgx_gc_refer(lgx_val_t *val) {
    val->u.gc.ref_cnt ++;
}

void lgx_gc_release(lgx_val_t *val) {
    val->u.gc.ref_cnt --;
}

static int minor_gc(lgx_vm_t *vm, lgx_vm_stack_t *stack) {
    int i;
    // 第一次遍历，根据引用计数进行垃圾回收
    for (i = 0; i < stack->size; i++) {
        lgx_val_t *v = &stack->buf[i];

        if (v->type == T_UNDEFINED || v->u.gc.ref_cnt) {
            continue;
        }

        // TODO 释放引用计数为 0 的变量
        
        v->type = T_UNDEFINED;
    }
    // 第二次遍历，把没有被释放的变量移动到老年代
    for (i = 0; i < stack->size; i++) {
        lgx_val_t *v = &stack->buf[i];

        if (v->type != T_UNDEFINED && v->u.gc.ref_cnt) {
            if (vm->heap.old.base >= vm->heap.old.size) {
                // 老年代已满
                if (full_gc(vm)) {
                    return 1;
                }
            }
            // 把变量移动到老年代
            vm->heap.old.buf[vm->heap.old.base++] = *v;
            v->type = T_UNDEFINED;
        }
    }
    return 0;
}

static int full_gc(lgx_vm_t *vm) {
    // TODO 遍历新生代、老年代，进行一遍标记

    // TODO 遍历新生代，执行清除

    // TODO 遍历老年代，执行清除、压缩
}

// 分配一个变量
lgx_val_t* lgx_gc_alloc(lgx_vm_t *vm) {
    lgx_vm_stack_t *cur = lgx_list_first_entry(&vm->heap.young.head, lgx_vm_stack_t, head);

    if (cur->base >= cur->size) {
        // 当前内存块已满
        lgx_vm_stack_t *next = lgx_list_next_entry(cur, lgx_vm_stack_t, head);
        if (minor_gc(vm, next)) {
            // Out Of Memory
            return NULL;
        }
        lgx_list_move_tail(&cur->head, &vm->heap.young.head);
        cur = next;
    }

    return &cur->buf[cur->base++];
}