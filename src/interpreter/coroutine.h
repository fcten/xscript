#ifndef LGX_COROUTINE_H
#define LGX_COROUTINE_H

#include "vm.h"

#define LGX_MAX_CO_LIMIT 1024 * 1024

int lgx_co_stack_init(lgx_co_stack_t *stack, unsigned size);

lgx_co_t* lgx_co_create(lgx_vm_t *vm, lgx_fun_t *fun);
int lgx_co_delete(lgx_vm_t *vm, lgx_co_t *co);

int lgx_co_schedule(lgx_vm_t *vm);
int lgx_co_yield(lgx_vm_t *vm);
int lgx_co_resume(lgx_vm_t *vm, lgx_co_t *co);
int lgx_co_suspend(lgx_vm_t *vm);
int lgx_co_died(lgx_vm_t *vm);

int lgx_co_has_task(lgx_vm_t *vm);
int lgx_co_has_ready_task(lgx_vm_t *vm);

#endif // LGX_COROUTINE_H