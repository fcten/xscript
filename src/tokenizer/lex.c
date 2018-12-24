#include "../common/common.h"
#include "lex.h"

extern const lgx_token_t lgx_reserved_words[LGX_RESERVED_WORDS];

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

static int is_reserved(lgx_lex_t* ctx) {
    struct dict_tree* node = &root;
    char n;
    int i;

    for (i = ctx->milestone; i < ctx->offset; i++) {
        n = ctx->source[i];

        if (n < 'a' || n > 'z') {
            return TK_ID;
        }

        if (node->next[n - 'a']) {
            node = node->next[n - 'a'];
        } else {
            return TK_ID;
        }
    }

    if (node->count > 0) {
        return node->token;
    } else {
        return TK_ID;
    }
}

int lgx_lex_init() {
    int i;
    for (i = 0; i < LGX_RESERVED_WORDS; i++) {
        dict_tree_add(lgx_reserved_words[i].token, lgx_reserved_words[i].s);
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
                return TK_EOL;
            } else {
                return TK_UNKNOWN;
            }
        case '\n':  // 换行
            ctx->line++;
            return TK_EOL;
        case '\t':
        case ' ':  // 空格与制表符
            step_to_eot(ctx);
            return TK_SPACE;
        case '+':
            if (is_next(ctx, '=')) {
                return TK_ASSIGN_ADD;
            } else {
                return n;
            }
        case '-':
            if (is_next(ctx, '>')) {
                return TK_PTR;
            } else if (is_next(ctx, '=')) {
                return TK_ASSIGN_SUB;
            } else {
                return n;
            }
        case '*':
            if (is_next(ctx, '=')) {
                return TK_ASSIGN_MUL;
            } else {
                return n;
            }
        case '/':
            if (is_next(ctx, '/')) {
                step_to_eol(ctx);
                return TK_COMMENT;
            } else if (is_next(ctx, '*')) {
                step_to_eoc(ctx);
                return TK_COMMENT;
            } else if (is_next(ctx, '=')) {
                return TK_ASSIGN_DIV;
            } else {
                return n;
            }
        case '>':
            if (is_next(ctx, '=')) {
                return TK_GE;
            } else if (is_next(ctx, '>')) {
                if (is_next(ctx, '=')) {
                    return TK_ASSIGN_SHR;
                } else {
                    return TK_SHR;
                }
            } else {
                return n;
            }
        case '<':
            if (is_next(ctx, '=')) {
                return TK_LE;
            } else if (is_next(ctx, '<')) {
                if (is_next(ctx, '=')) {
                    return TK_ASSIGN_SHL;
                } else {
                    return TK_SHL;
                }
            } else {
                return n;
            }
        case '=':
            if (is_next(ctx, '=')) {
                return TK_EQ;
            } else {
                return n;
            }
        case '!':
            if (is_next(ctx, '=')) {
                return TK_NE;
            } else {
                return n;
            }
        case '&':
            if (is_next(ctx, '&')) {
                return TK_AND;
            } else if (is_next(ctx, '=')) {
                return TK_ASSIGN_AND;
            } else {
                return n;
            }
        case '|':
            if (is_next(ctx, '|')) {
                return TK_OR;
            } else if (is_next(ctx, '=')) {
                return TK_ASSIGN_OR;
            } else {
                return n;
            }
        case '~':
        case '^':
        case '[':
        case ']':
        case '(':
        case ')':
        case '{':
        case '}':
        case '.':
        case ':':
        case ';':
        case ',':
            return n;
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
                return is_reserved(ctx);
            } else {
                // 非法字符
                return TK_UNKNOWN;
            }
    }
}
