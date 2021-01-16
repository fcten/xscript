#ifndef _XSCRIPT_TOKENIZER_SCANNER_HPP_
#define _XSCRIPT_TOKENIZER_SCANNER_HPP_

#include <string>
#include "token.hpp"
#include "trie.hpp"

namespace xscript::tokenizer {

class scanner {
private:
    std::unordered_map<char, trie> trie_root;

    std::string data;
    std::string_view source;

    // 当前偏移
    size_t offset;

    // 当前 token 起始偏移
    size_t milestone;

    void add_token(token t);

    bool is_next(char n);
    void step_to_eos(char n);
    token_t step_to_eon();
    token_t step_to_eoi();
    void step_to_eonb();
    void step_to_eonh();
    token_t step_to_eond();
    token_t step_to_eot();

 public:
    scanner(std::string path);

    token next();

    std::string_view get_literal();

    void print();
};

}

#endif // _XSCRIPT_TOKENIZER_SCANNER_HPP_