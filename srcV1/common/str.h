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

#define lgx_str_set(stri, text)  (stri).length = sizeof(text) - 1; (stri).buffer = (char *) text
#define lgx_str_set_null(stri)   (stri).length = 0; (stri).buffer = NULL

int lgx_str_init(lgx_str_t* str);
int lgx_str_cleanup(lgx_str_t* str);

#endif // LGX_STR_H