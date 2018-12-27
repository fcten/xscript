#include "../common/common.h"
#include "../common/bytecode.h"
#include "../common/operator.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "../common/str.h"
#include "../common/res.h"
#include "../common/exception.h"
#include "vm.h"
#include "gc.h"
#include "coroutine.h"

#define R(r)  (vm->regs[r])
#define C(r)  (vm->constant->table[r].v)
#define G(r)  (vm->co_main->stack.buf[r])

void lgx_vm_throw(lgx_vm_t *vm, lgx_val_t *e) {
    assert(vm->co_running);
    lgx_co_throw(vm->co_running, e);
    vm->regs = vm->co_running->stack.buf + vm->co_running->stack.base;
}

void lgx_vm_throw_s(lgx_vm_t *vm, const char *fmt, ...) {
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

    lgx_vm_throw(vm, &e);
}

void lgx_vm_throw_v(lgx_vm_t *vm, lgx_val_t *v) {
    lgx_val_t e;
    e = *v;

    // 把原始变量标记为 undefined，避免 exception 值被释放
    v->type = T_UNDEFINED;

    lgx_vm_throw(vm, &e);
}

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc) {
    wbt_list_init(&vm->co_ready);
    wbt_list_init(&vm->co_suspend);
    wbt_list_init(&vm->co_died);

    wbt_list_init(&vm->heap.young);
    wbt_list_init(&vm->heap.old);
    vm->heap.young_size = 0;
    vm->heap.old_size = 0;

    vm->bc = bc;
    vm->exception = bc->exception;
    vm->constant = bc->constant;

    // 创建事件池
    vm->events = wbt_event_init();
    if (!vm->events) {
        return 1;
    }

    vm->co_id = 0;
    vm->co_count = 0;

    // 创建主协程
    vm->co_running = NULL;

    lgx_fun_t *fun = lgx_fun_new(0);
    fun->addr = 0;
    fun->end = bc->bc_top - 1;
    fun->stack_size = bc->reg->max + 1;

    vm->co_main = lgx_co_create(vm, fun);
    if (!vm->co_main) {
        return 1;
    }

    lgx_co_run(vm, vm->co_main);

    return 0;
}

int lgx_vm_cleanup(lgx_vm_t *vm) {
    // 释放事件池
    if (vm->events) {
        wbt_event_cleanup(vm->events);
        wbt_free(vm->events);
        vm->events = NULL;
    }

    // 释放主协程堆栈
    assert(vm->co_main);
    lgx_co_t *co = vm->co_main;
    unsigned base = co->stack.base;
    assert(co->stack.buf[base].type == T_FUNCTION);
    unsigned size = base + co->stack.buf[base].v.fun->stack_size;
    int n;
    for (n = 0; n < size; n ++) {
        lgx_gc_ref_del(&co->stack.buf[n]);
        co->stack.buf[n].type = T_UNDEFINED;
    }

    // 释放主协程
    vm->co_main = NULL;
    lgx_co_delete(vm, co);

    // TODO 释放消息队列

    lgx_bc_cleanup(vm->bc);

    // 清理存在循环引用的变量
    wbt_list_t *list = vm->heap.young.next;
    while(list != &vm->heap.young) {
        wbt_list_t *next = list->next;
        xfree(list);
        list = next;
    }
    list = vm->heap.old.next;
    while(list != &vm->heap.old) {
        wbt_list_t *next = list->next;
        xfree(list);
        list = next;
    }

    return 0;
}

// 确保堆栈上有足够的剩余空间
int lgx_vm_checkstack(lgx_vm_t *vm, unsigned int stack_size) {
    assert(vm->co_running);

    if (lgx_co_checkstack(vm->co_running, stack_size) == 0) {
        vm->regs = vm->co_running->stack.buf + vm->co_running->stack.base;
        return 0;
    } else {
        return 1;
    }
}

int lgx_vm_execute(lgx_vm_t *vm) {
    if (!vm->co_running) {
        return 0;
    }

    unsigned i, pa, pb, pc;
    unsigned *bc = vm->bc->bc;

    for(;;) {
        i = bc[vm->co_running->pc++];

        // 单步执行
        //lgx_bc_echo(vm->co_running->pc-1, i);
        //getchar();

        pa = PA(i);
        pb = PB(i);
        pc = PC(i);
        // pd 和 pe 较少使用，所以不默认解析

        switch(OP(i)) {
            case OP_MOV:{
                lgx_gc_ref_add(&R(pb));
                lgx_gc_ref_del(&R(pa));

                R(pa).type = R(pb).type;
                R(pa).v = R(pb).v;

                break;
            }
            case OP_MOVI:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_LONG;
                R(pa).v.l = PD(i);
                break;
            }
            case OP_ADD:{
                lgx_gc_ref_del(&R(pa));

                if (R(pb).type == T_LONG && R(pc).type == T_LONG) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l + R(pc).v.l;
                } else if (R(pb).type == T_DOUBLE && R(pc).type == T_DOUBLE) {
                    R(pa).type = T_DOUBLE;
                    R(pa).v.d = R(pb).v.d + R(pc).v.d;
                } else {
                    if (lgx_op_add(&R(pa), &R(pb), &R(pc))) {
                        lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), "+", lgx_val_typeof(&R(pc)));
                    }
                    lgx_gc_ref_add(&R(pa));
                }
                break;
            }
            case OP_SUB:{
                lgx_gc_ref_del(&R(pa));

                if (R(pb).type == T_LONG && R(pc).type == T_LONG) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l - R(pc).v.l;
                } else if (R(pb).type == T_DOUBLE && R(pc).type == T_DOUBLE) {
                    R(pa).type = T_DOUBLE;
                    R(pa).v.d = R(pb).v.d - R(pc).v.d;
                } else {
                    if (lgx_op_sub(&R(pa), &R(pb), &R(pc))) {
                        lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), "-", lgx_val_typeof(&R(pc)));
                    }
                }
                break;
            }
            case OP_MUL:{
                lgx_gc_ref_del(&R(pa));

                if (R(pb).type == T_LONG && R(pc).type == T_LONG) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l * R(pc).v.l;
                } else if (R(pb).type == T_DOUBLE && R(pc).type == T_DOUBLE) {
                    R(pa).type = T_DOUBLE;
                    R(pa).v.d = R(pb).v.d * R(pc).v.d;
                } else {
                    if (lgx_op_mul(&R(pa), &R(pb), &R(pc))) {
                        lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), "*", lgx_val_typeof(&R(pc)));
                    }
                }
                break;
            }
            case OP_DIV:{
                lgx_gc_ref_del(&R(pa));

                if (R(pb).type == T_LONG && R(pc).type == T_LONG) {
                    if (UNEXPECTED(R(pc).v.l == 0)) {
                        lgx_vm_throw_s(vm, "division by zero\n");
                    } else {
                        R(pa).type = T_LONG;
                        R(pa).v.l = R(pb).v.l / R(pc).v.l;
                    }
                } else if (R(pb).type == T_DOUBLE && R(pc).type == T_DOUBLE) {
                    if (UNEXPECTED(R(pc).v.d == 0)) {
                        lgx_vm_throw_s(vm, "division by zero\n");
                    } else {
                        R(pa).type = T_DOUBLE;
                        R(pa).v.d = R(pb).v.d / R(pc).v.d;
                    }
                } else {
                    if (lgx_op_div(&R(pa), &R(pb), &R(pc))) {
                        lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), "/", lgx_val_typeof(&R(pc)));
                    }
                }
                break;
            }
            case OP_ADDI:{
                lgx_gc_ref_del(&R(pa));

                if (R(pb).type == T_LONG) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l + pc;
                } else if (R(pb).type == T_DOUBLE) {
                    R(pa).type = T_DOUBLE;
                    R(pa).v.d = R(pb).v.d + pc;
                } else {
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = pc;
                    if (lgx_op_add(&R(pa), &R(pb), &c)) {
                        lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(pb)));
                    }
                }
                break;
            }
            case OP_SUBI:{
                lgx_gc_ref_del(&R(pa));

                if (R(pb).type == T_LONG) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l - pc;
                } else if (R(pb).type == T_DOUBLE) {
                    R(pa).type = T_DOUBLE;
                    R(pa).v.d = R(pb).v.d - pc;
                } else {
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = pc;
                    if (lgx_op_sub(&R(pa), &R(pb), &c)) {
                        lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(pb)));
                    }
                }
                break;
            }
            case OP_MULI:{
                lgx_gc_ref_del(&R(pa));

                if (R(pb).type == T_LONG) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l * pc;
                } else if (R(pb).type == T_DOUBLE) {
                    R(pa).type = T_DOUBLE;
                    R(pa).v.d = R(pb).v.d * pc;
                } else {
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = pc;
                    if (lgx_op_mul(&R(pa), &R(pb), &c)) {
                        lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(pb)));
                    }
                }
                break;
            }
            case OP_DIVI:{
                lgx_gc_ref_del(&R(pa));

                if (R(pb).type == T_LONG) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l / pc;
                } else if (R(pb).type == T_DOUBLE) {
                    R(pa).type = T_DOUBLE;
                    R(pa).v.d = R(pb).v.d / pc;
                } else {
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = pc;
                    if (lgx_op_div(&R(pa), &R(pb), &c)) {
                        lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(pb)));
                    }
                }
                break;
            }
            case OP_NEG:{
                lgx_gc_ref_del(&R(pa));

                if (R(pb).type == T_LONG) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = -R(pb).v.l;
                } else if (R(pb).type == T_DOUBLE) {
                    R(pa).type = T_LONG;
                    R(pa).v.d = -R(pb).v.d;
                } else {
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_SHL:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l << R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), "<<", lgx_val_typeof(&R(pc)));
                }
                break;
            }
            case OP_SHR:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l >> R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), ">>", lgx_val_typeof(&R(pc)));
                }
                break;
            }
            case OP_SHLI:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l << pc;
                } else {
                    lgx_vm_throw_s(vm, "makes integer from %s without a cast", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_SHRI:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l >> pc;
                } else {
                    lgx_vm_throw_s(vm, "makes integer from %s without a cast", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_AND:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l & R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), "&", lgx_val_typeof(&R(pc)));
                }
                break;
            }
            case OP_OR:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l | R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), "|", lgx_val_typeof(&R(pc)));
                }
                break;
            }
            case OP_XOR:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l ^ R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), "^", lgx_val_typeof(&R(pc)));
                }
                break;
            }
            case OP_NOT:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = ~R(pb).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(pb)), "~", lgx_val_typeof(&R(pc)));
                }
                break;
            }
            case OP_EQ:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;

                if (lgx_val_cmp(&R(pb), &R(pc))) {
                    R(pa).v.l = 1;
                } else {
                    R(pa).v.l = 0;
                }
                break;
            }
            case OP_LE:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;

                if (R(pb).type == T_LONG && R(pc).type == T_LONG) {
                    R(pa).v.l = R(pb).v.l <= R(pc).v.l;
                } else if (R(pb).type == T_DOUBLE && R(pc).type == T_DOUBLE) {
                    R(pa).v.l = R(pb).v.d <= R(pc).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LT:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;

                if (R(pb).type == T_LONG && R(pc).type == T_LONG) {
                    R(pa).v.l = R(pb).v.l < R(pc).v.l;
                } else if (R(pb).type == T_DOUBLE && R(pc).type == T_DOUBLE) {
                    R(pa).v.l = R(pb).v.d < R(pc).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_EQI:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;

                if (R(pb).type == T_LONG) {
                    R(pa).v.l = R(pb).v.l == pc;
                } else {
                    R(pa).v.l = 0;
                }
                break;
            }
            case OP_GEI:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;
                
                if (R(pb).type == T_LONG) {
                    R(pa).v.l = R(pb).v.l >= pc;
                } else if (R(pb).type == T_DOUBLE) {
                    R(pa).v.l = R(pb).v.d >= pc;
                } else {
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_LEI:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;
                
                if (R(pb).type == T_LONG) {
                    R(pa).v.l = R(pb).v.l <= pc;
                } else if (R(pb).type == T_DOUBLE) {
                    R(pa).v.l = R(pb).v.d <= pc;
                } else {
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_GTI:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;
                
                if (R(pb).type == T_LONG) {
                    R(pa).v.l = R(pb).v.l > pc;
                } else if (R(pb).type == T_DOUBLE) {
                    R(pa).v.l = R(pb).v.d > pc;
                } else {
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_LTI:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;
                
                if (R(pb).type == T_LONG) {
                    R(pa).v.l = R(pb).v.l < pc;
                } else if (R(pb).type == T_DOUBLE) {
                    R(pa).v.l = R(pb).v.d < pc;
                } else {
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_LNOT:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;

                if (EXPECTED(R(pb).type == T_BOOL)) {
                    R(pa).v.l = !R(pb).v.l;
                } else {
                    lgx_vm_throw_s(vm, "makes boolean from %s without a cast", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_TEST:{
                if (EXPECTED(R(pa).type == T_BOOL)) {
                    if (!R(pa).v.l) {
                        vm->co_running->pc += PD(i);
                    }
                } else {
                    lgx_vm_throw_s(vm, "makes boolean from %s without a cast", lgx_val_typeof(&R(pa)));
                }
                break;
            }
            case OP_JMP:{                
                if (R(pa).type == T_LONG) {
                    vm->co_running->pc = R(pa).v.l;
                } else {
                    lgx_vm_throw_s(vm, "makes integer from %s without a cast", lgx_val_typeof(&R(pa)));
                }
                break;
            }
            case OP_JMPI:{
                vm->co_running->pc = PE(i);
                break;
            }
            case OP_CALL_NEW:{
                if (EXPECTED(R(pa).type == T_FUNCTION)) {
                    // 确保空余堆栈空间足够容纳本次函数调用
                    if (UNEXPECTED(lgx_vm_checkstack(vm, R(pa).v.fun->stack_size) != 0)) {
                        // runtime error
                        lgx_vm_throw_s(vm, "maximum call stack size exceeded");
                    }
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(pa)));
                }
                break;
            }
            case OP_CALL_SET:{
                lgx_gc_ref_add(&R(pb));
                lgx_gc_ref_del(&R(R(0).v.fun->stack_size + pa));
                R(R(0).v.fun->stack_size + pa) = R(pb);
                break;
            }
            case OP_TAIL_CALL:{
                if (EXPECTED(R(pa).type == T_FUNCTION)) {
                    lgx_fun_t *fun = R(pa).v.fun;
                    unsigned int base = R(0).v.fun->stack_size;

                    lgx_gc_ref_add(&R(pa));
                    lgx_gc_ref_del(&R(0));
                    R(0) = R(pa);

                    // 移动参数
                    int n;
                    for (n = 4; n < 4 + fun->args_num + 1; n ++) {
                        lgx_gc_ref_del(&R(n));
                        R(n) = R(base + n);
                        R(base + n).type = T_UNDEFINED;
                    }

                    // 跳转到函数入口
                    vm->co_running->pc = fun->addr;
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(pa)));
                }
                break;
            }
            case OP_CALL:{
                if (EXPECTED(R(pb).type == T_FUNCTION)) {
                    lgx_fun_t *fun = R(pb).v.fun;
                    unsigned int base = R(0).v.fun->stack_size;

                    // 写入函数信息
                    R(base + 0) = R(pb);
                    lgx_gc_ref_add(&R(base + 0));

                    // 写入返回值地址
                    R(base + 1).type = T_LONG;
                    R(base + 1).v.l = pa;

                    // 写入返回地址
                    R(base + 2).type = T_LONG;
                    R(base + 2).v.l = vm->co_running->pc;

                    // 写入堆栈地址
                    R(base + 3).type = T_LONG;
                    R(base + 3).v.l = vm->co_running->stack.base;

                    if (fun->buildin) {
                        fun->buildin(vm);
                        // 如果触发了协程切换，则返回
                        if (vm->co_running == NULL) {
                            return 0;
                        }
                    } else if (fun->is_async) {
                        lgx_co_t *co = lgx_co_create(vm, fun);
                        if (!co) {
                            lgx_vm_throw_s(vm, "out of memory");
                        } else {
                            // 复制参数到新的 coroutine 中
                            // 由于类方法有隐藏参数 this，所以这里复制的参数数量为 fun->args_num + 1
                            int n;
                            for (n = 4; n < 4 + fun->args_num + 1; n ++) {
                                co->stack.buf[n] = R(base + n);
                                R(base + n).type = T_UNDEFINED;
                            }

                            lgx_val_t ret;
                            ret.type = T_OBJECT;
                            ret.v.obj = lgx_co_obj_new(vm, co);
                            if (!ret.v.obj) {
                                lgx_vm_throw_s(vm, "out of memory");
                            } else {
                                co->on_yield = lgx_co_await;

                                lgx_co_return_object(vm->co_running, ret.v.obj);
                            }
                        }
                    } else {
                        // 切换执行堆栈
                        vm->co_running->stack.base += R(0).v.fun->stack_size;
                        vm->regs = vm->co_running->stack.buf + vm->co_running->stack.base;

                        // 跳转到函数入口
                        vm->co_running->pc = fun->addr;
                    }
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_RET:{
                // 跳转到调用点
                long long pc = R(2).v.l;

                // 判断返回值
                int ret_idx = R(1).v.l;
                int has_ret = pa;
                lgx_val_t ret_val;
                if (has_ret) {
                    ret_val = R(pa);
                    lgx_gc_ref_add(&ret_val);
                }

                // 释放所有局部变量和临时变量
                if (UNEXPECTED(pc < 0 && vm->co_running == vm->co_main)) {
                    // 主协程退出时保留堆栈，以保证全局变量可以访问
                } else {
                    int n;
                    for (n = 0; n < R(0).v.fun->stack_size; n ++) {
                        lgx_gc_ref_del(&R(n));
                        R(n).type = T_UNDEFINED;
                    }
                }

                // 切换执行堆栈
                vm->co_running->stack.base = R(3).v.l;
                vm->regs = vm->co_running->stack.buf + vm->co_running->stack.base;

                // 写入返回值
                lgx_gc_ref_del(&R(ret_idx));
                if (has_ret) {
                    R(ret_idx) = ret_val;
                } else {
                    R(ret_idx).type = T_UNDEFINED;
                }

                if (UNEXPECTED(pc < 0)) {
                    // 如果在顶层作用域 return，则终止运行
                    // 此时，寄存器 1 中保存着返回值
                    lgx_co_died(vm);
                    return 0;
                } else {
                    vm->co_running->pc = pc;
                }

                break;
            }
            case OP_ARRAY_SET:{
                if (EXPECTED(R(pa).type == T_ARRAY)) {
                    if (pb == 0) {
                        lgx_hash_add(R(pa).v.arr, &R(pc));
                    } else if (R(pb).type == T_LONG || R(pb).type == T_STRING) {
                        lgx_hash_node_t n;
                        n.k = R(pb);
                        n.v = R(pc);
                        lgx_hash_set(R(pa).v.arr, &n);
                    } else {
                        // runtime warning
                        lgx_vm_throw_s(vm, "attempt to set a %s key, integer or string expected", lgx_val_typeof(&R(pa)));
                    }
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to set a %s value, array expected", lgx_val_typeof(&R(pa)));
                }
                break;
            }
            case OP_ARRAY_NEW:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_ARRAY;
                R(pa).v.arr = lgx_hash_new(0);
                if (UNEXPECTED(!R(pa).v.arr)) {
                    R(pa).type = T_UNDEFINED;
                    lgx_vm_throw_s(vm, "out of memory");
                    break;
                }
                lgx_gc_ref_add(&R(pa));
                lgx_gc_trace(vm, &R(pa));
                break;
            }
            case OP_ARRAY_GET:{
                if (EXPECTED(R(pb).type == T_ARRAY)) {
                    if (EXPECTED(R(pc).type == T_LONG || R(pc).type == T_STRING)) {
                        lgx_hash_node_t *n = lgx_hash_get(R(pb).v.arr, &R(pc));
                        lgx_val_t v = R(pa);
                        if (n) {
                            R(pa) = n->v;
                        } else {
                            // runtime warning
                            R(pa).type = T_UNDEFINED;
                        }
                        lgx_gc_ref_del(&v);
                        lgx_gc_ref_add(&R(pa));
                    } else {
                        // runtime warning
                        lgx_vm_throw_s(vm, "attempt to index a %s key, integer or string expected", lgx_val_typeof(&R(pc)));
                    }
                } else {
                    lgx_vm_throw_s(vm, "attempt to index a %s value, array expected", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_LOAD:{
                unsigned pd = PD(i);
                // TODO switch 所使用的数组以及常量类型的数组无需执行复制
                if (C(pd).type == T_ARRAY) {
                    lgx_gc_ref_del(&R(pa));
                    R(pa).type = T_ARRAY;
                    R(pa).v.arr = lgx_hash_new(C(pd).v.arr->size);
                    if (UNEXPECTED(!R(pa).v.arr)) {
                        R(pa).type = T_UNDEFINED;
                        lgx_vm_throw_s(vm, "out of memory");
                        break;
                    }
                    lgx_gc_ref_add(&R(pa));
                    lgx_gc_trace(vm, &R(pa));
                    if (UNEXPECTED(lgx_hash_copy(C(pd).v.arr, R(pa).v.arr) != 0)) {
                        lgx_vm_throw_s(vm, "copy hash table failed");
                    }
                } else {
                    lgx_gc_ref_add(&C(pd));
                    lgx_gc_ref_del(&R(pa));
                    R(pa) = C(pd);
                }
                break;
            }
            case OP_GLOBAL_GET:{
                lgx_gc_ref_add(&G(pb));
                lgx_gc_ref_del(&R(pa));
                R(pa) = G(pb);
                break;
            }
            case OP_GLOBAL_SET:{
                lgx_gc_ref_add(&R(pa));
                lgx_gc_ref_del(&G(pb));
                G(pb) = R(pa);
                break;
            }
            case OP_OBJECT_NEW:{
                unsigned pd = PD(i);
                if (EXPECTED(C(pd).type == T_OBJECT)) {
                    lgx_gc_ref_del(&R(pa));
                    R(pa).type = T_OBJECT;
                    R(pa).v.obj = lgx_obj_new(C(pd).v.obj);
                    if (UNEXPECTED(!R(pa).v.obj)) {
                        R(pa).type = T_UNDEFINED;
                        lgx_vm_throw_s(vm, "out of memory");
                        break;
                    }
                    lgx_gc_ref_add(&R(pa));
                    lgx_gc_trace(vm, &R(pa));
                } else {
                    lgx_vm_throw_s(vm, "attempt to new a %s value, object expected", lgx_val_typeof(&C(pd)));
                }
                break;
            }
            case OP_OBJECT_GET:{
                if (EXPECTED(R(pb).type == T_OBJECT)) {
                    if (EXPECTED(R(pc).type == T_STRING)) {
                        lgx_gc_ref_del(&R(pa));
                        //lgx_obj_print(R(pb).v.obj, 0);
                        lgx_val_t *v = lgx_obj_get(R(pb).v.obj, &R(pc));
                        if (v) {
                            R(pa) = *v;
                        } else {
                            lgx_vm_throw_s(vm, "property or method %.*s not exists", R(pc).v.str->length, R(pc).v.str->buffer);
                        }
                        lgx_gc_ref_add(&R(pa));
                    } else {
                        lgx_vm_throw_s(vm, "attempt to index a %s value, string expected", lgx_val_typeof(&R(pc)));
                    }
                } else {
                    lgx_vm_throw_s(vm, "attempt to access a %s value, object expected", lgx_val_typeof(&R(pb)));
                }
                break;
            }
            case OP_OBJECT_SET:{
                if (EXPECTED(R(pa).type == T_OBJECT)) {
                    if (EXPECTED(R(pb).type == T_STRING)) {
                        if (lgx_obj_set(R(pa).v.obj, &R(pb), &R(pc)) != 0) {
                            lgx_vm_throw_s(vm, "property %.*s not exists", R(pb).v.str->length, R(pb).v.str->buffer);
                        }
                    } else {
                        lgx_vm_throw_s(vm, "attempt to index a %s value, string expected", lgx_val_typeof(&R(pb)));
                    }
                } else {
                    lgx_vm_throw_s(vm, "attempt to access a %s value, object expected", lgx_val_typeof(&R(pa)));
                }
                break;
            }
            case OP_THROW: {
                lgx_vm_throw_v(vm, &R(pa));
                break;
            }
            case OP_AWAIT: {
                // TODO is_instanceof 判断
                // static lgx_obj_t *coroutine = NULL;

                if (EXPECTED(R(pb).type == T_OBJECT)) {
                    // 参数为 Coroutine 对象
                    lgx_str_t s;
                    lgx_str_set(s, "res");
                    lgx_val_t k, *v;
                    k.type = T_STRING;
                    k.v.str = &s;
                    v = lgx_obj_get(R(pb).v.obj, &k);
                    if (!v || v->type != T_RESOURCE || v->v.res->type != LGX_CO_RES_TYPE) {
                        lgx_vm_throw_s(vm, "invalid `Coroutine` object");
                    } else {
                        lgx_co_t *co = (lgx_co_t *)v->v.res->buf;

                        if (co->status != CO_DIED) {
                            vm->co_running->child = co;
                            lgx_co_suspend(vm);
                            return 0;
                        } else {
                            // 该 Coroutine 已经退出
                            lgx_gc_ref_del(&R(pa));
                            R(pa) = co->stack.buf[1];
                            co->stack.buf[1].type = T_UNDEFINED;
                        }
                    }
                } else {
                    if (R(pb).type == T_OBJECT) {
                        lgx_vm_throw_s(vm, "attempt to await a %s<%.*s> value, object<Coroutine> expected",
                            lgx_val_typeof(&R(pb)),
                            R(pb).v.obj->name->length,
                            R(pb).v.obj->name->buffer
                        );
                    } else {
                        lgx_vm_throw_s(vm, "attempt to await a %s value, object<Coroutine> expected", lgx_val_typeof(&R(pb)));
                    }
                }
                break;
            }
            case OP_NOP: break;
            case OP_HLT: {
                // 释放所有局部变量和临时变量
                int n;
                for (n = 0; n < R(0).v.fun->stack_size; n ++) {
                    lgx_gc_ref_del(&R(n));
                    R(n).type = T_UNDEFINED;
                }

                // 写入返回值
                R(1).type = T_UNDEFINED;

                return 0;
            }
            case OP_ECHO:{
                lgx_val_print(&R(pa), 0);
                printf("\n");
                break;
            }
            case OP_TYPEOF:{
                lgx_gc_ref_del(&R(pa));
                lgx_op_typeof(&R(pa), &R(pb));
                break;
            }
            default:
                lgx_vm_throw_s(vm, "unknown op %d @ %d", OP(i), vm->co_running->pc - 1);
        }
    }

    return 0;
}

extern wbt_atomic_t wbt_wating_to_exit;

int lgx_vm_start(lgx_vm_t *vm) {
    assert(vm->co_running);

    time_t timeout = 0;

    while (!wbt_wating_to_exit) {
        lgx_vm_execute(vm);

        if (!lgx_co_has_task(vm)) {
            // 全部协程均执行完毕
            break;
        }

        if (vm->co_running) {
            continue;
        } else if (lgx_co_has_ready_task(vm)) {
            lgx_co_schedule(vm);
        } else {
            timeout = wbt_timer_process(&vm->events->timer);
            wbt_event_wait(vm->events, timeout);
            timeout = wbt_timer_process(&vm->events->timer);
        }
    }

    return 0;
}