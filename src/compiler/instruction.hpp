#ifndef _XSCRIPT_COMPILER_INSTRUCTION_HPP_
#define _XSCRIPT_COMPILER_INSTRUCTION_HPP_

#include "operation.hpp"

namespace xscript::compiler {

class instruction {
private:
    // 指令类型
    operation type;

public:
    uint32_t generate();

};

}

#endif // _XSCRIPT_COMPILER_INSTRUCTION_HPP_