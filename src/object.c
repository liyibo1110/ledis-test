#include "redis.h"
/**
 * 创建一个新的redisObject对象
 */ 
robj *createObject(int type, void *ptr){
    robj *obj = malloc(sizeof(*obj));
    obj->type = type;
    obj->encoding = REDIS_ENCODING_RAW; //默认为普通字符串
    obj->ptr = ptr;
    obj->lru = LRU_CLOCK();
    obj->refcount = 1;  //引用数+1
    return obj;
}

/**
 * 创建一个REDIS_ENCODING_RAW编码的字符对象
 * ptr需要被封装成sds类型
 */ 
robj *createRawStringObject(char *ptr, size_t len){
    return createObject(REDIS_STRING, sdsnewlen(ptr, len));
}

/**
 * 根据len不同，选择不同的String封装实现
 * 目前去掉了Embed紧缩类型的实现，只有raw一种类型
 */ 
robj *createStringObject(char *ptr, size_t len){
    return createRawStringObject(ptr, len);
}

/**
 * 将long long类型的值，转换成字符串，生成redisObject对象
 * 不考虑共享缓存小值，也不使用原版自定义的转换函数
 */ 
robj *createStringObjectFromLongLong(long long value){
    return createObject(REDIS_STRING, sdsfromlonglong(value));
}

/**
 * 将long double类型的值，转换成字符串，生成redisObject对象
 * 原版使用了特殊技巧
 */ 
robj *createStringObjectFromLongDouble(long double value){
    char buf[256];
    //转成17位精度的小数，这种精度可以在大部分机器上被rouning而不改变每一位数值
    int len = snprintf(buf, sizeof(buf), "%.17Lf", value);
    //开始倒着遍历，去掉最后一位的0
    if(strchr(buf, '.') != NULL){
        //将指针指向buf的最后一位
        char *p = buf + len - 1;
        while(*p == '0'){   //如果是0，就将len减1，直到不是0
            p--;
            len--;
        }
        if(*p=='.'){    //如果是小数点，则len再减一
            len--;
        }
    }
    return createStringObject(buf, len);
}

/**
 * 将给定的字符串类型的redisObject对象，复制并返回一个新的
 */ 
robj *dupStringObject(robj *o){

    robj *d;

    //假定type肯定是REDIS_STRING
    switch(o->encoding){
        case REDIS_ENCODING_RAW:{
            return createRawStringObject(o->ptr, sdslen(o->ptr));
        }
        case REDIS_ENCODING_INT:{
            d = createObject(REDIS_STRING, NULL);
            d->encoding = REDIS_ENCODING_INT;
            d->ptr = o->ptr;
            return d;
        }
        default:{
            break;
        }
    }
    //有可能不返回任何值，所以会有警告
}

/**
 * 创建一个链表编码的列表对象
 */ 
robj *createListObject(void){
    list *l = listCreate();
    robj *o = createObject(REDIS_LIST, l);
    listSetFreeMethod(l, decrRefCountVoid);
    o->encoding = REDIS_ENCODING_LINKEDLIST;
    return o;
}

robj *createSetObject(void){
    dict *d = dictCreate(&setDictType, NULL);
    robj *o = createObject(REDIS_SET, d);
    o->encoding = REDIS_ENCODING_HT;
    return o;
}

robj *createIntsetObject(void){
    intset *is = intsetNew();
    robj *o = createObject(REDIS_SET, is);
    o->encoding = REDIS_ENCODING_INTSET;
    return o;
}

robj *createHashObject(void){
    dict *d = dictCreate(&hashDictType, NULL);
    robj *o = createObject(REDIS_HASH, d);
    o->encoding = REDIS_ENCODING_HT;
    return o;
}

/**
 * 给对象的计数器加1
 */ 
void incrRefCount(robj *o){
    o->refcount++;
}

/**
 * 给对象的计数器减1
 * 当refcount变成0时，释放对象
 */ 
void decrRefCount(robj *o){
    if(o->refcount == 1){
        switch(o->type){
            case REDIS_STRING:{
                freeStringObject(o);
                break;
            }
            case REDIS_LIST:{
                freeListObject(o);
                break;
            }
            case REDIS_SET:{
                freeSetObject(o);
                break;
            }
            case REDIS_HASH:{
                freeHashObject(o);
                break;
            }
            case REDIS_ZSET:{
                freeZsetObject(o);
                break;
            }
            default: break;
        }
        //别忘了清理自己
        free(o);
    }else{
        o->refcount--;
    }
}

/**
 * 作用于特定数据结构的释放函数业务封装
 */ 
void decrRefCountVoid(void *o){
    decrRefCount(o);
}

/**
 * 将引用计数器单纯置0，但并不释放
 */ 
robj *resetRefCount(robj *obj){
    obj->refcount = 0;
    return obj;
}

/**
 * 对于String类型的type，如果是RAW类型，则释放sdshdr结构体空间
 */ 
void freeStringObject(robj *o){
    if(o->encoding == REDIS_ENCODING_RAW){
        sdsfree(o->ptr);
    }
}
void freeListObject(robj *o){
    switch(o->encoding){
        case REDIS_ENCODING_LINKEDLIST:{
            listRelease((list *)o->ptr);
            break;
        }
        case REDIS_ENCODING_ZIPLIST:{
            free(o->ptr);
            break;
        }
        default:{

        }
    }
}
void freeSetObject(robj *o){
    switch(o->encoding){
        case REDIS_ENCODING_HT:{
            dictRelease((dict*)o->ptr);
            break;
        }
        case REDIS_ENCODING_INTSET:{
            free(o->ptr);
            break;
        }
    }
}
void freeZsetObject(robj *o){
    //没实现skiplist
}
void freeHashObject(robj *o){
    switch(o->encoding){
        case REDIS_ENCODING_HT:{
            dictRelease((dict*)o->ptr);
            break;
        }
        case REDIS_ENCODING_ZIPLIST:{
            free(o->ptr);
            break;
        }
    }
}

int main(){
    printf("abc");
    getchar();
    return 0;
}