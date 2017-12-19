#include "redis.h"

/**
 * 全局变量
 */
struct redisServer server;

/**
 * 实现所有的命令结构参数，注意redisCommand并没有使用typedef起别名，所以这里不是定义而是实现
 */
struct redisCommand redisCommandTable[] = {
    {"echo", echoCommand, 2, "r", 0, 0, 0, 0}
};

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

/**
 * hash table type实现
 */ 
unsigned int dictSdsHash(const void *key){
    return dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
}

unsigned int dictSdsCaseHash(const void *key){
    return dictGenCaseHashFunction((unsigned char*)key, sdslen((char*)key));
}

int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2){
    int len1 = sdslen((sds)key1);
    int len2 = sdslen((sds)key2);
    if(len1 != len2){
        return 0;
    }
    return memcmp(key1, key2, len1) == 0;
}

unsigned int dictObjHash(const void *key){
    const robj *o = key;
    return dictGenHashFunction(o->ptr, sdslen((sds)o->ptr));
}

int dictObjKeyCompare(void *privdata, const void *key1, const void *key2){
    const robj *o1 = key1;
    const robj *o2 = key2;
    return dictSdsKeyCompare(privdata, o1->ptr, o2->ptr);
}

int dictSdsKeyCaseCompare(void *privdata, const void *key1, const void *key2){
    return strcasecmp(key1, key2) == 0;
}

void dictSdsDestructor(void *privdata, void *key){
    (void)privdata;
    sdsfree(key);
}

/**
 * 定义command命令hash表的type实现
 * key为sds对象， value为command结构体的指针
 */
dictType commandTableDictType = {
    dictSdsCaseHash,    //hash生成函数
    NULL,               //key复制函数
    NULL,                //value复制函数
    dictSdsKeyCaseCompare,   //key比较函数
    dictSdsDestructor,  //key销毁函数
    NULL                //value销毁函数
};

/**
 * 初始化服务器各项参数
 */ 
void initServerConfig(){

    //设置服务器运行的ID
    getRandomHexChars(server.runid, REDIS_RUN_ID_SIZE);
    //给运行ID加入结尾字符
    server.runid[REDIS_RUN_ID_SIZE] = '\0';
    //设置默认配置文件路径
    server.configfile = NULL;
    //设置默认服务器周期调用频率
    server.hz = REDIS_DEFAULT_HZ;
    //设置服务器运行架构
    server.arch_bits = (sizeof(long) == 8) ? 64 : 32;
    //设置服务端口号
    server.port = REDIS_SERVERPORT;
    server.tcp_backlog = REDIS_TCP_BACKLOG;
    server.dbnum = REDIS_DEFAULT_DBNUM;
    server.maxidletime = REDIS_MAXIDLETIME;
    server.tcpkeepalive = REDIS_DEFAULT_TCP_KEEPALIVE;
    server.daemonize = REDIS_DEFAULT_DAEMONIZE;
     //将默认值复制了一份，注意strdup并不会free空间，但是这里只调用一次
    server.pidfile = strdup(REDIS_DEFAULT_PID_FILE);
    server.maxmemory = REDIS_DEFAULT_MAXMEMORY;
    server.shutdown_asap = 0;

    //初始化LRU时间
    server.lruclock = getLRUClock();

    server.commands = dictCreate(&commandTableDictType, NULL);
}

/**
 * 根据redis.c顶部定义的命令列表，创建命令表
 */ 
void populateCommandTable(void){
    //获取command的数量
    int numcount = (sizeof(redisCommandTable)) / (sizeof(struct redisCommand));
    for (int i = 0; i < numcount; i++){
        struct redisCommand *c = redisCommandTable + i;
        char *f = c->sflags;    //取出字符串形式的标志字符串
        while(*f != '\0'){  //遍历字符串，例如rw
            switch(*f){
                case 'w': c->flags |= REDIS_CMD_WRITE; break;
                case 'r': c->flags |= REDIS_CMD_READONLY; break;
                case 'm': c->flags |= REDIS_CMD_DENYOOM; break;
                case 'a': c->flags |= REDIS_CMD_ADMIN; break;
                case 'p': c->flags |= REDIS_CMD_PUBSUB; break;
                case 's': c->flags |= REDIS_CMD_NOSCRIPT; break;
                case 'R': c->flags |= REDIS_CMD_RANDOM; break;
                case 'S': c->flags |= REDIS_CMD_SORT_FOR_SCRIPT; break;
                case 'l': c->flags |= REDIS_CMD_LOADING; break;
                case 't': c->flags |= REDIS_CMD_STALE; break;
                case 'M': c->flags |= REDIS_CMD_SKIP_MONITOR; break;
                case 'k': c->flags |= REDIS_CMD_ASKING; break;
                default: redisPanic("Unsupported command flag"); break;
            }
            f++;
        }
    }
}

int main(int argc, char *argv[]){

}