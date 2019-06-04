#ifndef LGX_VALUE_H
#define LGX_VALUE_H

#include "../parser/type.h"

int lgx_value_init(lgx_value_t* v);
void lgx_value_cleanup(lgx_value_t* v);

int lgx_value_cmp(lgx_value_t* v1, lgx_value_t* v2);

void lgx_array_print(lgx_array_t* arr);
void lgx_value_print(lgx_value_t* v);

char* lgx_value_typeof(lgx_value_t* v);

lgx_string_t* lgx_string_new();
lgx_array_t* lgx_array_new();
lgx_function_t* lgx_fucntion_new();

void lgx_array_cleanup(lgx_ht_t* arr);

int lgx_string_dup(lgx_string_t* src, lgx_string_t* dst);
int lgx_array_dup(lgx_array_t* src, lgx_array_t* dst);
int lgx_value_dup(lgx_value_t* src, lgx_value_t* dst);

#endif // LGX_VALUE_H