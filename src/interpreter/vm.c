#include "../common/common.h"
#include "../common/ht.h"
#include "../compiler/bytecode.h"
#include "vm.h"
#include "value.h"
#include "gc.h"
#include "coroutine.h"

#define R(r)  (vm->regs[r])
#define C(r)  (vm->constant->table[r].v)
#define G(r)  (vm->co_main->stack.buf[r])

void lgx_vm_throw(lgx_vm_t *vm, lgx_value_t *e) {
    assert(vm->co_running);
    lgx_co_throw(vm->co_running, e);
    vm->regs = vm->co_running->stack.buf + vm->co_running->stack.base;
}

void lgx_vm_throw_s(lgx_vm_t *vm, const char *fmt, ...) {
    char *buf = (char *)xmalloc(128);
    if (!buf) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, 128, fmt, args);
    va_end(args);

    lgx_value_t e;
    e.type = T_STRING;
    e.v.str = xcalloc(1, sizeof(lgx_string_t));
    e.v.str->string.length = len;
    e.v.str->string.size = 128;
    e.v.str->string.buffer = buf;

    lgx_gc_ref_add(&e);

    lgx_vm_throw(vm, &e);
}

void lgx_vm_throw_v(lgx_vm_t *vm, lgx_value_t *v) {
    lgx_value_t e;
    e = *v;

    // 把原始变量标记为 unknown，避免 exception 值被释放
    v->type = T_UNKNOWN;

    lgx_vm_throw(vm, &e);
}

int lgx_vm_init(lgx_vm_t *vm, lgx_compiler_t *c) {
    lgx_list_init(&vm->co_ready);
    lgx_list_init(&vm->co_suspend);
    lgx_list_init(&vm->co_died);

    lgx_list_init(&vm->heap.young);
    lgx_list_init(&vm->heap.old);
    vm->heap.young_size = 0;
    vm->heap.old_size = 0;

    vm->c = c;
    vm->exception = &c->exception;
    vm->constant = &c->constant;

    vm->co_id = 0;
    vm->co_count = 0;

    vm->co_running = NULL;

    return 0;
}


int lgx_vm_call(lgx_vm_t *vm, lgx_function_t *fun) {
    vm->co_main = lgx_co_create(vm, fun);
    if (!vm->co_main) {
        return 1;
    }

    lgx_co_run(vm, vm->co_main);

    return lgx_vm_start(vm);
}

int lgx_vm_cleanup(lgx_vm_t *vm) {
    // 释放主协程堆栈
    assert(vm->co_main);
    lgx_co_t *co = vm->co_main;
    unsigned base = co->stack.base;
    assert(co->stack.buf[base].type == T_FUNCTION);
    unsigned size = base + co->stack.buf[base].v.fun->stack_size;
    int n;
    for (n = 0; n < size; n ++) {
        lgx_gc_ref_del(&co->stack.buf[n]);
        co->stack.buf[n].type = T_UNKNOWN;
    }

    // 释放主协程
    vm->co_main = NULL;
    lgx_co_delete(vm, co);

    // TODO 释放消息队列

    // 清理存在循环引用的变量
    lgx_list_t *list = vm->heap.young.next;
    while(list != &vm->heap.young) {
        lgx_list_t *next = list->next;
        xfree(list);
        list = next;
    }
    list = vm->heap.old.next;
    while(list != &vm->heap.old) {
        lgx_list_t *next = list->next;
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
    unsigned *bc = vm->c->bc.buffer;

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
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), "+", lgx_value_typeof(&R(pc)));
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
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), "-", lgx_value_typeof(&R(pc)));
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
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), "*", lgx_value_typeof(&R(pc)));
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
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), "/", lgx_value_typeof(&R(pc)));
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_value_typeof(&R(pb)));
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_value_typeof(&R(pb)));
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_value_typeof(&R(pb)));
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_value_typeof(&R(pb)));
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_value_typeof(&R(pb)));
                }
                break;
            }
            case OP_SHL:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l << R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), "<<", lgx_value_typeof(&R(pc)));
                }
                break;
            }
            case OP_SHR:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l >> R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), ">>", lgx_value_typeof(&R(pc)));
                }
                break;
            }
            case OP_SHLI:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l << pc;
                } else {
                    lgx_vm_throw_s(vm, "makes integer from %s without a cast", lgx_value_typeof(&R(pb)));
                }
                break;
            }
            case OP_SHRI:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l >> pc;
                } else {
                    lgx_vm_throw_s(vm, "makes integer from %s without a cast", lgx_value_typeof(&R(pb)));
                }
                break;
            }
            case OP_AND:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l & R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), "&", lgx_value_typeof(&R(pc)));
                }
                break;
            }
            case OP_OR:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l | R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), "|", lgx_value_typeof(&R(pc)));
                }
                break;
            }
            case OP_XOR:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG && R(pc).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = R(pb).v.l ^ R(pc).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), "^", lgx_value_typeof(&R(pc)));
                }
                break;
            }
            case OP_NOT:{
                lgx_gc_ref_del(&R(pa));

                if (EXPECTED(R(pb).type == T_LONG)) {
                    R(pa).type = T_LONG;
                    R(pa).v.l = ~R(pb).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_value_typeof(&R(pb)), "~", lgx_value_typeof(&R(pc)));
                }
                break;
            }
            case OP_EQ:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;

                if (lgx_value_cmp(&R(pb), &R(pc))) {
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_value_typeof(&R(pb)));
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_value_typeof(&R(pb)));
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_value_typeof(&R(pb)));
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_value_typeof(&R(pb)));
                }
                break;
            }
            case OP_LNOT:{
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_BOOL;

                if (EXPECTED(R(pb).type == T_BOOL)) {
                    R(pa).v.l = !R(pb).v.l;
                } else {
                    lgx_vm_throw_s(vm, "makes boolean from %s without a cast", lgx_value_typeof(&R(pb)));
                }
                break;
            }
            case OP_TEST:{
                if (EXPECTED(R(pa).type == T_BOOL)) {
                    if (!R(pa).v.l) {
                        vm->co_running->pc += PD(i);
                    }
                } else {
                    lgx_vm_throw_s(vm, "makes boolean from %s without a cast", lgx_value_typeof(&R(pa)));
                }
                break;
            }
            case OP_JMP:{                
                if (R(pa).type == T_LONG) {
                    vm->co_running->pc = R(pa).v.l;
                } else {
                    lgx_vm_throw_s(vm, "makes integer from %s without a cast", lgx_value_typeof(&R(pa)));
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
                    lgx_vm_throw_s(vm, "attempt to call a %s value, function expected", lgx_value_typeof(&R(pa)));
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
                    lgx_function_t *fun = R(pa).v.fun;
                    unsigned int base = R(0).v.fun->stack_size;

                    lgx_gc_ref_add(&R(pa));
                    lgx_gc_ref_del(&R(0));
                    R(0) = R(pa);

                    // 移动参数
                    int n;
                    for (n = 4; n < 4 + fun->type->arg_len + 1; n ++) {
                        lgx_gc_ref_del(&R(n));
                        R(n) = R(base + n);
                        R(base + n).type = T_UNKNOWN;
                    }

                    // 跳转到函数入口
                    vm->co_running->pc = fun->addr;
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to call a %s value, function expected", lgx_value_typeof(&R(pa)));
                }
                break;
            }
            case OP_CALL:{
                if (EXPECTED(R(pb).type == T_FUNCTION)) {
                    lgx_function_t *fun = R(pb).v.fun;
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
                    }/* else if (fun->is_async) {
                        lgx_co_t *co = lgx_co_create(vm, fun);
                        if (!co) {
                            lgx_vm_throw_s(vm, "out of memory");
                        } else {
                            // 复制参数到新的 coroutine 中
                            // 由于类方法有隐藏参数 this，所以这里复制的参数数量为 fun->args_num + 1
                            int n;
                            for (n = 4; n < 4 + fun->args_num + 1; n ++) {
                                co->stack.buf[n] = R(base + n);
                                R(base + n).type = T_UNKNOWN;
                            }

                            lgx_value_t ret;
                            ret.type = T_OBJECT;
                            ret.v.obj = lgx_co_obj_new(vm, co);
                            if (!ret.v.obj) {
                                lgx_vm_throw_s(vm, "out of memory");
                            } else {
                                co->on_yield = lgx_co_await;

                                lgx_co_return_object(vm->co_running, ret.v.obj);
                            }
                        }
                    }*/ else {
                        // 切换执行堆栈
                        vm->co_running->stack.base += R(0).v.fun->stack_size;
                        vm->regs = vm->co_running->stack.buf + vm->co_running->stack.base;

                        // 跳转到函数入口
                        vm->co_running->pc = fun->addr;
                    }
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to call a %s value, function expected", lgx_value_typeof(&R(pb)));
                }
                break;
            }
            case OP_RET:{
                // 跳转到调用点
                long long pc = R(2).v.l;

                // 判断返回值
                int ret_idx = R(1).v.l;
                int has_ret = pa;
                lgx_value_t ret_val;
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
                        R(n).type = T_UNKNOWN;
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
                    R(ret_idx).type = T_UNKNOWN;
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
                /*
                if (EXPECTED(R(pa).type == T_ARRAY)) {
                    if (pb == 0) {
                        lgx_ht_add(R(pa).v.arr, &R(pc));
                    } else if (R(pb).type == T_LONG || R(pb).type == T_STRING) {
                        lgx_ht_node_t n;
                        n.k = R(pb);
                        n.v = R(pc);
                        lgx_ht_set(R(pa).v.arr, &n);
                    } else {
                        // runtime warning
                        lgx_vm_throw_s(vm, "attempt to set a %s key, integer or string expected", lgx_value_typeof(&R(pa)));
                    }
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to set a %s value, array expected", lgx_value_typeof(&R(pa)));
                }
                */
                break;
            }
            case OP_ARRAY_NEW:{
                /*
                lgx_gc_ref_del(&R(pa));

                R(pa).type = T_ARRAY;
                R(pa).v.arr = lgx_hash_new(0);
                if (UNEXPECTED(!R(pa).v.arr)) {
                    R(pa).type = T_UNKNOWN;
                    lgx_vm_throw_s(vm, "out of memory");
                    break;
                }
                lgx_gc_ref_add(&R(pa));
                lgx_gc_trace(vm, &R(pa));
                */
                break;
            }
            case OP_ARRAY_GET:{
                /*
                if (EXPECTED(R(pb).type == T_ARRAY)) {
                    if (EXPECTED(R(pc).type == T_LONG || R(pc).type == T_STRING)) {
                        lgx_ht_node_t *n = lgx_hash_get(R(pb).v.arr, &R(pc));
                        lgx_value_t v = R(pa);
                        if (n) {
                            R(pa) = *(lgx_value_t*)n->v;
                        } else {
                            // runtime warning
                            R(pa).type = T_UNKNOWN;
                        }
                        lgx_gc_ref_del(&v);
                        lgx_gc_ref_add(&R(pa));
                    } else {
                        // runtime warning
                        lgx_vm_throw_s(vm, "attempt to index a %s key, integer or string expected", lgx_value_typeof(&R(pc)));
                    }
                } else {
                    lgx_vm_throw_s(vm, "attempt to index a %s value, array expected", lgx_value_typeof(&R(pb)));
                }
                */
                break;
            }
            case OP_LOAD:{
                /*
                unsigned pd = PD(i);

                lgx_gc_ref_del(&R(pa));
                if (UNEXPECTED(lgx_value_dup(C(pd), &R(pa)) != 0)) {
                    lgx_vm_throw_s(vm, "out of memory");
                    break;
                }
                lgx_gc_ref_add(&R(pa));
                */
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
            case OP_THROW: {
                lgx_vm_throw_v(vm, &R(pa));
                break;
            }
            case OP_NOP: break;
            case OP_HLT: {
                // 释放所有局部变量和临时变量
                int n;
                for (n = 0; n < R(0).v.fun->stack_size; n ++) {
                    lgx_gc_ref_del(&R(n));
                    R(n).type = T_UNKNOWN;
                }

                // 写入返回值
                R(1).type = T_UNKNOWN;

                return 0;
            }
            case OP_TYPEOF:{
                /*
                lgx_gc_ref_del(&R(pa));
                lgx_op_typeof(&R(pa), &R(pb));
                */
                break;
            }
            default:
                lgx_vm_throw_s(vm, "unknown op %d @ %d", OP(i), vm->co_running->pc - 1);
        }
    }

    return 0;
}

int lgx_vm_start(lgx_vm_t *vm) {
    assert(vm->co_running);

    while (1) {
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
            // error: dead lock
            return 1;
        }
    }

    return 0;
}