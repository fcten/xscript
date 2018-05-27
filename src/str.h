#ifndef LGX_STR_H
#define LGX_STR_H

typedef struct {
    unsigned size;
    unsigned length;
    unsigned char buffer[];
} lgx_str_t;

int lgx_str_cmp(lgx_str_t *str1, lgx_str_t *str2);

#endif // LGX_STR_H