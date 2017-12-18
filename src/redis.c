#include "redis.h"

/**
 * 返回当前系统秒数，但只返回后24位
 */ 
unsigned int getLRUClock(void){
   return (mstime() / REDIS_LRU_CLOCK_RESOLUTION) & REDIS_LRU_CLOCK_MAX;
}

/**
 * 返回当前微秒级时间
 */ 
long long ustime(void){
    struct timeval tv;
    long long result;
    gettimeofday(&tv, NULL);
    result = ((long long)tv.tv_sec) * 1000 * 1000;
	return result += tv.tv_usec;
}

/**
 * 返回当前毫秒级时间
 */ 
long long mstime(void){
    return ustime() / 1000;
}