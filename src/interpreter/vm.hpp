#ifndef _XSCRIPT_INTERPRETER_VM_HPP_
#define _XSCRIPT_INTERPRETER_VM_HPP_

#include <vector>
#include <memory>
#include <cstdint>
#include "coroutine.hpp"

namespace xscript::interpreter {

class vm {
private:
    // 协程
    std::unique_ptr<coroutine> co_running;
    std::vector<std::unique_ptr<coroutine>> co_ready;
    std::vector<std::unique_ptr<coroutine>> co_suspend;
    std::vector<std::unique_ptr<coroutine>> co_died;

    // 协程创建统计
    uint64_t co_id;
    // 协程数量统计
    uint64_t co_count;

public:
    bool run();

};

}

#endif // _XSCRIPT_INTERPRETER_VM_HPP_