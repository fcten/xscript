#include <stdlib.h>
#include <string.h>

#include "lex.h"

struct rw {
    int token;
    char* s;
};

// 关键字 && 保留字
const struct rw reserved_word[] = {
    {TK_AUTO, "auto"},    {TK_FOR, "for"},         {TK_DO, "do"},
    {TK_WHILE, "while"},  {TK_BREAK, "break"},     {TK_CONTINUE, "continue"},
    {TK_IF, "if"},        {TK_ELSE, "else"},       {TK_SWITCH, "switch"},
    {TK_CASE, "case"},    {TK_DEFAULT, "default"}, {TK_FUNCTION, "function"},
    {TK_RETURN, "return"}};

/*
string
number - float, integer
bool
function
array
object
null
resource

true, false

class
*/
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

static void step_to_eos(lgx_lex_t* ctx) {
    while (ctx->offset < ctx->length) {
        if (is_next(ctx, '"')) {
            break;
        } else {
            ctx->offset++;
        }
    }
}

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

static void step_to_eon(lgx_lex_t* ctx) {
    char n;
    while (ctx->offset < ctx->length) {
        n = ctx->source[ctx->offset];
        if (n > '9' || n < '0') {
            break;
        } else {
            ctx->offset++;
        }
    }
}

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
            node->next[s[i] - 'a'] = calloc(1, sizeof(struct dict_tree));
        }
        node = node->next[s[i] - 'a'];
    }
    node->count++;
    node->token = token;
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
    for (i = 0; i < sizeof(reserved_word) / sizeof(struct rw); i++) {
        dict_tree_add(reserved_word[i].token, reserved_word[i].s);
    }
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
        case '-':
        case '*':
            return n;
        case '/':
            if (is_next(ctx, '/')) {
                step_to_eol(ctx);
                return TK_COMMENT;
            } else if (is_next(ctx, '*')) {
                step_to_eoc(ctx);
                return TK_COMMENT;
            } else {
                return n;
            }
        case '>':
            if (is_next(ctx, '=')) {
                return TK_GE;
            } else if (is_next(ctx, '>')) {
                return TK_SHR;
            } else {
                return n;
            }
        case '<':
            if (is_next(ctx, '=')) {
                return TK_LE;
            } else if (is_next(ctx, '<')) {
                return TK_SHL;
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
            } else {
                return n;
            }
        case '|':
            if (is_next(ctx, '|')) {
                return TK_OR;
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
            step_to_eos(ctx);
            return TK_STRING;
        default:
            if (n >= '0' && n <= '9') {
                step_to_eon(ctx);
                return TK_NUMBER;
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
