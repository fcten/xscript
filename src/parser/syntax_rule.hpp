#ifndef _XSCRIPT_PARSER_SYNTAX_RULE_HPP_
#define _XSCRIPT_PARSER_SYNTAX_RULE_HPP_

#include <string>
#include <vector>

namespace xscript::parser {
class syntax_rule {
public:
    std::string_view name;
    std::vector<std::string_view> chain;

};

}

#endif // _XSCRIPT_PARSER_SYNTAX_RULE_HPP_