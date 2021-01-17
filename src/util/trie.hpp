#ifndef _XSCRIPT_UTIL_TRIE_HPP_
#define _XSCRIPT_UTIL_TRIE_HPP_

#include <string>
#include <unordered_map>

namespace xscript::util {

template<typename T, typename N>
class trie {
public:
    T token;

    std::unordered_map<N, trie<T, N>> children;

};

}

#endif // _XSCRIPT_UTIL_TRIE_HPP_