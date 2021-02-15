#include <cassert>
#include "register_allocator.hpp"

namespace xscript::parser {

register_allocator::register_allocator() {
    register_allocator(256);
}

register_allocator::register_allocator(int32_t max) {
    assert(max > 0);
    
    for (int32_t i = max-1 ; i >= 0 ; i--) {
        regs.push(i);
    }
}

int32_t register_allocator::allocate() {
    if (regs.empty()) {
        return -1;
    }
    int32_t r = regs.top();
    regs.pop();
    return r;
}

void register_allocator::release(int32_t r) {
    regs.push(r);
}

}
