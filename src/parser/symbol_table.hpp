#ifndef _XSCRIPT_PARSER_SYMBOL_TABLE_HPP_
#define _XSCRIPT_PARSER_SYMBOL_TABLE_HPP_

#include <string>
#include <unordered_map>
#include "value.hpp"
#include "symbol.hpp"

namespace xscript::parser {

class symbol_table {
private:
    std::unordered_map<std::string_view, symbol> symbols;

public:
    bool add(std::string_view n, symbol s);
    symbol& get(std::string_view n);

};

}

#endif // _XSCRIPT_PARSER_SYMBOL_TABLE_HPP_