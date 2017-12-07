#include "dict.h"

//控制字典是否可以自动rehash
static int dict_can_resize = 1;
//当系统启动子进程，负载因子要达到5才可以rehash
static unsigned int dict_force_resize_radio = 5;

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

/**
 *  执行N步渐进式rehash
 *  返回1表示只移动了一部分hash内容过去
 *  返回0表示全部移动完毕
 */ 
int dictRehash(dict *d, int n){
    //只能在rehash进行中才能执行
    if(!dictIsRehashing(d)){
        return 0;
    }

    while(n--){ //只重复N次操作
        if(d->ht[0].used == 0){ //0号hash表used为0，说明已经全部移动完毕，只剩交换hash表本身了
            //直接释放0号表的节点数组
            free(d->ht[0].table);
            //1号表直接给0号表
            d->ht[0] = d->ht[1];
            //1号表直接重置
            _dictReset(&d->ht[1]);
            //关闭dict的rehash标识
            d->rehashindex = -1;
            return 0;
        }

        //确保rehashidx没有越界，idx最多只会比size小1，永远不可能相等
        assert(d->ht[0].size >(unsigned)d->rehashindex);

        //寻找下一个非空的桶，如果是空桶，则继续寻找
        while(d->ht[0].table[d->rehashindex] == NULL){
            d->rehashindex++;
        }

        dictEntry *entry, *nextEntry;
        //取得要迁移的entry
        entry = d->ht[0].table[d->rehashindex];

        while(entry){   //开始遍历桶中的节点
            nextEntry = entry->next;
            //算出hash值，以及再算出1号表的桶索引值
            size_t h = dictHashKey(d, entry->key) & d->ht[1].sizemask;
            //先将要迁移的entry后驱，指向原来桶里的头元素
            entry->next = d->ht[1].table[h];
            //最后再将迁移的entry放到新桶的头部
            d->ht[1].table[h] = entry;

            //更新计数器
            d->ht[0].used--;
            d->ht[1].used++;

            //处理桶里的下一个节点，直到nextEntry为NULL
            entry = nextEntry;
        }

        //桶中的都迁移走了，要把这个桶也清空，只需要赋值NULL，因为桶里的entry被1号表指向了，不能free
        d->ht[0].table[d->rehashindex] = NULL;
        d->rehashindex++;
    }
}

/**
 * 启动自动rehash
 */ 
void dictEnableResize(void){
    dict_can_resize = 1;
}

/**
 *  关闭自动rehash
 */ 
void dictDisableResize(void){
    dict_can_resize = 0;    
}

