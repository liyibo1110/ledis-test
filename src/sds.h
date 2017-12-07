#define SDS_MAX_PREALLOC (1024*1024)    //最大预分配长度为1M

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct sdshdr{
    //记录buf数组中已经使用的字节数量
    int len;
    //记录buf数组中未使用字节的数量
    int free;
    //用于保存字符串
    char buf[];
};

char *sdsnew(const char *init);
char *sdsnewlen(const char *init, size_t initlen);
void sdsfree(char *buf);
char *sdsempty(void);
size_t sdslen(const char *buf);
size_t sdsavail(const char *buf);
char *sdsdup(const char *buf);
void sdsclear(char *buf);
char *sdscat(char *buf, const char *t);
char *sdscatlen(char *buf, const char *t, size_t len);
char *sdscpy(char *buf, const char *t);
char *sdscpylen(char *buf, const char *t, size_t len);
int sdscmp(const char *buf1, const char *buf2);
char *sdstrim(char *buf, const char *cset);
void sdsrange(char *buf, int start, int end);
void sdstolower(char *buf);
void sdstoupper(char *buf);
size_t sdsAllocSize(char *buf);

//**************底层API***************************//
char *sdsMakeRoom(char *buf, size_t addlen);
char *sdsRemoveFreeSpace(char *buf);