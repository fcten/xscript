#include <stdio.h>
#include <stdlib.h>

#include "gc.h"

void lgx_gc_enable(lgx_vm_t *vm) {
    vm->gc_enable = 1;
}

void lgx_gc_disable(lgx_vm_t *vm) {
    vm->gc_enable = 0;
}

void lgx_gc_free(lgx_val_t *val) {
    int i;
    switch (val->type) {
        case T_STRING:
            if (val->v.str->gc.ref_cnt) {
                return;
            }
            free(val->v.str);
            val->type = T_UNDEFINED;
            break;
        case T_ARRAY:
            if (val->v.arr->gc.ref_cnt) {
                return;
            }
            // 递归释放子节点
            for(i = 0; i < val->v.arr->table_offset; i++) {
                lgx_gc_free(&val->v.arr->table[i].k);
                lgx_gc_free(&val->v.arr->table[i].v);
            }
            lgx_hash_cleanup(val->v.arr);
            free(val->v.arr);
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

static int minor_gc(lgx_vm_t *vm, lgx_vm_stack_t *stack) {
    int i, cnt = 0;
    // 第一次遍历，清理垃圾
    for (i = 0; i < stack->base; i++) {
        lgx_gc_free(&stack->buf[i]);
    }
    // 第二次遍历，把没有被释放的变量移动到老年代
    for (i = 0; i < stack->base; i++) {
        lgx_val_t *v = &stack->buf[i];

        if (v->type == T_UNDEFINED) {
            continue;
        }

        if (vm->heap.old.base >= vm->heap.old.size) {
            // 老年代已满
            if (full_gc(vm)) {
                return 1;
            }
        }
        // 把变量移动到老年代
        vm->heap.old.buf[vm->heap.old.base++] = *v;
        v->type = T_UNDEFINED;
        cnt ++;
    }

    stack->base = 0;

    // TODO
    //printf("[minor gc] %d move to old\n", cnt);

    return 0;
}

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
