#ifndef LGX_SOURCE_H
#define LGX_SOURCE_H

typedef struct lgx_source_s {
    // 源文件绝对路径
    char *path;
    // 源文件内容
    char *content;
    // 源文件长度
    int length;
} lgx_source_t;

int lgx_source_init(lgx_source_t* source, char *path);
int lgx_source_cleanup(lgx_source_t* source);

#endif // LGX_SOURCE_H