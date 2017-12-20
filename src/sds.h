#ifndef __SDS_H__
#define __SDS_H__

#define SDS_MAX_PREALLOC (1024*1024)    //最大预分配长度为1M

#include <sys/types.h>

/**
 * 给自定义字符串对象起个类型别名
 */ 
typedef char *sds;

struct sdshdr{
    //记录buf数组中已经使用的字节数量
    int len;
    //记录buf数组中未使用字节的数量
    int free;
    //用于保存字符串
    char buf[];
};

sds sdsnew(const char *init);
sds sdsnewlen(const void *init, size_t initlen);
void sdsfree(sds s);
sds sdsempty(void);
size_t sdslen(const sds s);
size_t sdsavail(const sds s);
sds sdsdup(const sds s);
void sdsclear(sds s);
sds sdscat(sds s, const char *t);
sds sdscatlen(sds s, const void *t, size_t len);
sds sdscatsds(sds s, const sds t);
sds sdscpy(sds s, const char *t);
sds sdscpylen(sds s, const char *t, size_t len);
int sdscmp(const sds s1, const sds s2);
sds sdstrim(sds s, const char *cset);
void sdsrange(sds s, int start, int end);
void sdstolower(sds s);
void sdstoupper(sds s);
size_t sdsAllocSize(sds s);

sds sdscatrepr(sds s, const char *p, size_t len);

sds sdsfromlonglong(long long value);

//**************底层API***************************//
sds sdsMakeRoom(sds s, size_t addlen);
sds sdsRemoveFreeSpace(sds s);

#endif // !__SDS_H___