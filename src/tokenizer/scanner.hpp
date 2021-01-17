#ifndef _XSCRIPT_TOKENIZER_SCANNER_HPP_
#define _XSCRIPT_TOKENIZER_SCANNER_HPP_

#include <string>
#include "token.hpp"
#include "../util/trie.hpp"

namespace xscript::tokenizer {

typedef util::trie<token_t, char> trie;

class scanner {
private:
    static std::unordered_map<char, trie*> trie_root;
    static std::unordered_map<char, trie*> init_trie();

    std::string_view source;

    // 当前偏移
    size_t offset;

    // 当前 token 起始偏移
    size_t milestone;

    bool is_next(char n);
    void step_to_eos(char n);
    token_t step_to_eon();
    token_t step_to_eoi();
    void step_to_eonb();
    void step_to_eonh();
    token_t step_to_eond();
    token_t step_to_eot();

 public:
    scanner(std::string_view s);

    token next();

    std::string_view get_literal();

    void print();
    void reset();
};

}

#endif // _XSCRIPT_TOKENIZER_SCANNER_HPP_