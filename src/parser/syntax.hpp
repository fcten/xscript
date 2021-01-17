#ifndef _XSCRIPT_PARSER_SYNTAX_HPP_
#define _XSCRIPT_PARSER_SYNTAX_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include "../tokenizer/scanner.hpp"
#include "ast.hpp"

namespace xscript::parser {

class syntax {
private:
    std::unordered_map<std::string, std::unique_ptr<ast>> packages;

public:
    syntax();

    bool load(std::string path);
    bool reload(std::string path);

};

}

#endif // _XSCRIPT_PARSER_SYNTAX_HPP_