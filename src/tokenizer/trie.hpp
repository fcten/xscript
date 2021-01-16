#ifndef _XSCRIPT_TOKENIZER_TRIE_HPP_
#define _XSCRIPT_TOKENIZER_TRIE_HPP_

#include <string>
#include <unordered_map>
#include "token.hpp"

namespace xscript::tokenizer {

class trie {
public:
    token_t token;

    std::unordered_map<char, trie> children;

};

}

#endif // _XSCRIPT_TOKENIZER_TRIE_HPP_