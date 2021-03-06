#ifndef LGX_COROUTINE_H
#define LGX_COROUTINE_H

#include "vm.h"

#define LGX_MAX_CO_LIMIT 1024 * 1024

int lgx_co_stack_init(lgx_co_stack_t *stack, unsigned size);

lgx_co_t* lgx_co_create(lgx_vm_t *vm, lgx_function_t *fun);
int lgx_co_delete(lgx_vm_t *vm, lgx_co_t *co);

int lgx_co_schedule(lgx_vm_t *vm);
int lgx_co_yield(lgx_vm_t *vm);
int lgx_co_run(lgx_vm_t *vm, lgx_co_t *co);
int lgx_co_resume(lgx_vm_t *vm, lgx_co_t *co);
int lgx_co_suspend(lgx_vm_t *vm);
int lgx_co_died(lgx_vm_t *vm);

int lgx_co_has_task(lgx_vm_t *vm);
int lgx_co_has_ready_task(lgx_vm_t *vm);

int lgx_co_return(lgx_co_t *co, lgx_value_t *v);
int lgx_co_return_long(lgx_co_t *co, long long v);
int lgx_co_return_double(lgx_co_t *co, double v);
int lgx_co_return_true(lgx_co_t *co);
int lgx_co_return_false(lgx_co_t *co);
int lgx_co_return_void(lgx_co_t *co);
int lgx_co_return_string(lgx_co_t *co, lgx_string_t *str);
int lgx_co_return_array(lgx_co_t *co, lgx_array_t *arr);

int lgx_co_set(lgx_co_t *co, unsigned pos, lgx_value_t *v);
int lgx_co_set_long(lgx_co_t *co, unsigned pos, long long l);
int lgx_co_set_function(lgx_co_t *co, unsigned pos, lgx_function_t *fun);
int lgx_co_set_string(lgx_co_t *co, unsigned pos, lgx_string_t *str);

lgx_value_t *lgx_co_get(lgx_co_t *co, unsigned pos);

int lgx_co_backtrace(lgx_co_t *co);

void lgx_co_throw(lgx_co_t *co, lgx_value_t *e);
void lgx_co_throw_s(lgx_co_t *co, const char *fmt, ...);
void lgx_co_throw_v(lgx_co_t *co, lgx_value_t *v);

int lgx_co_checkstack(lgx_co_t *co, unsigned int stack_size);

#endif // LGX_COROUTINE_H