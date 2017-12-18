#include <sys/time.h>
#include "sds.h"
#include "adlist.h"
#include "dict.h"
#include "intset.h"

/**
 * 定义全局操作结果代码
 */ 
#define REDIS_OK 0
#define REDIS_ERR -1

/**
 *  对象类型
 */
#define REDIS_STRING 0
#define REDIS_LIST 1
#define REDIS_SET 2
#define REDIS_ZSET 3
#define REDIS_HASH 4

/**
 * 对象编码类型
 */ 
#define REDIS_ENCODING_RAW 0
#define REDIS_ENCODING_INT 1
#define REDIS_ENCODING_HT 2
#define REDIS_ENCODING_ZIPMAP 3
#define REDIS_ENCODING_LINKEDLIST 4
#define REDIS_ENCODING_ZIPLIST 5
#define REDIS_ENCODING_INTSET 6
#define REDIS_ENCODING_SKIPLIST 7
#define REDIS_ENCODING_EMBSTR 8

/**
 * redis对象相关
 */ 
#define REDIS_LRU_BITS 24
#define REDIS_LRU_CLOCK_MAX ((1<<REDIS_LRU_BITS)-1) //24位全为1，即最大的24位数
#define REDIS_LRU_CLOCK_RESOLUTION 1000

typedef struct redisDb{
    dict *dict; //保存库里所有的键值对
    int id; //数据库号码
} redisDb;

typedef struct redisClient{
    int fd; //  套接字描述符
    redisDb *db;    //客户端当前正在使用的数据库
    int dictid; //正在使用的数据库id
    robj *name; //客户端的名字
    sds querybuf;   //查询字符的缓冲区
} redisClient;

struct redisServer{
    redisDb *db;
};

typedef struct redisObject{
    //类型
    unsigned int type:4;
    //编码
    unsigned int encoding:4;
    //最后一次访问的时间
    unsigned int lru:REDIS_LRU_BITS;
    //引用计数
    int refcount;
    //指向底层实现结构的指针
    void *ptr;
} robj;

/**
 * 返回LRU时钟时间，此为简化版本，每次都要进行系统调用，取精确的时间
 */ 
#define LRU_CLOCK() (getLRUClock())

/**
 * 系统核心函数
 */
unsigned int getLRUClock(void);

/**
 * redisObject相关函数
 */
robj *createObject(int type, void *ptr);
robj *createStringObject(char *ptr, size_t len);
robj *createRawStringObject(char *ptr, size_t len);
robj *createStringObjectFromLongLong(long long value);
robj *createStringObjectFromLongDouble(long double value);
robj *dupStringObject(robj *o);

robj *createListObject(void);
robj *createSetObject(void);
robj *createIntsetObject(void);
robj *createHashObject(void);

void incrRefCount(robj *o);
void decrRefCount(robj *o);
void decrRefCountVoid(void *o);
robj *resetRefCount(robj *obj);
void freeStringObject(robj *o);
void freeListObject(robj *o);
void freeSetObject(robj *o);
void freeZsetObject(robj *o);
void freeHashObject(robj *o);

/**
 * 对外公开的数据
 */
extern dictType setDictType;
extern dictType hashDictType;
/**
 * 工具函数
 */
long long ustime(void);
long long mstime(void);