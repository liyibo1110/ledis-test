#include "redis.h"

void _redisAssert(char *estr, char *file, int line){
    redisLog("=====ASSERTION FAILED=====");
    redisLog("===> %s:%d '%s' is not true", file, line, estr);
}

void _redisPanic(char *msg, char *file, int line){
    redisLog("------------------------------------------------------");
    redisLog("error at：%s #%s:%d", msg, file, line);
    redisLog("------------------------------------------------------");
}