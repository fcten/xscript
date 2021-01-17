#include <iostream>
#include "syntax.hpp"

namespace xscript::parser {

syntax::syntax(std::string path) :
    scanner(path),
    line(1)
{

}

// 解析下一个 token
void syntax::next() {
    while (true) {
        tokenizer::token token = scanner.next();
        switch (token.type) {
            case tokenizer::TK_SPACE:
            case tokenizer::TK_TAB:
                // 忽略空格与制表符
                break;
            case tokenizer::TK_COMMENT_LINE:
                // 跳过单行注释
                while (token != tokenizer::TK_EOF && token != tokenizer::TK_EOL) {
                    token = scanner.next();
                    if (token == tokenizer::TK_EOL) {
                        line++;
                    }
                }
                break;
            case tokenizer::TK_COMMENT_BEGIN:
                // 跳过多行注释
                while (token != tokenizer::TK_EOF && token != tokenizer::TK_COMMENT_END) {
                    token = scanner.next();
                    if (token == tokenizer::TK_EOL) {
                        line++;
                    }
                }
                break;
            case tokenizer::TK_EOL:
                line++;
                prev_token = cur_token;
                cur_token = token;
                return;
            case tokenizer::TK_EOF:
                // 扫描结束
                if (cur_token == tokenizer::TK_EOF) {
                    return;
                }
            default:
                prev_token = cur_token;
                cur_token = token;
                return;
        }
    }
}

bool syntax::parse() {
    while (cur_token != tokenizer::TK_EOF) {
        syntax_rule rule;

        // 读取规则名
        next();
        if (cur_token != tokenizer::TK_IDENTIFIER) {
            if (cur_token == tokenizer::TK_EOL) {
                // 跳过空行
                continue;
            } else if (cur_token == tokenizer::TK_EOF) {
                return true;
            }
            std::cout << "[syntax error] [main.x:" << line << "] `identifier` expected, `" << cur_token.literal << "`(" << cur_token.type << ") found" << std::endl;
            return false;
        }
        rule.name = cur_token.literal;

        // 读取 <-
        next();
        if (cur_token != tokenizer::TK_LEFT_ARROW) {
            std::cout << "[syntax error] [main.x:" << line << "] `<-` expected, `" << cur_token.literal << "`(" << cur_token.type << ") found" << std::endl;
            return false;
        }

        // 读取规则序列
        next();
        while (cur_token != tokenizer::TK_EOF && cur_token != tokenizer::TK_EOL) {
            rule.chain.push_back(cur_token.literal);
            next();
        }

        rules.push_back(rule);
    }

    return true;
}

}
