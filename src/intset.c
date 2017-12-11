#include "intset.h"
/**
 * 暂时去掉了所有大小端的转换函数调用，假定系统只支持小端系统（linux或windows）
 */ 

#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))


/**
 * 为整形集合分配内存，以及初始化字段
 * 注意数组没有初始化
 */ 
intset *intsetAdd(void){
    intset *is = malloc(sizeof(intset));
    is->encoding = INTSET_ENC_INT16;
    is->length = 0;
    return is;
}

static intset *intsetResize(intset *is, uint32_t len){
    //重新计算intset结构所需要的contents数组内存大小
    size_t size = len * (is->encoding);
    is = realloc(is, sizeof(intset) + size);
    return is;
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