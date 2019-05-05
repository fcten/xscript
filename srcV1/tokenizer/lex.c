#include <string.h>

#include "lex.h"

extern const lgx_token_str_t reserved_words[LGX_RESERVED_WORDS];

static int is_next(lgx_lex_t* ctx, char n) {
    if (ctx->offset >= ctx->length) {
        return 0;
    }

    if (ctx->source[ctx->offset] == n) {
        ctx->offset++;
        return 1;
    } else {
        return 0;
    }
}

// 行
static void step_to_eol(lgx_lex_t* ctx) {
    char n;
    while (ctx->offset < ctx->length) {
        n = ctx->source[ctx->offset++];
        if (n == '\r') {
            if (is_next(ctx, '\n')) {
                ctx->offset -= 2;
                break;
            } else {
                // error
            }
        } else if (n == '\n') {
            ctx->offset--;
            break;
        }
    }
}

// 字符串
static void step_to_eos(lgx_lex_t* ctx, char end) {
    while (ctx->offset < ctx->length) {
        // 处理转义字符 \r \n \t \\ \" \' \0 \xFF
        if (is_next(ctx, '\\')) {
            // 这里只要确保读到正确的字符串结尾，不需要判断转义字符是否合法
            ctx->offset++;
        } else if (is_next(ctx, end)) {
            break;
        } else {
            ctx->offset++;
        }
    }
}

// 注释
static void step_to_eoc(lgx_lex_t* ctx) {
    char n;
    while (ctx->offset < ctx->length) {
        n = ctx->source[ctx->offset++];
        if (n == '*') {
            if (is_next(ctx, '/')) {
                break;
            }
        } else if (n == '\r') {
            if (is_next(ctx, '\n')) {
                ctx->line++;
            } else {
                // error
            }
        } else if (n == '\n') {
            ctx->line++;
        }
    }
}

// 空格与制表符
static void step_to_eot(lgx_lex_t* ctx) {
    char n;
    while (ctx->offset < ctx->length) {
        n = ctx->source[ctx->offset];
        if (n != ' ' && n != '\t') {
            break;
        } else {
            ctx->offset++;
        }
    }
}

// 二进制
static void step_to_eonb(lgx_lex_t* ctx) {
    char n;
    while (ctx->offset < ctx->length) {
        n = ctx->source[ctx->offset];
        if ( n >= '0' && n <= '1' ) {
            ctx->offset++;
        } else {
            break;
        }
    }
}

// 十六进制
static void step_to_eonh(lgx_lex_t* ctx) {
    char n;
    while (ctx->offset < ctx->length) {
        n = ctx->source[ctx->offset];
        if ( (n >= '0' && n <= '9') ||
             (n >= 'a' && n <= 'f') ||
             (n >= 'A' && n <= 'F') ) {
            ctx->offset++;
        } else {
            break;
        }
    }
}

// 十进制
// TODO 科学计数法
static int step_to_eond(lgx_lex_t* ctx) {
    char n;
    int f = 0;
    while (ctx->offset < ctx->length) {
        n = ctx->source[ctx->offset];
        if ( n >= '0' && n <= '9' ) {
            ctx->offset++;
        } else if (n == '.') {
            if (f) {
                break;
            } else {
                ctx->offset++;
                f = 1;
            }
        } else {
            break;
        }
    }
    if (f) {
        return TK_DOUBLE;
    } else {
        return TK_LONG;
    }
}

// 数字
static int step_to_eon(lgx_lex_t* ctx) {
    char n = ctx->source[ctx->offset++];
    if (n == '0') {
        n = ctx->source[ctx->offset++];
        if (n == 'b' || n == 'B') {
            step_to_eonb(ctx);
            return TK_LONG;
        } else if (n == 'x' || n == 'X') {
            step_to_eonh(ctx);
            return TK_LONG;
        } else {
            ctx->offset--;
            return step_to_eond(ctx);
        }
    } else {
        return step_to_eond(ctx);
    }
}

// 标识符
static void step_to_eoi(lgx_lex_t* ctx) {
    char n;
    while (ctx->offset < ctx->length) {
        n = ctx->source[ctx->offset];
        if ((n >= '0' && n <= '9') || (n >= 'a' && n <= 'z') ||
            (n >= 'A' && n <= 'Z') || n == '_') {
            ctx->offset++;
        } else {
            break;
        }
    }
}

// 字典树
struct dict_tree {
    int count;
    int token;
    struct dict_tree* next[26];
};

struct dict_tree root;

void dict_tree_add(int token, char* s) {
    struct dict_tree* node = &root;
    size_t l = strlen(s), i;
    for (i = 0; i < l; ++i) {
        if (node->next[s[i] - 'a'] == NULL) {
            node->next[s[i] - 'a'] = xcalloc(1, sizeof(struct dict_tree));
        }
        node = node->next[s[i] - 'a'];
    }
    node->count++;
    node->token = token;
}

void dict_tree_delete(struct dict_tree* p) {
    int i;
    for (i = 0; i < 26; i++) {
        if (p->next[i]) {
            dict_tree_delete(p->next[i]);
            xfree(p->next[i]);
        }
    }
}

static int is_token(lgx_lex_t* ctx) {
    struct dict_tree* node = &root;
    char n;
    int i;

    for (i = ctx->milestone; i < ctx->offset; i++) {
        n = ctx->source[i];

        if (n < 'a' || n > 'z') {
            return TK_IDENTIFIER;
        }

        if (node->next[n - 'a']) {
            node = node->next[n - 'a'];
        } else {
            return TK_IDENTIFIER;
        }
    }

    if (node->count > 0) {
        return node->token;
    } else {
        return TK_IDENTIFIER;
    }
}

int lgx_lex_init() {
    int i;
    for (i = 0; i < LGX_RESERVED_WORDS; i++) {
        dict_tree_add(reserved_words[i].token, reserved_words[i].s);
    }
    //printf("[Info] %d reserved words\n", LGX_RESERVED_WORDS);
    return 0;
}

int lgx_lex_cleanup() {
    dict_tree_delete(&root);
    return 0;
}

int lgx_lex(lgx_lex_t* ctx) {
    if (ctx->offset >= ctx->length) {
        return TK_EOF;
    }

    ctx->milestone = ctx->offset;

    char n = ctx->source[ctx->offset++];

    switch (n) {
        case '\r':
            if (is_next(ctx, '\n')) {
                ctx->line++;
                ctx->line_start = ctx->offset;
                return TK_EOL;
            } else {
                return TK_UNKNOWN;
            }
        case '\n':  // 换行
            ctx->line++;
            ctx->line_start = ctx->offset;
            return TK_EOL;
        case '\t':
        case ' ':  // 空格与制表符
            step_to_eot(ctx);
            return TK_SPACE;
        case '+':
            if (is_next(ctx, '=')) {
                return TK_ADD_ASSIGN;
            } else if (is_next(ctx, '+')) {
                return TK_INC;
            } else {
                return TK_ADD;
            }
        case '-':
            if (is_next(ctx, '>')) {
                return TK_ARROW;
            } else if (is_next(ctx, '=')) {
                return TK_SUB_ASSIGN;
            } else if (is_next(ctx, '-')) {
                return TK_DEC;
            } else {
                return TK_SUB;
            }
        case '*':
            if (is_next(ctx, '=')) {
                return TK_MUL_ASSIGN;
            } else {
                return TK_MUL;
            }
        case '/':
            if (is_next(ctx, '/')) {
                step_to_eol(ctx);
                return TK_COMMENT;
            } else if (is_next(ctx, '*')) {
                step_to_eoc(ctx);
                return TK_COMMENT;
            } else if (is_next(ctx, '=')) {
                return TK_DIV_ASSIGN;
            } else {
                return TK_DIV;
            }
        case '%':
            if (is_next(ctx, '=')) {
                return TK_MOD_ASSIGN;
            } else {
                return TK_MOD;
            }
        case '>':
            if (is_next(ctx, '=')) {
                return TK_GREATER_EQUAL;
            } else if (is_next(ctx, '>')) {
                if (is_next(ctx, '=')) {
                    return TK_SHR_ASSIGN;
                } else {
                    return TK_SHR;
                }
            } else {
                return TK_GREATER;
            }
        case '<':
            if (is_next(ctx, '=')) {
                return TK_LESS_EQUAL;
            } else if (is_next(ctx, '<')) {
                if (is_next(ctx, '=')) {
                    return TK_SHL_ASSIGN;
                } else {
                    return TK_SHL;
                }
            } else {
                return TK_LESS;
            }
        case '=':
            if (is_next(ctx, '=')) {
                return TK_EQUAL;
            } else {
                return TK_ASSIGN;
            }
        case '!':
            if (is_next(ctx, '=')) {
                return TK_NOT_EQUAL;
            } else {
                return TK_NOT;
            }
        case '&':
            if (is_next(ctx, '&')) {
                return TK_LOGIC_AND;
            } else if (is_next(ctx, '=')) {
                return TK_AND_ASSIGN;
            } else {
                return TK_AND;
            }
        case '|':
            if (is_next(ctx, '|')) {
                return TK_LOGIC_OR;
            } else if (is_next(ctx, '=')) {
                return TK_OR_ASSIGN;
            } else {
                return TK_OR;
            }
        case '~':
            if (is_next(ctx, '=')) {
                return TK_NOT_ASSIGN;
            } else {
                return TK_NOT;
            }
        case '^':
            if (is_next(ctx, '=')) {
                return TK_XOR_ASSIGN;
            } else {
                return TK_XOR;
            }
        case '(': return TK_LEFT_PAREN;
        case ')': return TK_RIGHT_PAREN;
        case '[': return TK_LEFT_BRACK;
        case ']': return TK_RIGHT_BRACK;
        case '{': return TK_LEFT_BRACE;
        case '}': return TK_RIGHT_BRACE;
        case '.': return TK_DOT;
        case ':': return TK_COLON;
        case ';': return TK_SEMICOLON;
        case ',': return TK_COMMA;
        case '"':
            step_to_eos(ctx, '"');
            return TK_STRING;
        case '\'':
            step_to_eos(ctx, '\'');
            return TK_CHAR;
        default:
            if (n >= '0' && n <= '9') {
                ctx->offset--;
                return step_to_eon(ctx);
            } else if (n == '_' || (n >= 'a' && n <= 'z') ||
                       (n >= 'A' && n <= 'Z')) {
                step_to_eoi(ctx);
                return is_token(ctx);
            } else {
                // 非法字符
                return TK_UNKNOWN;
            }
    }
}
