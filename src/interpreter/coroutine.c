#include <assert.h>
#include "../common/common.h"
#include "../common/str.h"
#include "../common/obj.h"
#include "../common/res.h"
#include "../common/exception.h"
#include "coroutine.h"
#include "gc.h"

#define LGX_MAX_STACK_SIZE (256 << 8)

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

    // 主协程不能删除，必须和虚拟机一起销毁
    if (co == vm->co_main) {
        return 0;
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
        return 0;
    } else {
        return lgx_co_run(vm, co);
    }
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
    assert(co->stack.buf[co->stack.base].type == T_FUNCTION);
    // 参数起始地址
    int base = co->stack.base + co->stack.buf[co->stack.base].v.fun->stack_size;

    assert(co->stack.buf[base].type == T_FUNCTION);
    // 返回值地址
    int ret = co->stack.buf[base + 1].v.l;

    // 写入返回值
    lgx_gc_ref_del(&co->stack.buf[ret]);
    co->stack.buf[ret] = *v;
    lgx_gc_ref_add(&co->stack.buf[ret]);

    // 释放函数栈
    int n;
    int size = co->stack.buf[base].v.fun->stack_size;
    for (n = 0; n < size; n ++) {
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
    if (co->ref_cnt == 0 && co->status == CO_DIED) {
        lgx_co_delete(co->vm, co);
    }

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

    if (co->status != CO_DIED) {
        return 0;
    }

    // 恢复父协程执行
    if (co->parent && co->parent->status == CO_SUSPEND && co->parent->child == co) {
        // 重新执行 await 指令以写入返回值
        co->parent->pc --;
        lgx_co_resume(vm, co->parent);
    }

    co->on_yield = NULL;

    return 0;
}

// 输出协程的当前调用栈
int lgx_co_backtrace(lgx_co_t *co) {
    unsigned int base = co->stack.base;
    while (1) {
        printf("  <function:%d> %d\n", co->stack.buf[base].v.fun->addr, base);

        if (base != 0) {
            base = co->stack.buf[base+3].v.l;
        } else {
            break;
        }
    }

    return 0;
}

void lgx_co_throw(lgx_co_t *co, lgx_val_t *e) {
    unsigned pc = co->pc - 1;

    // 匹配最接近的 try-catch 块
    unsigned long long int key = ((unsigned long long int)pc + 1) << 32;

    wbt_str_t k;
    k.str = (char *)&key;
    k.len = sizeof(key);

    lgx_exception_t *exception;
    lgx_exception_block_t *block = NULL;
    wbt_rb_node_t *n = wbt_rb_get_lesser(co->vm->exception, &k);
    if (n) {
        exception = (lgx_exception_t *)n->value.str;
        if (pc >= exception->try_block.start && pc <= exception->try_block.end) {
            // 匹配参数类型符合的 catch block
            lgx_exception_block_t *b;
            wbt_list_for_each_entry(b, lgx_exception_block_t, &exception->catch_blocks, head) {
                if (b->e->type == T_UNDEFINED) {
                    block = b;
                    break;
                } else if (b->e->type == e->type) {
                    if (b->e->type == T_OBJECT) {
                        if (lgx_obj_is_instanceof(e->v.obj, b->e->v.obj)) {
                            block = b;
                            break;
                        }
                    } else {
                        block = b;
                        break;
                    }
                }
            }
        }
    }

    if (block) {
        // 跳转到指定的 catch block
        co->pc = block->start;

        // 把异常变量写入到 catch block 的参数中
        //printf("%d\n",block->e->u.c.reg);
        lgx_gc_ref_del(&co->stack.buf[co->stack.base + block->e->u.c.reg]);
        co->stack.buf[co->stack.base + block->e->u.c.reg] = *e;
    } else {
        // 没有匹配的 catch 块，递归向上寻找
        unsigned base = co->stack.base;
        lgx_val_t *regs = co->stack.buf + base;

        assert(regs[0].type == T_FUNCTION);
        assert(regs[2].type == T_LONG);
        assert(regs[3].type == T_LONG);

        if (regs[2].v.l >= 0) {
            // 释放所有局部变量和临时变量
            int n;
            for (n = 0; n < regs[0].v.fun->stack_size; n ++) {
                lgx_gc_ref_del(&regs[n]);
                regs[n].type = T_UNDEFINED;
            }

            // 切换执行堆栈
            co->stack.base = regs[3].v.l;

            // 在函数调用点重新抛出异常
            co->pc = regs[2].v.l;
            lgx_co_throw(co, e);
        } else {
            // 遍历调用栈依然未能找到匹配的 catch 块，退出当前协程
            printf("[uncaught exception] [%llu] ", co->id);
            lgx_val_print(e);
            printf("\n");

            lgx_gc_ref_del(e);
            // TODO 写入到协程返回值中？

            lgx_co_backtrace(co);

            co->pc = regs[0].v.fun->end;
        }
    }
}

void lgx_co_throw_s(lgx_co_t *co, const char *fmt, ...) {
    char *buf = (char *)xmalloc(128);

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, 128, fmt, args);
    va_end(args);

    lgx_val_t e;
    e.type = T_STRING;
    e.v.str = lgx_str_new_ref(buf, 128);
    e.v.str->length = len;
    e.v.str->is_ref = 0;

    lgx_gc_ref_add(&e);

    lgx_co_throw(co, &e);
}

void lgx_co_throw_v(lgx_co_t *co, lgx_val_t *v) {
    lgx_val_t e;
    e = *v;

    // 把原始变量标记为 undefined，避免 exception 值被释放
    v->type = T_UNDEFINED;

    lgx_co_throw(co, &e);
}

// 确保堆栈上有足够的剩余空间
int lgx_co_checkstack(lgx_co_t *co, unsigned int stack_size) {
    lgx_co_stack_t *stack = &co->stack;
    lgx_val_t *regs = &stack->buf[stack->base];

    assert(regs[0].type == T_FUNCTION);

    if (stack->base + regs[0].v.fun->stack_size + stack_size < stack->size) {
        return 0;
    }

    unsigned int size = stack->size;
    while (stack->base + regs[0].v.fun->stack_size + stack_size >= size) {
        size *= 2;
    }

    if (size > LGX_MAX_STACK_SIZE) {
        return 1;
    }

    lgx_val_t *s = (lgx_val_t *)xrealloc(stack->buf, size * sizeof(lgx_val_t));
    if (!s) {
        return 1;
    }
    // 初始化新空间
    memset(s + stack->size, 0, (size - stack->size) * sizeof(lgx_val_t));

    stack->buf = s;
    stack->size = size;

    return 0;
}