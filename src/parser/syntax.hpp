#ifndef _XSCRIPT_PARSER_SYNTAX_HPP_
#define _XSCRIPT_PARSER_SYNTAX_HPP_

#include <string>
#include <vector>
#include "../tokenizer/scanner.hpp"
#include "syntax_rule.hpp"

namespace xscript::parser {

typedef util::trie<std::string_view, std::string_view> trie;

class syntax {
private:
    tokenizer::scanner scanner;

    std::vector<syntax_rule> rules;
    
    trie trie_root;

    // 起始符

    // 

    // 终止符

    // 词法分析相关数据
    tokenizer::token prev_token;
    tokenizer::token cur_token;

    size_t line;

    void next();

public:
    syntax(std::string path);

    bool parse();

};

}

#endif // _XSCRIPT_PARSER_SYNTAX_HPP_