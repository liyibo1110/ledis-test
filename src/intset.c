#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "intset.h"
/**
 * 暂时去掉了所有大小端的转换函数调用，假定系统只支持小端系统（linux或windows）
 */ 

#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

static intset *intsetResize(intset *is, uint32_t len){
    //重新计算intset结构所需要的contents数组内存大小
    size_t size = len * (is->encoding);
    is = realloc(is, sizeof(intset) + size);
    return is;
}

/**
 * 根据传入的int值，返回相应的int自定义类型
 */ 
static uint8_t _intsetValueEncoding(int64_t v){
    if(v > INT32_MAX || v < INT32_MIN){
        return INTSET_ENC_INT64;
    }else if(v > INT16_MAX || v < INT16_MIN){
        return INTSET_ENC_INT32;
    }else{
        return INTSET_ENC_INT16;
    }
}

static int64_t _intsetGetEncoding(intset *is, int pos, uint8_t encoding){
    int64_t v64;
    int32_t v32;
    int16_t v16;

    if(encoding == INTSET_ENC_INT64){
        memcpy(&v64, ((int64_t*)is->contents)+pos, sizeof(v64));
        return v64;
    }else if(encoding == INTSET_ENC_INT32){
        memcpy(&v32, ((int32_t*)is->contents)+pos, sizeof(v32));
        return v32;
    }else{
        memcpy(&v16, ((int16_t*)is->contents)+pos, sizeof(v16));
        return v16;
    }
}

static int64_t _intsetGet(intset *is, int pos){
    return _intsetGetEncoding(is, pos, is->encoding);
}

/**
 * 将value放入pos位置
 */ 
static void _intsetSet(intset *is, uint32_t pos, int64_t value){
    uint32_t encoding = is->encoding;
    if(encoding == INTSET_ENC_INT64){
        ((int64_t *)is->contents)[pos] = value;
    }else if(encoding == INTSET_ENC_INT32){
        ((int32_t *)is->contents)[pos] = value;
    }else{
        ((int16_t *)is->contents)[pos] = value;
    }

}

/**
 * 寻找value在is中是否存在
 * 找到value则返回1，并将pos设为相同值的位置
 * 找不到则返回0，并将pos设为要insert的位置
 */ 
static uint8_t intsetSearch(intset *is, int64_t value, uint32_t *pos){
    
    //如果intset为空，则直接将pos置为首尾
    if(is->length == 0){
        if(pos){
            *pos = 0;
        }
        return 1;
    }else{
        if(value > _intsetGet(is, is->length-1)){
            if(pos){
                *pos = is->length;
            }
            return 1;
        }else if(value < _intsetGet(is, 0)){
            if(pos){
                *pos = 0;
            }
            return 1;
        }
    }

    int min = 0;
    int max = is->length - 1;
    int mid = -1;
    int64_t current = -1;

    //到这里说既不为空，又不是在首尾，需要进行二分查找（非递归版本）
    while(max>=min){
        mid = (max + min) / 2;
        current = _intsetGet(is, mid);
        if(current > value){    //中值比value还要大，则value属于左半侧
            max = mid - 1;
        }else if(current < value){  //中值比value还要小，则value属于右半侧
            min = mid + 1;
        }else{
            break;
        }
    }

    //已经出来了，要么current正好就是value，要么就是压根就没有
    if(current == value){
        if(pos){
            *pos = mid; //跳出来后的mid就是最后的位置
        }
        return 1;
    }else{
        if(pos){
            *pos = min; //跳出来后的min就是要insert的位置
        }
        return 0;
    }
}

/**
 * 根据value的encoding，先将intset升级到相应的encoding
 * 然后再将value新增进去
 */ 
static intset *intsetUpgradeAndAdd(intset *is, int64_t value){
    uint32_t encoding = is->encoding;   //记录旧的encoding
    uint8_t valueEncoding = _intsetValueEncoding(value);
    int length = is->length;    //记录旧intset的长度
    int prepend = value < 0 ? 1 : 0;
    is->encoding = valueEncoding;   //重要，必须先修改intset的encoding
    is = intsetResize(is, length + 1);  //改变了encoding，resize后结构发生了大变化
    //开始移动元素，机制有点复杂
    while(length--){
        //注意最后传入的encoding是旧的
        //根据prepend的不同，决定是把最后空位留出来，还是首位空位留出来
        _intsetSet(is, length + prepend, _intsetGetEncoding(is, length, encoding));
    }
    if(prepend){    //写到首位
        _intsetSet(is, 0, value);
    }else{
        _intsetSet(is, is->length, value);
    }
    //长度+1
    is->length++;
    return is;
}

/**
 * 将intset中指定的长度，从from移到to位置
 * 注意from也可以大于to，这样就是往左整体移动了
 * 所以函数名应该叫intsetMove更合适
 */ 
static void intsetMoveTail(intset *is, uint32_t from, uint32_t to){
    //要移动的元素个数，例如一共3个，要从第2个开始移动（即3-1=2）最终要一起移动2个元素
    uint32_t bytes = is->length - from;
    uint32_t encoding = is->encoding;
    void *src, *dest;
    if(encoding == INTSET_ENC_INT64){
        src = ((int64_t*)is->contents) + from;
        dest = ((int64_t*)is->contents) + to;
        bytes = bytes * sizeof(int64_t);
    }else if(encoding == INTSET_ENC_INT32){
        src = ((int32_t*)is->contents) + from;
        dest = ((int32_t*)is->contents) + to;
        bytes = bytes * sizeof(int32_t);
    }else{
        src = ((int16_t*)is->contents) + from;
        dest = ((int16_t*)is->contents) + to;
        bytes = bytes * sizeof(int16_t);
    }
    /**
     * 最后要进行移动，注意是整体内容的移动（并不是一个元素一个元素移动）
     * 注意使用了memmove而不是memcpy，前者会判断src、dest是否有重叠区域并处理
     */ 
    memmove(dest, src, bytes);
}

/**
 * 为整形集合分配内存，以及初始化字段
 * 注意数组没有初始化
 */ 
intset *intsetNew(void){
    intset *is = malloc(sizeof(intset));
    is->encoding = INTSET_ENC_INT16;
    is->length = 0;
    return is;
}

/**
 * 将元素value插入整数集合
 * 传入的success将保存插入结果
 * 为1说明成功，如元素已存在，则返回0
 */ 
intset *intsetAdd(intset *is, int64_t value, int8_t *success){
    uint8_t encoding = _intsetValueEncoding(value);
    uint32_t pos;

    if(success){    //传来的success不为NULL，则默认为成功insert
        *success = 1;
    }

    if(encoding > is->encoding){
        //如果新值的encoding大于原整数集合的encoding，则先将intset升级（写value也由这个函数负责）
        return intsetUpgradeAndAdd(is, value);
    }else{  //否则说明新值的编码符合原有的
        //根据value查找是否值已存在，同时在内部更新pos变量的值
        if(intsetSearch(is, value, &pos)){
            if(success){
                *success = 0;
            }
            return is;
        }
        //准备insert，先要扩展内部数组
        is = intsetResize(is, is->length + 1);
        //如果pos不是末位，则要将右侧的元素向右移一位
        if(pos < is->length){
            intsetMoveTail(is, pos, pos+1);
        }
        //正式insert
        _intsetSet(is, pos, value);
        //length加1
        is->length++;
        return is;
    }
}

/**
 * 删除一个元素，success来表示操作结果
 * 为0则表示值不存在，为1表示删除成功
 */ 
intset *intsetRemove(intset *is, int64_t value, int8_t *success){

    uint8_t encoding = _intsetValueEncoding(value);
    uint32_t pos;
    if(success){
        //假定删除失败
        *success = 0;
    }

    /**
     * 如果encoding大于原始的encoding，说明肯定不存在，直接当做找不到
     */ 
    if((encoding <= is->encoding) && intsetSearch(is, value, &pos)){
        //如果要删除元素不是最后一位，则直接把后面的所有数据前移1位，相当于是覆盖要删除的值
        if(pos < is->length-1){
            intsetMoveTail(is, pos+1, pos);
        }
        //重新调整空间
        intsetResize(is, is->length-1);
        is->length--;

        if(success){ 
            *success = 1;
        }
    }

    return is;
}

/**
 * 查找value在intset中是否存在
 * 存在则返回1，否则返回0
 */ 
uint8_t intsetFind(intset *is, int64_t value){
    uint8_t encoding = _intsetValueEncoding(value);
    //encoding大于原intset的encoding说明一定不存在，然后再尝试查找
    return encoding <= is->encoding && intsetSearch(is, value, NULL);
}

/**
 * 随机返回intset里面的值
 */ 
int64_t intsetRandom(intset *is){
    return _intsetGet(is, rand() % is->length);
}

uint32_t intsetLen(intset *is){
    return is->length;
}

/**
 * 返回一个intset的总大小（结构本身大小+所有数组元素的总大小）
 */ 
size_t intsetBlobLen(intset *is){
    return (sizeof(intset)) + is->length * is->encoding;
}

int main(void){
    int a[5] = {1,2,3,4,5};
    int *ptr = a;
    for (int i = 0; i < 5;i++){
        printf("%d", ptr++);   
        printf("\n");   
    }
    getchar();
    return 0;   
}