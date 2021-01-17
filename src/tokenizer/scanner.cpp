#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <unordered_map>
#include "scanner.hpp"

namespace xscript::tokenizer {

std::unordered_map<std::string_view, token_t> tokens = {

    // TK_UNKNOWN

    // TK_EOF

    {"\r\n",        TK_EOL},
    {"\n",          TK_EOL},

    {"//",          TK_COMMENT_LINE},
    {"/*",          TK_COMMENT_BEGIN},
    {"*/",          TK_COMMENT_END},
    
    {" ",           TK_SPACE},
    {"\t",          TK_TAB},

    // TK_IDENTIFIER

    // TK_LITERAL_LONG
    // TK_LITERAL_DOUBLE
    // TK_LITERAL_CHAR
    // TK_LITERAL_STRING

    {"+",          TK_ADD},
    {"-",          TK_SUB},
    {"*",          TK_MUL},
    {"/",          TK_DIV},
    {"%",          TK_MOD},

    {"&",          TK_BITWISE_AND},
    {"|",          TK_BITWISE_OR},
    {"~",          TK_BITWISE_NOT},
    {"^",          TK_BITWISE_XOR},
    {"<<",         TK_BITWISE_SHL},
    {">>",         TK_BITWISE_SHR},

    {"=",          TK_ASSIGN},
    {"+=",         TK_ASSIGN_ADD},
    {"-=",         TK_ASSIGN_SUB},
    {"*=",         TK_ASSIGN_MUL},
    {"/=",         TK_ASSIGN_DIV},
    {"%=",         TK_ASSIGN_MOD},
    {"&=",         TK_ASSIGN_AND},
    {"|=",         TK_ASSIGN_OR},
    {"^=",         TK_ASSIGN_XOR},
    {"<<=",        TK_ASSIGN_SHL},
    {">>=",        TK_ASSIGN_SHR},

    {"&&",         TK_LOGIC_AND},
    {"||",         TK_LOGIC_OR},
    {"!",          TK_LOGIC_NOT},

    {"++",         TK_INC},
    {"--",         TK_DEC},

    {"==",         TK_EQUAL},
    {"!=",         TK_NOT_EQUAL},
    {">",          TK_GREATER},
    {"<",          TK_LESS},
    {">=",         TK_GREATER_EQUAL},
    {"<=",         TK_LESS_EQUAL},

    {"(",          TK_LEFT_PAREN},
    {")",          TK_RIGHT_PAREN},
    {"[",          TK_LEFT_BRACK},
    {"]",          TK_RIGHT_BRACK},
    {"{",          TK_LEFT_BRACE},
    {"}",          TK_RIGHT_BRACE},

    {",",          TK_COMMA},
    {".",          TK_DOT},
    {":",          TK_COLON},
    {";",          TK_SEMICOLON},
    {"->",         TK_RIGHT_ARROW},
    {"<-",         TK_LEFT_ARROW},
    {"...",        TK_ELLIPSIS}
};

std::unordered_map<std::string_view, token_t> reserved_words = {

    {"var",         TK_VAR},
    {"const",       TK_CONST},

    {"int",         TK_INT},
    {"float",       TK_FLOAT},
    {"bool",        TK_BOOL},
    {"string",      TK_STRING},
    {"struct",      TK_STRUCT},
    {"interface",   TK_INTERFACE},
    {"func",        TK_FUNCTION},

    {"type",        TK_TYPE},

    {"true",        TK_TRUE},
    {"false",       TK_FALSE},
    {"null",        TK_NULL},

    {"return",      TK_RETURN},

    {"if",          TK_IF},
    {"else",        TK_ELSE},
    {"switch",      TK_SWITCH},
    {"case",        TK_CASE},
    {"default",     TK_DEFAULT},

    {"for",         TK_FOR},
    {"do",          TK_DO},
    {"while",       TK_WHILE},

    {"break",       TK_BREAK},
    {"continue",    TK_CONTINUE},

    {"try",         TK_TRY},
    {"catch",       TK_CATCH},
    {"throw",       TK_THROW},

    {"co",          TK_CO},

    {"import",      TK_IMPORT},
    {"export",      TK_EXPORT},
    {"package",     TK_PACKAGE},

    {"as",          TK_AS},
    {"echo",        TK_ECHO}
};

std::unordered_map<char, trie*> scanner::trie_root = scanner::init_trie();

std::unordered_map<char, trie*> scanner::init_trie() {
    std::unordered_map<char, trie*> root;

    // 初始化 trie
    for (auto iter = tokens.begin(); iter != tokens.end(); ++iter) {
        trie *node = nullptr;
        for (size_t i = 0 ; i < iter->first.length() ; i++) {
            char n = iter->first.at(i);
            if (node == nullptr) {
                auto it = root.find(n);
                if (it == root.end()) {
                    root[n] = new trie();
                    it = root.find(n);
                }
                node = it->second;
            } else {
                auto it = node->children.find(n);
                if (it == node->children.end()) {
                    node->children[n] = new trie();
                    it = node->children.find(n);
                }
                node = it->second;
            }
        }
        
        assert(node->token == TK_UNKNOWN);
        node->token = iter->second;
    }

    return root;
}

scanner::scanner(std::string_view s) :
    offset(0),
    milestone(0)
{
    source = s;
}

bool scanner::is_next(char n) {
    if (offset >= source.length()) {
        return 0;
    }

    if (source.at(offset) == n) {
        offset++;
        return true;
    } else {
        return false;
    }
}

token scanner::next() {
    if (offset >= source.length()) {
        return token(TK_EOF);
    }

    milestone = offset;

    char n = source.at(offset++);
    
    if (n == '"') {
        step_to_eos('"');
        return token(TK_LITERAL_STRING, get_literal());
    }
    
    if (n == '\'') {
        step_to_eos('\'');
        return token(TK_LITERAL_CHAR, get_literal());
    }
    
    if (n >= '0' && n <= '9') {
        offset--;
        token_t t = step_to_eon();
        return token(t, get_literal());
    }
    
    if (n == '_' || (n >= 'a' && n <= 'z') ||
                (n >= 'A' && n <= 'Z')) {
        offset--;
        token_t t = step_to_eoi();
        return token(t, get_literal());
    }

    offset--;
    token_t t = step_to_eot(); 
    return token(t, get_literal());
}

// token
token_t scanner::step_to_eot() {
    trie *node = nullptr;
    token_t t = TK_UNKNOWN;
    while (offset < source.length()) {
        char n = source.at(offset++);
        if (node == nullptr) {
            auto it = trie_root.find(n);
            if (it == trie_root.end()) {
                break;
            }
            node = it->second;
        } else {
            auto it = node->children.find(n);
            if (it == node->children.end()) {
                offset--;
                break;
            }
            node = it->second;
        }
        if (node->token != TK_UNKNOWN) {
            t = node->token;
        }    
    }

    return t;
}

// 字符串
void scanner::step_to_eos(char end) {
    while (offset < source.length()) {
        // 处理转义字符 \r \n \t \\ \" \' \0 \xFF
        if (is_next('\\')) {
            // 这里只要确保读到正确的字符串结尾，不需要判断转义字符是否合法
            offset++;
        } else if (is_next(end)) {
            break;
        } else {
            offset++;
        }
    }
}

// 二进制
void scanner::step_to_eonb() {
    while (offset < source.length()) {
        char n = source.at(offset);
        if ( n >= '0' && n <= '1' ) {
            offset++;
        } else {
            break;
        }
    }
}

// 十六进制
void scanner::step_to_eonh() {
    while (offset < source.length()) {
        char n = source.at(offset);
        if ( (n >= '0' && n <= '9') ||
             (n >= 'a' && n <= 'f') ||
             (n >= 'A' && n <= 'F') ) {
            offset++;
        } else {
            break;
        }
    }
}

// 十进制
// TODO 科学计数法
token_t scanner::step_to_eond() {
    int f = 0;
    while (offset < source.length()) {
        char n = source.at(offset);
        if ( n >= '0' && n <= '9' ) {
            offset++;
        } else if (n == '.') {
            if (f) {
                break;
            } else {
                offset++;
                f = 1;
            }
        } else {
            break;
        }
    }
    if (f) {
        return TK_LITERAL_DOUBLE;
    } else {
        return TK_LITERAL_LONG;
    }
}

// 数字
token_t scanner::step_to_eon() {
    char n = source.at(offset++);
    if (n == '0') {
        n = source.at(offset++);
        if (n == 'b' || n == 'B') {
            step_to_eonb();
            return TK_LITERAL_LONG;
        } else if (n == 'x' || n == 'X') {
            step_to_eonh();
            return TK_LITERAL_LONG;
        } else {
            offset--;
            return step_to_eond();
        }
    } else {
        return step_to_eond();
    }
}

// 标识符
token_t scanner::step_to_eoi() {
    while (offset < source.length()) {
        char n = source.at(offset);
        if ((n >= '0' && n <= '9') || (n >= 'a' && n <= 'z') ||
            (n >= 'A' && n <= 'Z') || n == '_') {
            offset++;
        } else {
            break;
        }
    }

    auto it = reserved_words.find(get_literal());
    if (it == reserved_words.end()) {
        return TK_IDENTIFIER;
    } else {
        return it->second;
    }
}

std::string_view scanner::get_literal() {
    return source.substr(milestone, offset-milestone);
}

void scanner::print() {
    token t;
    do {
        t = next();
        if (t != TK_SPACE && t != TK_TAB && t != TK_EOL && t != TK_EOF) {
            std::cout << t.type << ' ' << t.literal << std::endl;
        }
    } while (t != TK_EOF);
}

void scanner::reset() {
    offset = 0;
    milestone = 0;
}

}
