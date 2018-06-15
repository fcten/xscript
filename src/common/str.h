#ifndef LGX_STR_H
#define LGX_STR_H

typedef struct {
    unsigned size;
    unsigned length;
    char buffer[];
} lgx_str_t;

typedef struct {
    unsigned length;
char *buffer;
} lgx_str_ref_t;

lgx_str_t* lgx_str_new(char *str, unsigned len);

int lgx_str_cmp(lgx_str_t *str1, lgx_str_t *str2);

#endif // LGX_STR_H