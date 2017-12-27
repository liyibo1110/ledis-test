#ifndef __REDIS_H___
#define __REDIS_H___

#include <stdio.h>
#include <stdlib.h>
#include "sds.h"
#include "adlist.h"
#include "dict.h"
#include "intset.h"

/**
 * 定义当前软件版本
 */ 
#define REDIS_VERSION "1.0.0"

/**
 * 定义全局操作结果代码
 */ 
#define REDIS_OK 0
#define REDIS_ERR -1

/**
 * 默认服务器配置的值
 */ 
#define REDIS_DEFAULT_HZ 10 //hz值默认为10，即0.1秒调用一次
#define REDIS_MIN_HZ 1  //hz值的下限，每秒至少执行1次
#define REDIS_MAX_HZ 500    //hz值的上限
#define REDIS_SERVERPORT 6379   //默认TCP端口
#define REDIS_TCP_BACKLOG 511   //TCP backlog监听端口
#define REDIS_MAXIDLETIME 0 //默认客户端超时时间为无限
#define REDIS_RUN_ID_SIZE 40
#define REDIS_MAX_CLIENTS 10000 //最大支持10000个客户端
#define REDIS_DEFAULT_DBNUM 16  //默认开启16个数据库
#define REDIS_CONFIGLINE_MAX    1024    //最大配置项数
#define REDIS_DEFAULT_DAEMONIZE 0
#define REDIS_DEFAULT_TCP_KEEPALIVE 0
#define REDIS_DEFAULT_LOGFILE ""    //默认log文件为空
#define REDIS_MAX_LOGMSG_LEN 1024   //最长的log字节数为1k
#define REDIS_DEFAULT_MAXMEMORY 0
#define REDIS_DEFAULT_PID_FILE "/var/run/redis.pid" //默认进程pid文件

// 命令标志
#define REDIS_CMD_WRITE 1                   /* "w" flag */
#define REDIS_CMD_READONLY 2                /* "r" flag */
#define REDIS_CMD_DENYOOM 4                 /* "m" flag */
#define REDIS_CMD_NOT_USED_1 8              /* no longer used flag */
#define REDIS_CMD_ADMIN 16                  /* "a" flag */
#define REDIS_CMD_PUBSUB 32                 /* "p" flag */
#define REDIS_CMD_NOSCRIPT  64              /* "s" flag */
#define REDIS_CMD_RANDOM 128                /* "R" flag */
#define REDIS_CMD_SORT_FOR_SCRIPT 256       /* "S" flag */
#define REDIS_CMD_LOADING 512               /* "l" flag */
#define REDIS_CMD_STALE 1024                /* "t" flag */
#define REDIS_CMD_SKIP_MONITOR 2048         /* "M" flag */
#define REDIS_CMD_ASKING 4096               /* "k" flag */

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

/**
 * debug相关宏函数
 */ 
//参数必须为真，否则提示assert错误并退出
#define redisAssert(_e) ((_e)?(void)0:(_redisAssert(#_e,__FILE__,__LINE__),_exit(1)))
//调用即说明出现error，提示错误并退出
#define redisPanic(_e) _redisPanic(#_e,__FILE__,__LINE__),_exit(1)

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

/**
 * 定义函数指针类型，里面封装具体命令的实现
 */ 
typedef void redisCommandProc(redisClient *c);

/**
 * redis命令封装结构体
 */ 
struct redisCommand{
    char *name; //命令名字
    redisCommandProc *proc;
    int arity;
    char *sflags;   //字符串表示的flag
    int flags;  //实际的flag
    //指定哪些参数时key，用以下3个变量就可以表示任何位置的参数
    int firstkey;
    int lastkey;
    int keystep;
};

struct redisServer{

    /* 通用属性 */
    char *configfile; //配置文件的绝对路径地址，也可以为NULL
    int hz; //serverCron系统周期函数，每秒调用的次数
    redisDb *db;    //数据库
    char *pidfile;  //进程pid文件路径
    int arch_bits;  //架构类型，是32位还是64位的
    char runid[REDIS_RUN_ID_SIZE + 1];  //服务器的RUN ID，每次运行的值都不一样
    unsigned lruclock:REDIS_LRU_BITS;   //当前系统时间的缓存，目前不使用缓存
    int shutdown_asap;  //关闭服务器的标志位

    dict *commands; //命令表（不考虑rename配置项）

    /* 网络相关 */
    int port;   //监听端口
    int tcp_backlog;    //backlog监听端口

    /* 数据库相关 */
    int dbnum;
    int maxidletime;    //客户端最大空转时间
    int tcpkeepalive;   //如果不是0，则开启SO_KEEPALIVE
    int daemonize;  //是否为守护进程

    /* 日志相关 */
    char *logfile;  //log文件路径

    unsigned long long maxmemory;   //最大可用内存
};
 


/**
 * 返回LRU时钟时间，此为简化版本，每次都要进行系统调用，取精确的时间
 */ 
#define LRU_CLOCK() (getLRUClock())

/**
 * 系统核心函数
 */
unsigned int getLRUClock(void);
void populateCommandTable(void);
//如果是gcc编译器，就使用编译安全版的附加功能
#ifdef __GNUC__
void redisLog(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));
#else
void redisLog(const char *fmt, ...);
#endif
void redisLogRaw(const char *msg);

/**
 * 所有命令函数原型
 */
void echoCommand(redisClient *c);

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
 * 配置相关函数
 */

void loadServerConfig(char *filename, char *option);

/**
 * 对外公开的数据
 */
extern struct redisServer server;
extern dictType setDictType;
extern dictType hashDictType;
/**
 * 工具函数
 */
long long ustime(void);
long long mstime(void);
void getRandomHexChars(char *p, unsigned int len);
/**
 * debug相关函数
 */
void _redisAssert(char *estr, char *file, int line);
void _redisPanic(char *msg, char *file, int line);
#endif // !__REDIS_H___