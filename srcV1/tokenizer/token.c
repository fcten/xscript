#include <string.h>
#include "../common/common.h"
#include "token.h"

typedef struct {
    lgx_token_t token;
    char* s;
} lgx_token_str_t;

#define LGX_RESERVED_WORDS (TK_LENGTH - TK_VAR)

const lgx_token_str_t reserved_words[LGX_RESERVED_WORDS] = {
    {TK_VAR, "var"},
    {TK_CONST, "const"},

    {TK_INT, "int"},
    {TK_FLOAT, "float"},
    {TK_BOOL, "bool"},
    {TK_STRING, "string"},
    {TK_ARRAY, "array"},
    {TK_STRUCT, "struct"},
    {TK_INTERFACE, "interface"},
    {TK_FUNCTION, "function"},

    {TK_TYPE, "type"},

    {TK_TRUE, "true"},
    {TK_FALSE, "false"},
    {TK_NULL, "null"},

    {TK_RETURN, "return"},

    {TK_IF, "if"},
    {TK_ELSE, "else"},
    {TK_SWITCH, "switch"},
    {TK_CASE, "case"},
    {TK_DEFAULT, "default"},

    {TK_FOR, "for"},
    {TK_DO, "do"},
    {TK_WHILE, "while"},

    {TK_BREAK, "break"},
    {TK_CONTINUE, "continue"},

    {TK_TRY, "try"},
    {TK_CATCH, "catch"},
    {TK_THROW, "throw"},

    {TK_CO, "co"},

    {TK_IMPORT, "import"},
    {TK_EXPORT, "export"},
    {TK_PACKAGE, "package"}
};

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

int lgx_token_init() {
    int i;
    for (i = 0; i < LGX_RESERVED_WORDS; i++) {
        dict_tree_add(reserved_words[i].token, reserved_words[i].s);
    }
    //printf("[Info] %d reserved words\n", LGX_RESERVED_WORDS);
    return 0;
}

int lgx_token_cleanup() {
    dict_tree_delete(&root);
    return 0;
}

lgx_token_t lgx_token_detect_reserved_word(char *str, int len) {
    struct dict_tree* node = &root;
    char n;
    int i;

    for (i = 0; i < len; i++) {
        n = str[i];

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