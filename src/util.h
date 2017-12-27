#ifndef __REDIS_UTIL_H__
#define __REDIS_UTIL_H__

#include "sds.h"

sds getAbsolutePath(char *filename);
long long memtoll(const char *p, int *err);
#endif // !__REDIS_UTIL_H___
