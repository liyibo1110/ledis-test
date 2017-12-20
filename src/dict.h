#ifndef __DICT_H__
#define __DICT_H__

#include <stdint.h>

/*
 * 字典操作状态
 */
#define DICT_OK 0
#define DICT_ERR 1
//初始hash表总量
#define DICT_HT_INITIAL_SIZE 4



typedef struct dictEntry{
    void *key;
    union{
        void *val;
        uint64_t u64;
        int64_t s64;
    } v;
    struct dictEntry *next;
} dictEntry;

typedef struct dictType{
    //计算hash的函数
    unsigned int (*hashFunction)(const void *key);
    //复制key的函数
    void *(*keyDup)(void *privdata, const void *key);
    //复制val的函数
    void *(*valDup)(void *privdata, const void *obj);
    //对比key的函数
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    //销毁键的函数
    void (*keyDestructor)(void *privdata, void *key);
    //销毁值的函数
    void (*valDestructor)(void *privdata, void *obj);
} dictType;

typedef struct dictht{
    dictEntry **table;
    //hash表大小
    size_t size;
    //hash掩码，总是size-1
    size_t sizemask;
    //已有节点总数
    size_t used;
} dictht;

typedef struct dict{
    dictType *type;
    void *privdata;
    dictht ht[2];
    /*
     *  rehash值，-1为不处于rehash状态中，当rehash时，这个值会递增，当整个完成会置回-1
     *  rehash并不是一次就能完成的操作，而是可中断的操作（避免Java的大GC暂停问题），因此需要这个字段来记录状态和进度
     */
    int rehashindex;
    //正在运行的安全迭代器数量
    int iterators;
} dict;

typedef struct dictIterator{
    dict *d;
    /*
     *  table : 正在被迭代的ht表，要么0要么1
     *  index : 迭代器目前正指向的hash表索引位置
     *  safe : 如果为1，程序可以对字典进行修改，如果为0，则只能读取
     */
    int table, index, safe;
    dictEntry *entry, *nextEntry;
    //不明确干啥的
    long long fingerprint;
} dictIterator;

//将有符号整数设为指定entry的值
#define dictSetSignedIntegerVal(entry, val) \
    do {entry->v.s64 = (val);} while(0)
//将无符号整数设为指定entry的值
#define dictSetUnSignedIntegerVal(entry, val) \
    do {entry->v.u64 = (val);} while(0)
//对比2个key
#define dictCompareKeys(d, key1, key2) \
    ((d)->type->keyCompare) ? (d)->type->keyCompare((d)->privdata, key1, key2) : (key1 == key2)
//释放指定字典中entry的key
#define dictFreeKey(d, entry) do{ \
    if((d)->type->keyDestructor){  \
        (d)->type->keyDestructor((d)->privdata, entry->key);   \
    }  \
} while(0) 
//设定指定字典中entry的key
#define dictSetKey(d, entry, _k) do{ \
    if((d)->type->keyDup){  \
        entry->key = (d)->type->keyDup((d)->privdata, (_k));   \
    }else{  \
        entry->key = (_k);  \
    }       \
} while(0)   
//释放指定字典中entry的value
#define dictFreeVal(d, entry) do{ \
    if((d)->type->valDestructor){  \
        (d)->type->valDestructor((d)->privdata, entry->v.val);   \
    }  \
} while(0) 
//设定指定字典中entry的value
#define dictSetVal(d, entry, _v) do{ \
    if((d)->type->valDup){  \
        entry->v.val = (d)->type->valDup((d)->privdata, (_v));   \
    }else{  \
        entry->v.val = (_v);   \
    }   \
} while(0) 

//计算给定key的hash值
#define dictHashKey(d, key) (d)->type->hashFunction(key)
//返回给定entry的key
#define dictGetKey(entry) ((entry)->key)
//返回给定entry的节点值
#define dictGetVal(entry) ((entry)->v.val)
//返回给定entry的有符号整数值
#define dictGetSignedIntegerVal(entry) ((entry)->v.s64)
//返回给定entry的无符号整数值
#define dictGetUnsignedIntegerVal(entry) ((entry)->v.u64)
//返回给定字典大小，注意是2个hash表的和
#define dictSlots(d) ((d)->ht[0].size + (d)->ht[1].size)
//返回给定字典已有节点数量，注意是2个hash表的和
#define dictSize(d) ((d)->ht[0].used + (d)->ht[1].used)
//查看字典是否正在rehash，源码中的参数却叫ht
#define dictIsRehashing(d) ((d)->rehashindex != -1)

dict *dictCreate(dictType *type, void *privdata);
void dictRelease(dict *d);
int dictResize(dict *d);
int dictExpand(dict *d, size_t size);
int dictAdd(dict *d, void *key, void *val);
int dictReplace(dict *d, void *key, void *val);
dictEntry *dictAddRaw(dict *d, void *key);
dictEntry *dictFind(dict *d, void *key);
void *dictFetchValue(dict *d, void *key);
int dictDelete(dict *d, const void *key);
int dictNoFreeDelete(dict *d, const void *key);

int dictRehash(dict *d, int n);
int dictRehashMilliseconds(dict *d, int ms);

dictIterator *dictGetIterator(dict *d);
dictIterator *dictGetSafeIterator(dict *d);
void dictReleaseIterator(dictIterator *iter);
dictEntry *dictNext(dictIterator *iter);

void dictSetHashFunctionSeed(unsigned int initval);
unsigned int dictGetHashFunctionSeed(void);
unsigned int dictGenHashFunction(const void *key, int len);
unsigned int dictGenCaseHashFunction(const unsigned char *buf, int len);

void dictEnableResize(void);
void dictDisableResize(void);

#endif // !__DICT_H___