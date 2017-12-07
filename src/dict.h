/*
 * 字典操作状态
 */
#define DICT_OK 0
#define DICT_ERR 1
//初始hash表总量
#define DICT_HT_INITIAL_SIZE 4

#include <stdint.h>
#include <string.h>

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
    size_t (*hashFunction)(const void *key);
    //复制key的函数
    void *(*keyDup)(const void *key);
    //复制val的函数
    void *(*valDup)(const void *obj);
    //对比key的函数
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    //销毁键的函数
    void (*keyDestructor)(void *privdate, const void *key);
    //销毁值的函数
    void (*valDestructor)(void *privdata, const void *obj);
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
    dict *dict;
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

dict *dictCreate(dictType *type, void *privdata);