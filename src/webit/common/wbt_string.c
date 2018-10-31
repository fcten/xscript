/* 
 * File:   wbt_string.c
 * Author: Fcten
 *
 * Created on 2014年8月18日, 下午3:50
 */

#include "wbt_string.h"
#include "wbt_memory.h"

/*
 * 将一个 wbt_str_t 结构体转化为一个标准 C 字符串
 * 由于使用了静态局部变量，该函数返回的标准字符串将在下一次调用该方法时失效。
 */
const char * wbt_stdstr(wbt_str_t * str) {
    static wbt_str_t wbt_stdstr_tmp = wbt_null_string;

    if( str->len + 1 > wbt_stdstr_tmp.len ) {
        void * p = wbt_realloc( wbt_stdstr_tmp.str, str->len + 1 );
        if( p == NULL ) {
            return NULL;
        }
        wbt_stdstr_tmp.str = p;
        wbt_stdstr_tmp.len = str->len + 1;
    }

    wbt_memcpy( wbt_stdstr_tmp.str, str->str, str->len );
    *( wbt_stdstr_tmp.str + str->len ) = '\0';
    
    return wbt_stdstr_tmp.str;
}

/* 
 * 搜索 str2 字符串在 str1 字符串中第一次出现的位置
 * 大小写敏感
 * 如果没有找到该字符串,则返回 -1
 */
int wbt_strpos( wbt_str_t *str1, wbt_str_t *str2 ) {
    int pos = 0, i = 0, flag = 0;

    /* Bugfix: 由于 len 是 size_t 类型，执行减法前必须先判断其大小 */
    if( str1->len < str2->len ) {
        return -1;
    }

    for( pos = 0 ; pos <= (str1->len - str2->len) ; pos ++ ) {
        flag = 0;
        for( i = 0 ; i < str2->len ; i ++ ) {
            if( *(str1->str + pos + i) != *(str2->str + i) ) {
                flag = 1;
                break;
            }
        }
        if( !flag ) {
            return pos;
        }
    }
    
    return -1;
}

/* 
 * 搜索 str2 字符串在 str1 字符串中第一次出现的位置
 * 大小写不敏感
 * 如果没有找到该字符串,则返回 -1
 */
int wbt_stripos( wbt_str_t *str1, wbt_str_t *str2 ) {
    int pos = 0, i = 0, flag = 0;
    char ch1, ch2;

    if( str1->len < str2->len ) {
        return -1;
    }

    for( pos = 0 ; pos <= (str1->len - str2->len) ; pos ++ ) {
        flag = 0;
        for( i = 0 ; i < str2->len ; i ++ ) {
            if ( ( (ch1 = *(str1->str + pos + i)) >= 'A') && (ch1 <= 'Z') ) {
                ch1 += 0x20;
            }
            if ( ( (ch2 = *(str2->str + i)) >= 'A') && (ch2 <= 'Z') ) {
                ch2 += 0x20;
            }
            if( ch1 != ch2 ) {
                flag = 1;
                break;
            }
        }
        if( !flag ) {
            return pos;
        }
    }
    
    return -1;
}

/*
 * 比较字符串 str1 和 str2 的前 len 个字符
 * 大小写敏感
 * 相同返回 0，否则返回第一个不相等字符的差值
 */
int wbt_strncmp( wbt_str_t *str1, wbt_str_t *str2, int len ) {
    int pos = 0;
    
    for( pos = 0; pos < len ; pos ++ ) {
        if( *(str1->str + pos) != *(str2->str + pos) ) {
            return *(str1->str + pos) - *(str2->str + pos);
        }
    }

    return 0;
}

/*
 * 比较字符串 str1 和 str2 的前 len 个字符
 * 大小写不敏感
 * 相同返回 0，否则返回差值
 */
int wbt_strnicmp( wbt_str_t *str1, wbt_str_t *str2, int len ) {
    int pos = 0;
    char ch1, ch2;
    
    for( pos = 0; pos < len ; pos ++ ) {
        if ( ( (ch1 = *(str1->str + pos)) >= 'A') && (ch1 <= 'Z') ) {
            ch1 += 0x20;
        }
        if ( ( (ch2 = *(str2->str + pos)) >= 'A') && (ch2 <= 'Z') ) {
            ch2 += 0x20;
        }
        if( ch1 != ch2 ) {
            return ch1 - ch2;
        }
    }

    return 0;
}

/*
 * 比较字符串 str1 和 str2
 * 大小写不敏感
 * 相同返回 0，否则返回差值
 */
int wbt_stricmp( wbt_str_t *str1, wbt_str_t *str2 ) {
    int value, length;

    if( str1->len > str2->len ) {
        length = str2->len;
    } else {
        length = str1->len;
    }
    
    value = wbt_strnicmp( str1, str2, length );
    if(value != 0) return value;

    if( str1->len > str2->len ) {
        value = *(str1->str + length);
    } else if( str1->len < str2->len ) {
        value = - *(str2->str + length);
    } else {
        value = 0;
    }

    return value;
}

/*
 * 比较字符串 str1 和 str2
 * 大小写敏感
 * 相同返回 0，否则返回第一个不相等字符的差值
 */
int wbt_strcmp( wbt_str_t *str1, wbt_str_t *str2) {
    int value, length;

    if( str1->len > str2->len ) {
        length = str2->len;
    } else {
        length = str1->len;
    }
    
    value = wbt_strncmp( str1, str2, length );
    if(value != 0) return value;

    if( str1->len > str2->len ) {
        value = *(str1->str + length);
    } else if( str1->len < str2->len ) {
        value = - *(str2->str + length);
    } else {
        value = 0;
    }

    return value;
}

/* 连接字符串 */
void wbt_strcat( wbt_str_t * dest, wbt_str_t * src, int max_len ) {
    /* 防止缓冲区溢出 */
    int src_len = src->len;
    if( dest->len + src->len > max_len ) src_len = max_len - dest->len;

    wbt_memcpy( dest->str + dest->len, src->str, src_len );
    dest->len += src_len;
}

int wbt_atoi(wbt_str_t * str) {
    /* warning: A string of more than 16 characters is not supported */
    if(str->len>=16) {
        return -1;
    }
    
    char temp[32];
    strncpy(temp,str->str,str->len);
    temp[str->len] = '\0';
    return atoi(temp);
}

unsigned long long int wbt_str_to_ull(wbt_str_t * str, int base) {
    if(str->len>=21) {
        return -1;
    }
    
    char temp[32];
    strncpy(temp,str->str,str->len);
    temp[str->len] = '\0';
    return strtoull(temp, NULL, base);
}
