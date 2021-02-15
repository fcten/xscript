#ifndef _XSCRIPT_PARSER_REGISTER_ALLOCATOR_HPP_
#define _XSCRIPT_PARSER_REGISTER_ALLOCATOR_HPP_

#include <stack>
#include <cstdint>

namespace xscript::parser {

class register_allocator {
private:
    std::stack<int32_t> regs;
    
public:
    register_allocator();
    register_allocator(int32_t max);

    int32_t allocate();
    void release(int32_t r);

};

}

#endif // _XSCRIPT_PARSER_REGISTER_ALLOCATOR_HPP_