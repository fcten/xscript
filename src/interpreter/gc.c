#include "gc.h"

void lgx_gc_enable(lgx_vm_t *vm) {
    vm->gc_enable = 1;
}

void lgx_gc_disable(lgx_vm_t *vm) {
    vm->gc_enable = 0;
}