#include "syntax.hpp"

namespace xscript::parser {

syntax::syntax(std::string path) :
    scanner(path)
{

}

bool syntax::parse() {
    return false;
}

}
