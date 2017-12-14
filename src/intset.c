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