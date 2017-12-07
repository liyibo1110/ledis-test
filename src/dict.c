#include "dict.h"

/**
 * 重置指定hash表的各项属性
 */
static void _dictReset(dictht *ht){
    ht->size = 0;
    ht->sizemask = 0;
    ht->table = NULL;
    ht->used = 0;
}

/**
 * 将给定的dict初始化
 */ 
static int _dictInit(dict *d, dictType *type, void *privdata){
    //注意要加&，因为ht数组定义的时候并不是指针而是真值
    _dictReset(&d->ht[0]);
    _dictReset(&d->ht[1]);
    //暂不对里面的hash表进行分配内存

    d->type = type;
    d->rehashindex = -1;
    d->privdata = privdata;
    d->iterators = 0;
    return DICT_OK;
}

/**
 * 创建一个新的dict
 */ 
dict *dictCreate(dictType *type, void *privdata){
    dict *d = malloc(sizeof(*d));
    if(d == NULL){
        return NULL;
    }
    _dictInit(d, type, privdata);
    return d;
}

