#include "dict.h"

//控制字典是否可以自动rehash
static int dict_can_resize = 1;
//当系统启动子进程，负载因子要达到5才可以rehash
static unsigned int dict_force_resize_radio = 5;


/**
 * 先声明private函数
 * 
 */
static void _dictReset(dictht *ht);
static int _dictInit(dict *d, dictType *type, void *privdata);
static int _dictKeyIndex(dict *d, const void *key);
static int _dictExpandIfNeeded(dict *d);
static size_t _dictNextPower(size_t size);
static void _dictRehashStep(dict *d);
static int _dictClear(dict *d, dictht *ht, void(callback)(void *));
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
 * 计算要将key插入到hash表时的索引位置
 * 如果key已经存在，则直接返回-1
 * 如果字典正在rehash，则要去1号表查索引，而不是0号
 * rehash过程中所有的新key都会放到1号表
 */ 
static int _dictKeyIndex(dict *d, const void *key){

    //在这里检查hash表是否需要扩展（注意只有在这里才会检查）
    if(_dictExpandIfNeeded(d) == DICT_ERR){
        return -1;
    }

    //计算hash值
    unsigned int hash = dictHashKey(d, key);
    unsigned int index;
    dictEntry *entry;
    //遍历2个hash表,从0号表开始
    for(unsigned int i = 0; i <= 1; i++){
        index = hash & d->ht[i].sizemask;
        entry = d->ht[i].table[index];
        //遍历这个桶，把每个entry的key都比较，看是否和新key重复
        while(entry){
            if(dictCompareKeys(d, entry->key, key)){
                return -1;
            }
            entry = entry->next;
        }
        //已经遍历完了0号表，如果没有rehash，则不会再遍历1号表，当做没找到重复从而返回桶索引值
        if(!dictIsRehashing(d)){
            break;
        }
    }

    return index;
}

/**
 * 检查hash是否要初始化（注意是在这里才进行的初始化），或者是否需要扩容
 */ 
static int _dictExpandIfNeeded(dict *d){
    //如果已经在rehash了，直接返回0，认为可以尝试insert
    if(dictIsRehashing(d)){
        return DICT_OK;
    }

    //如果是新的hash，则要进行初始化
    if(d->ht[0].size == 0){
        return dictExpand(d, DICT_HT_INITIAL_SIZE);
    }

    /**
     * 如果不是新hash，则检查是否要扩容，需要满足以下2个条件之一：
     * 1.所有桶里已使用节点数达到了桶的总数，并且dict_can_resize开启
     * 2.所有桶里已使用节点数达到了桶的总数，并且比值达到了5倍（dict_can_resize不开启也要扩容）
     */
    if(d->ht[0].used >= d->ht[0].size &&
        (dict_can_resize || (d->ht[0].used / d->ht[0].size) > dict_force_resize_radio)){
            //扩容参考值，就是已使用节点数*2
            return dictExpand(d, d->ht[0].used * 2);
        }

    return DICT_OK;
}

/**
 * 对字典进行单步扩容
 * 在对字典查找和修改的时候被调用，即一边使用一边rehash
 * 但是不能在有安全迭代器的情况下单步rehash，否则会有问题
 */ 
static void _dictRehashStep(dict *d){
    if(d->iterators == 0){
        dictRehash(d, 1);
    }
}

/**
 *  计算出首个大于等于给定size的2的N次方，作为新的size容量值
 *  例如：size是2的N次方，直接返回这个值（如size为4则返回4）
 *  否则往上寻找（如size为14则返回16）
 *  总之传进来的size，就是要被扩展后的基本参考值
 */ 
static size_t _dictNextPower(size_t size){
    unsigned long i = DICT_HT_INITIAL_SIZE;

    if (size >= LONG_MAX) return LONG_MAX;
    while(1) {
        if (i >= size){
            return i;
        }  
        i *= 2;
    }
}

/**
 * 清空给定的hash表：
 * 1.遍历table，获取每个桶的首位entry
 * 2.清理entry的key和val，最后才能free，并且继续清理后面的entry
 * 
 */ 
static int _dictClear(dict *d, dictht *ht, void(callback)(void *)){

    //遍历hash表中的每一个entry，直到遍历size次，或者used为0了
    for (size_t i = 0; i < ht->size && ht->used > 0; i++){

        //这步看不懂，应该是要传入一个回调函数来执行
        if (callback && (i & 65535) == 0){
            callback(d->privdata);
        }

        dictEntry *entry, *nextEntry;
        //如果table为NULL桶，则跳过
        if((entry = ht->table[i]) == NULL){
            continue;
        }
        //开始遍历单个桶
        while(entry){
            //尝试将下一个entry暂存
            nextEntry = entry->next;
            //删除key和val，并释放entry指向的内容，还要used减1
            dictFreeKey(d, entry);
            dictFreeVal(d, entry);
            free(entry);
            ht->used--;
            entry = nextEntry;
        }
    }

    //全部table里面的entry清理完毕，table这个数组本身也要清理
    free(ht->table);
    //最后重置这个hash表的其他字段值
    _dictReset(ht);
    return DICT_OK;
}

/**
 * 对字典的0号表进行初始化（字典刚被使用时）
 * 或者进行1号表的初始化（字典即将进行rehash）
 * 等于是对2个hash表做类似的操作，但是调用时机不同（一个是最初存储，一个是准备扩展）
 */ 
int dictExpand(dict *d, size_t size){
    //如果正在rehash，或者新的size比used还要小（相等于收缩过度了），直接返回1
    if(dictIsRehashing(d) || size < d->ht[0].used){
        return DICT_ERR;
    }

    //算出最终要变化成的size（不会比原来的size小，至少一样）
    size_t newSize = _dictNextPower(size);

    //声明新的hash表，注意这里不是指针了
    dictht n;
    n.size = newSize;
    n.sizemask = newSize - 1;
    n.used = 0;
    //为entry数组分配空间，大小为newSize*单个dictEntry指针大小
    n.table = calloc(1, newSize*(sizeof(dictEntry*)));

    if(d->ht[0].table == NULL){ //说明是初始化字典本身
        d->ht[0] = n;
    }else{  //说明是rehash后初始化已切换走的1号表
        d->ht[1] = n;
        //这里还要把字典的rehashindex置0（即将开始rehash）
        d->rehashindex = 0;
    }
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
 * 删除并释放整个字典
 */ 
void dictRelease(dict *d){
    //先删除2个hash表
    _dictClear(d, &(d->ht[0]), NULL);
    _dictClear(d, &(d->ht[1]), NULL);
    free(d);
}

/**
 * 将字典尝试缩小，小至桶里总节点数和桶数基本一致
 * 如果不允许rehash或者正在rehash，则返回DICT_ERR
 */ 
int dictResize(dict *d){
    if(!dict_can_resize || dictIsRehashing(d)){
        return DICT_ERR;
    }

    //缩成和used一样的值
    int size = d->ht[0].used;
    //小于4就定为4，不能再小了
    if(size < DICT_HT_INITIAL_SIZE){
        size = DICT_HT_INITIAL_SIZE;
    }
    return dictExpand(d, size);
}

/**
 * 尝试将给定键值添加到字典中
 * 如果key已经存在，则返回DICT_ERR
 */ 
int dictAdd(dict *d, void *key, void *val){
    //尝试新增key，如不重复则返回新key的节点
    dictEntry *entry = dictAddRaw(d, key);
    if(!entry){ //已存在则返回ERR
        return DICT_ERR;
    }else{
        //成功新增key后，还要把val也弄进去  
        dictSetVal(d, entry, val);
        return DICT_OK;
    }
}

/**
 * 尝试将给定键值添加到字典中
 * 如果key已经存在，则直接替换，而不是返回DICT_ERR
 * 如果是新添加的，则返回1，否则返回0(注意是写死的数，而不是DICT_OK了)
 */ 
int dictReplace(dict *d, void *key, void *val){

    //先尝试用dictAdd函数来insert
    if(dictAdd(d, key, val) == DICT_OK){
        return 1;
    }else{
        //先把原来的找出来，是肯定可以找到的
        dictEntry *entry = dictFind(d, key);
        //把找到的entry还要复制一个新出来，用来清除，注意不是指针而是实际的值了，相等于复制
        dictEntry auxEntry = *entry;
        //替换val，别的都不变
        dictSetVal(d, entry, val);
        //最后还要释放这个复制出来的entry，这里看不懂暂时
        dictFreeVal(d, (&auxEntry));
        return 0;
    }
}

/**
 * 尝试将key添加到字典中
 * 如果key已经存在，则返回NULL
 */ 
dictEntry *dictAddRaw(dict *d, void *key){
    //只要尝试新增key，就尝试执行单步rehash
    if(dictIsRehashing(d)){
        _dictRehashStep(d);
    }

    //开始根据key找到是否已存在
    int index = _dictKeyIndex(d, key);
    if(index == -1){    //已存在则返回NULL
        return NULL;
    }

    dictht *ht;
    //如果正在rehash，则往1号表增加，否则往0号表增加
    ht = dictIsRehashing(d) ? &d->ht[0] : &d->ht[1];
    //给entry分配空间
    dictEntry *entry;
    entry = malloc(sizeof(*entry));
    //将新的entry插入hash桶的头部
    entry->next = ht->table[index];
    ht->table[index] = entry;
    ht->used++;
    dictSetKey(d, entry, key);

    return entry;
}

/**
 * 根据key寻找entry，实现类似_dictKeyIndex函数
 * 
 */ 
dictEntry *dictFind(dict *d, void *key){

    if(d->ht[0].size == 0){ //hash表为空直接返回NULL
        return NULL;
    }

    if(dictIsRehashing(d)){ //尝试单步rehash
        _dictRehashStep(d);
    }

    //算出hash值
    unsigned int hash = dictHashKey(d, key);
    dictEntry *entry;
    //遍历2个hash表
    for (int i = 0; i <= 1; i++){
        unsigned int index = hash & d->ht[i].sizemask;
        entry = d->ht[i].table[index];
        while(entry){
            if(dictCompareKeys(d, entry->key, key)){
                return entry;
            }
            entry = entry->next;
        }
        if(!dictIsRehashing(d)){    //如果没有rehash中，找一遍就到了，到这里说明已经找不到了
            return NULL;
        }
    }

    //到这里说明正在rehash，并且2个hash表都没找到
    return NULL;
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
    return 0;
}

/**
 * 类似Java的System.currentTimeMillis()方法，返回当前时间戳
 * 注意返回的是long long类型，对应8字节64位，而不是long
 * long只有4字节32位，int在现在的编译器中基本也都是32位的了（早期是16位）
 * gettimeofday函数是由sys/time.h提供的
 */ 
long long timeInMilliseconds(void){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec)*1000) + (tv.tv_usec/1000);
}

/**
 * 在规定的时间内，不停执行包含100步rehash操作
 * 操作完成至少一次，并且到时间，都直接返回已执行完的步数
 */ 
int dictRehashMilliseconds(dict *d, int ms){
    //获取当天时间戳
    long long start = timeInMilliseconds();
    //记录已完成的桶迁移
    int rehashes = 0;
    while(dictRehash(d, 100)){
        rehashes = rehashes + 100;
        if((timeInMilliseconds() - start) > ms){
            break;
        }
    }

    //注意这个返回值，要么为0，要么总为100的倍数，需要后续观察调用方式如何处理这个值的
    return rehashes;
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



int main(){
    //long long time = timeInMilliseconds();
    printf("%lu", _dictNextPower(14));
    getchar();
    return 0;
}