#include "../common/common.h"
#include "source.h"

// 读取源文件以初始化 lgx_source_t 结构
int lgx_source_init(lgx_source_t* source, char *path) {
    // 初始化
    memset(source, 0, sizeof(lgx_source_t));

    int err_no = 0;

    FILE* fp = NULL;
    fp = fopen(path, "rb");
    if (!fp) {
        err_no = 1;
        goto error;
    }

    source->path = strdup(path);
    if (!source->path) {
        err_no = 2;
        goto error;
    }

    // 定位到文件末尾
    if (fseek(fp, 0L, SEEK_END) != 0) {
        err_no = 3;
        goto error;
    }

    // 得到文件大小
    // 注意：源文件长度不能大于 2GB
    int flen = ftell(fp);

    /* 根据文件大小动态分配内存空间 */
    source->content = (char*)xmalloc(flen);
    if (!source->content) {
        err_no = 4;
        goto error;
    }

    // 定位到文件开头
    if (fseek(fp, 0L, SEEK_SET) != 0) {
        err_no = 5;
        goto error;
    }

    // 一次性读取全部文件内容
    source->length = fread(source->content, 1, flen, fp);
    if (source->length != flen) {
        err_no = 6;
        goto error;
    }

    fclose(fp);

    return 0;

error:
    if (fp) {
        fclose(fp);
    }

    lgx_source_cleanup(source);

    return err_no;
}

int lgx_source_cleanup(lgx_source_t* source) {
    if (source->path) {
        xfree(source->path);
    }

    if (source->content) {
        xfree(source->content);
    }

    memset(source, 0, sizeof(lgx_source_t));

    return 0;
}