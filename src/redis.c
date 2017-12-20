#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "redis.h"
#include "util.h"

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
 * 工具函数实现
 */ 
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
 * 低级API，将msg输出文件或者标准输出（根据配置）
 */ 
void redisLogRaw(const char *msg){
    FILE *fp;
    int log_to_stdout = (server.logfile[0] == '\0');
    //要么输出文件里，要么输出stdout流
    fp = log_to_stdout ? stdout : fopen(server.logfile, "a");
    if(!fp){    //文件指针不存在直接返回
        return;
    }
    fprintf(fp, "%s", msg);
    fflush(fp);
    if(!log_to_stdout){ //不是输出stdout还得关闭文件
        fclose(fp);
    }
}

/**
 * 以printf的方式输出log，底层调用redisLogRaw函数
 * 使用了可变参数宏
 */ 
void redisLog(const char *fmt, ...){
    va_list ap; //声明可变参数容器
    char msg[REDIS_MAX_LOGMSG_LEN];
    va_start(ap, fmt);  //把可变参数搞进ap容器
    vsnprintf(msg, sizeof(msg), fmt, ap);   //直接使用可变参数版的snprintf函数
    va_end(ap); //还要清理ap容器
    redisLogRaw(msg); //msg里面有值了，最终输出log
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

    server.logfile = strdup(REDIS_DEFAULT_LOGFILE);

    server.daemonize = REDIS_DEFAULT_DAEMONIZE;
     //将默认值复制了一份，注意strdup并不会free空间，但是这里只调用一次
    server.pidfile = strdup(REDIS_DEFAULT_PID_FILE);
    server.maxmemory = REDIS_DEFAULT_MAXMEMORY;
    server.shutdown_asap = 0;

    //初始化LRU时间
    server.lruclock = getLRUClock();

    server.commands = dictCreate(&commandTableDictType, NULL);

    //加载所有命令列表
    populateCommandTable();

    //还要加载5个命令
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
        //最后将命令加到服务器命令字典
        int result = dictAdd(server.commands, sdsnew(c->name), c);
        redisAssert(result == DICT_OK);
    }
}

void version(){
    printf("Redis server v=%s bits=%d\n", REDIS_VERSION, sizeof(long) == 8 ? 64 : 32);
    exit(0);
}

void usage(){
    fprintf(stdout, "Usage: ./redis-server [/path/to/redis.conf] [options]\n");
    fprintf(stdout, "       ./redis-server - (read config from stdin)\n");
    fprintf(stdout, "       ./redis-server -v or --version\n");
    fprintf(stdout, "       ./redis-server -h or --help\n");
    fprintf(stdout, "Examples:\n");
    fprintf(stdout, "       ./redis-server (run the server with default conf)\n");
    fprintf(stdout, "       ./redis-server /etc/redis/6379.conf\n");
    fprintf(stdout, "       ./redis-server --port 7777\n");
    exit(1);
}

int main(int argc, char *argv[]){

    //初始化服务器
    initServerConfig();

    //检查输入参数
    if(argc >= 2){
        int i = 0;
        sds options = sdsempty();
        char *configfile = NULL;

        //检测输入-v或--version
        if(strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0){
            version();
        }
        //检测输入-h或--help
        if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
            usage();
        }

        //猜测第一个参数是否为配置文件路径
        if(argv[i][0] != '-' || argv[i][1] != '-'){
            configfile = argv[i];
            i++;
        }

        //循环加载后面的参数
        while(i != argc){
            if(argv[i][0] == '-' && argv[i][1] == '-'){
                //参数名,例如--port
                if(sdslen(options)){
                    options = sdscat(options, "\n");    //在上次的最后追加换行符
                    options = sdscat(options, argv[i] + 2);
                    options = sdscat(options, " ");
                }
            }else{
                //参数值，要前后加上双引号，最终形式例如port "1234"\n
                options = sdscatrepr(options, argv[i], strlen(argv[i]));
                options = sdscat(options, " ");
            }
            i++;
        }

        if(configfile){
            //如果指定了配置文件，则要将文件全路径给server对象
            server.configfile = getAbsolutePath(configfile);
        }

        //开始载入配置文件

    }else{
        redisLog("Warning: no config file specified, using the default config.");
    }
}