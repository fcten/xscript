#include <assert.h>
#include "../common/common.h"
#include "../common/str.h"
#include "../common/obj.h"
#include "../common/res.h"
#include "coroutine.h"
#include "gc.h"

int lgx_co_stack_init(lgx_co_stack_t *stack, unsigned size) {
    stack->size = size;
    stack->base = 0;
    stack->buf = (lgx_val_t *)xcalloc(stack->size, sizeof(lgx_val_t));
    if (!stack->buf) {
        return 1;
    }
    return 0;
}

int lgx_co_stack_cleanup(lgx_co_stack_t *stack) {
    if (stack->buf) {
        xfree(stack->buf);
        stack->buf = NULL;
    }

    return 0;
}

int lgx_co_set(lgx_co_t *co, unsigned pos, lgx_val_t *v) {
    assert(pos < co->stack.size);

    co->stack.buf[pos].type = v->type;
    co->stack.buf[pos].v = v->v;
    lgx_gc_ref_add(&co->stack.buf[pos]);

    return 0;
}

int lgx_co_set_long(lgx_co_t *co, unsigned pos, long long l) {
    lgx_val_t v;
    v.type = T_LONG;
    v.v.l = l;
    return lgx_co_set(co, pos, &v);
}

int lgx_co_set_function(lgx_co_t *co, unsigned pos, lgx_fun_t *fun) {
    lgx_val_t v;
    v.type = T_FUNCTION;
    v.v.fun = fun;
    return lgx_co_set(co, pos, &v);
}

int lgx_co_set_string(lgx_co_t *co, unsigned pos, lgx_str_t *str) {
    lgx_val_t v;
    v.type = T_STRING;
    v.v.str = str;
    return lgx_co_set(co, pos, &v);
}

int lgx_co_set_object(lgx_co_t *co, unsigned pos, lgx_obj_t *obj) {
    lgx_val_t v;
    v.type = T_OBJECT;
    v.v.obj = obj;
    return lgx_co_set(co, pos, &v);
}

lgx_co_t* lgx_co_create(lgx_vm_t *vm, lgx_fun_t *fun) {
    if (vm->co_count > LGX_MAX_CO_LIMIT) {
        return NULL;
    }

    if (fun->buildin) {
        // 只支持为 xscript 函数创建协程
        return NULL;
    }

    lgx_co_t *co = (lgx_co_t *)xcalloc(1, sizeof(lgx_co_t));
    if (!co) {
        return NULL;
    }

    if (lgx_co_stack_init(&co->stack, fun->stack_size > 16 ? fun->stack_size : 16)) {
        xfree(co);
        return NULL;
    }

    if (vm->co_running) {
        vm->co_running->ref_cnt ++;
        co->parent = vm->co_running;
    }
    co->pc = fun->addr;
    co->on_yield = NULL;

    co->vm = vm;

    co->status = CO_READY;
    wbt_list_add_tail(&co->head, &vm->co_ready);

    // 初始化函数
    lgx_co_set_function(co, 0, fun);
    // 写入返回值地址
    lgx_co_set_long(co, 1, 1);
    // 写入返回地址
    lgx_co_set_long(co, 2, -1);
    // 写入堆栈地址
    lgx_co_set_long(co, 3, 0);

    co->id = ++vm->co_id;
    vm->co_count ++;

    return co;
}

int lgx_co_delete(lgx_vm_t *vm, lgx_co_t *co) {
    if (!wbt_list_empty(&co->head)) {
        wbt_list_del(&co->head);
    }

    lgx_co_stack_cleanup(&co->stack);
    xfree(co);

    vm->co_count --;

    return 0;
}

int lgx_co_set_on_yield(lgx_co_t *co, int (*on_yield)(struct lgx_vm_s *vm)) {
    return 0;
}

int lgx_co_set_on_resume(lgx_co_t *co, int (*on_yield)(struct lgx_vm_s *vm)) {
    return 0;
}

int lgx_co_has_ready_task(lgx_vm_t *vm) {
    if (!wbt_list_empty(&vm->co_ready)) {
        return 1;
    }

    return 0;
}

int lgx_co_has_task(lgx_vm_t *vm) {
    if (vm->co_running) {
        return 1;
    }

    if (!wbt_list_empty(&vm->co_ready)) {
        return 1;
    }

    if (!wbt_list_empty(&vm->co_suspend)) {
        return 1;
    }

    return 0;
}

int lgx_co_schedule(lgx_vm_t *vm) {
    assert(vm->co_running == NULL);
    assert(lgx_co_has_ready_task(vm));

    lgx_co_t *co = wbt_list_first_entry(&vm->co_ready, lgx_co_t, head);

    return lgx_co_run(vm, co);
}

int lgx_co_run(lgx_vm_t *vm, lgx_co_t *co) {
    assert(vm->co_running == NULL);
    assert(co->status == CO_READY);

    if (!wbt_list_empty(&co->head)) {
        wbt_list_del(&co->head);
    }
    wbt_list_init(&co->head);

    co->child = NULL;
    co->status = CO_RUNNING;
    vm->co_running = co;

    vm->regs = co->stack.buf + co->stack.base;

    return 0;
}

int lgx_co_resume(lgx_vm_t *vm, lgx_co_t *co) {
    assert(co->status == CO_SUSPEND);

    if (!wbt_list_empty(&co->head)) {
        wbt_list_del(&co->head);
    }
    wbt_list_init(&co->head);

    co->child = NULL;
    co->status = CO_READY;

    wbt_list_add_tail(&co->head, &vm->co_ready);

    if (vm->co_running) {
        lgx_co_yield(vm);
    }

    return lgx_co_run(vm, co);
}

int lgx_co_yield(lgx_vm_t *vm) {
    assert(vm->co_running);

    lgx_co_t *co = vm->co_running;
    co->status = CO_READY;

    if (co->on_yield) {
        co->on_yield(vm);
    }

    if (vm->co_running == co) {
        vm->co_running = NULL;
    }
    wbt_list_add_tail(&co->head, &vm->co_ready);

    return 0;
}

int lgx_co_suspend(lgx_vm_t *vm) {
    assert(vm->co_running);

    lgx_co_t *co = vm->co_running;
    co->status = CO_SUSPEND;

    if (co->on_yield) {
        co->on_yield(vm);
    }

    if (vm->co_running == co) {
        vm->co_running = NULL;
    }
    wbt_list_add_tail(&co->head, &vm->co_suspend);

    return 0;
}

int lgx_co_died(lgx_vm_t *vm) {
    assert(vm->co_running);

    lgx_co_t *co = vm->co_running;
    co->status = CO_DIED;

    if (co->on_yield) {
        co->on_yield(vm);
    }

    if (vm->co_running == co) {
        vm->co_running = NULL;
    }

    if (co->parent) {
        co->parent->ref_cnt --;
        if (co->parent->ref_cnt == 0 && co->parent->status == CO_DIED) {
            lgx_co_delete(vm, co->parent);
        }
    }

    if (co->ref_cnt) {
        // 暂时不能删除
        wbt_list_move_tail(&co->head, &vm->co_died);
    } else {
        lgx_co_delete(vm, co);
    }

    return 0;
}

int lgx_co_return(lgx_co_t *co, lgx_val_t *v) {
    // 参数起始地址
    int base = co->stack.base + co->stack.buf[co->stack.base].v.fun->stack_size;

    // 返回值地址
    int ret = co->stack.buf[base + 1].v.l;

    // 写入返回值
    lgx_gc_ref_del(&co->stack.buf[ret]);
    co->stack.buf[ret] = *v;
    lgx_gc_ref_add(&co->stack.buf[ret]);

    // 释放函数栈
    int n;
    for (n = 4; n < co->stack.buf[base].v.fun->stack_size; n ++) {
        lgx_gc_ref_del(&co->stack.buf[base + n]);
        co->stack.buf[base + n].type = T_UNDEFINED;
    }

    return 0;
}

int lgx_co_return_long(lgx_co_t *co, long long v) {
    lgx_val_t ret;
    ret.type = T_LONG;
    ret.v.l = v;

    return lgx_co_return(co, &ret);
}

int lgx_co_return_double(lgx_co_t *co, double v) {
    lgx_val_t ret;
    ret.type = T_DOUBLE;
    ret.v.d = v;

    return lgx_co_return(co, &ret);
}

int lgx_co_return_true(lgx_co_t *co) {
    lgx_val_t ret;
    ret.type = T_BOOL;
    ret.v.d = 1;

    return lgx_co_return(co, &ret);
}

int lgx_co_return_false(lgx_co_t *co) {
    lgx_val_t ret;
    ret.type = T_BOOL;
    ret.v.d = 0;

    return lgx_co_return(co, &ret);
}

int lgx_co_return_undefined(lgx_co_t *co) {
    lgx_val_t ret;
    ret.type = T_UNDEFINED;

    return lgx_co_return(co, &ret);
}

int lgx_co_return_string(lgx_co_t *co, lgx_str_t *str) {
    lgx_val_t ret;
    ret.type = T_STRING;
    ret.v.str = str;

    return lgx_co_return(co, &ret);
}

int lgx_co_return_object(lgx_co_t *co, lgx_obj_t *obj) {
    lgx_val_t ret;
    ret.type = T_OBJECT;
    ret.v.obj = obj;

    return lgx_co_return(co, &ret);
}

int lgx_co_return_array(lgx_co_t *co, lgx_hash_t *hash) {
    lgx_val_t ret;
    ret.type = T_ARRAY;
    ret.v.arr = hash;

    return lgx_co_return(co, &ret);
}

void lgx_co_obj_on_delete(lgx_res_t *res) {
    lgx_co_t *co = res->buf;

    co->ref_cnt --;

    res->buf = NULL;
}

lgx_obj_t* lgx_co_obj_new(lgx_vm_t *vm, lgx_co_t *co) {
    lgx_str_t name, *property_res;
    lgx_str_set(name, "Coroutine");

    // TODO MEM LEAK
    property_res = lgx_str_new_ref("res", sizeof("res")-1);
    if (!property_res) {
        return NULL;
    }

    lgx_obj_t *obj = lgx_obj_create(&name);
    if (!obj) {
        return NULL;
    }

    lgx_res_t *res = lgx_res_new(LGX_CO_RES_TYPE, co);
    if (!res) {
        lgx_obj_delete(obj);
        return NULL;
    }

    res->on_delete = lgx_co_obj_on_delete;

    lgx_hash_node_t node;
    node.k.type = T_STRING;
    node.k.v.str = property_res;
    node.v.type = T_RESOURCE;
    node.v.v.res = res;
    if (lgx_obj_add_property(obj, &node) != 0) {
        lgx_res_delete(res);
        lgx_obj_delete(obj);
        return NULL;
    }

    co->ref_cnt ++;

    return obj;
}

int lgx_co_await(lgx_vm_t *vm) {
    lgx_co_t *co = vm->co_running;
    lgx_obj_t *obj = (lgx_obj_t *)co->ctx;

    if (co->status != CO_DIED) {
        return 0;
    }

    // TODO 写入返回值

    // 恢复父协程执行
    if (co->parent && co->parent->status == CO_SUSPEND && co->parent->child == co) {
        lgx_co_resume(vm, co->parent);
    }

    co->on_yield = NULL;
    co->ctx = NULL;

    lgx_val_t ret;
    ret.type = T_OBJECT;
    ret.v.obj = obj;
    lgx_gc_ref_del(&ret);

    return 0;
}