#ifndef LGX_STR_H
#define LGX_STR_H

#include "typedef.h"

lgx_str_t* lgx_str_new(char *str, unsigned len);
lgx_str_t* lgx_str_new_ref(char *str, unsigned len);
lgx_str_t* lgx_str_new_with_esc(char *str, unsigned len);
void lgx_str_delete(lgx_str_t *str);

int lgx_str_cmp(lgx_str_t *str1, lgx_str_t *str2);
lgx_str_t* lgx_str_copy(lgx_str_t *str);
int lgx_str_concat(lgx_str_t *str1, lgx_str_t *str2);

#endif // LGX_STR_H