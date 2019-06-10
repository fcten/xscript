#ifndef LGX_STR_H
#define LGX_STR_H

typedef struct lgx_str_s {
    // 缓冲区长度
    unsigned size;
    // 已使用的缓冲区长度（字符串长度）
    unsigned length;
    // 指针
    char *buffer;
} lgx_str_t;

#define lgx_str(stri)            {0, sizeof(stri) - 1, stri}
#define lgx_str_set(stri, text)  (stri).length = sizeof(text) - 1; (stri).buffer = (char *) text; (stri).size = 0
#define lgx_str_set_null(stri)   (stri).length = 0; (stri).buffer = NULL; (stri).size = 0

int lgx_str_init(lgx_str_t* str, unsigned size);
void lgx_str_cleanup(lgx_str_t* str);

int lgx_str_resize(lgx_str_t *str, unsigned size);

int lgx_str_cmp(lgx_str_t *str1, lgx_str_t *str2);
void lgx_str_dup(lgx_str_t *src, lgx_str_t *dst);
void lgx_str_concat(lgx_str_t *src, lgx_str_t *dst);
int lgx_str_append(lgx_str_t *src, lgx_str_t *dst);

void lgx_str_print(lgx_str_t *str);

#endif // LGX_STR_H