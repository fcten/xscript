#ifndef _XSCRIPT_PARSER_SYNTAX_RULE_HPP_
#define _XSCRIPT_PARSER_SYNTAX_RULE_HPP_

#include <string>
#include <unordered_map>
#include "../tokenizer/scanner.hpp"

namespace xscript::parser {

class syntax_rule {
private:
    tokenizer::scanner scanner;

    std::unordered_map<std::string_view, std::vector<std::string_view>> rules;
    
public:

};

}

#endif // _XSCRIPT_PARSER_SYNTAX_RULE_HPP_