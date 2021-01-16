#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include "scanner.hpp"

namespace xscript::tokenizer {

scanner::scanner(std::string path) :
    offset(0),
    milestone(0)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        // TODO 抛出错误
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    data = buffer.str();
    source = data;

    // 初始化 trie
    for (auto iter = tokens.begin(); iter != tokens.end(); ++iter) {
        add_token(token(iter->second, iter->first));
    }
}

void scanner::add_token(token t) {
    trie *node = nullptr;
    for (int i = 0 ; i < t.literal.length() ; i++) {
        char n = t.literal.at(i);
        if (node == nullptr) {
            auto it = trie_root.find(n);
            if (it == trie_root.end()) {
                trie_root[n] = trie();
                it = trie_root.find(n);
            }
            node = &it->second;
        } else {
            auto it = node->children.find(n);
            if (it == node->children.end()) {
                node->children[n] = trie();
                it = node->children.find(n);
            }
            node = &it->second;
        }
    }
    
    assert(node->token == TK_UNKNOWN);
    node->token = t.type;
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
            node = &it->second;
        } else {
            auto it = node->children.find(n);
            if (it == node->children.end()) {
                offset--;
                break;
            }
            node = &it->second;
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

}
