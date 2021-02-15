#ifndef _XSCRIPT_INTERPRETER_COROUTINE_HPP_
#define _XSCRIPT_INTERPRETER_COROUTINE_HPP_

#include <cstdint>
#include <vector>

namespace xscript::interpreter {

typedef enum {
    CO_READY,
    CO_RUNNING,
    CO_SUSPEND,
    CO_DIED
} co_status_t;

class coroutine {
private:
    // 协程 ID
    uint64_t id;
    // 协程状态
    co_status_t status;
    // 协程栈
    std::vector<char> stack;
    // 程序计数
    unsigned pc;
    // 协程 yield 时触发的回调函数
    //int (*on_yield)(lgx_vm_t *vm);
    void *ctx;
    // 所属的虚拟机对象
    //lgx_vm_t *vm;
    // 父协程
    struct lgx_co_s *parent;
    // 当前阻塞等待的子协程
    struct lgx_co_s *child;
    // 引用计数
    unsigned ref_cnt;
    

};

}

#endif // _XSCRIPT_INTERPRETER_COROUTINE_HPP_