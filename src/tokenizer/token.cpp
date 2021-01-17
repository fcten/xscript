#include "token.hpp"

namespace xscript::tokenizer {

token::token() :
    type(TK_UNKNOWN)
{

}

token::token(token_t t) :
    type(t)
{

}

token::token(token_t t, std::string_view v) :
    type(t),
    literal(v)
{

}

}
