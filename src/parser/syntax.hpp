#ifndef _XSCRIPT_PARSER_SYNTAX_HPP_
#define _XSCRIPT_PARSER_SYNTAX_HPP_

#include <string>
#include "../tokenizer/scanner.hpp"

namespace xscript::parser {

class syntax {
private:
    tokenizer::scanner scanner;


    // 起始符

    // 

    // 终止符

public:
    syntax(std::string path);

    bool parse();
};

}

#endif // _XSCRIPT_PARSER_SYNTAX_HPP_