#include <stdlib.h>

//双端链表节点
typedef struct listNode{
    //前置
    struct listNode *prev;
    //后置
    struct listNode *next;
    //存储值
    void *value;
} listNode;

typedef struct listIterator{
    //当前迭代到的节点
    listNode *next;
    //迭代方向
    int direction;
} listIterator;

//链表封装列表
typedef struct list{
    //头节点指针
    listNode *head;
    //尾节点指针
    listNode *tail;
    //链表节点总数
    size_t len;

    //节点复制函数
    void *(*dup)(void *ptr);
    //节点释放函数
    void (*free)(void *ptr);
    //节点对比函数
    int (*match)(void *ptr, void *key);
} list;

/**
 * 定义函数宏
 */ 
#define listSetDupMethod(l,m) ((l)->dup = (m))
#define listSetFreeMethod(l,m) ((l)->free = (m))
#define listSetMatchMethod(l,m) ((l)->match = (m))
#define listGetDupMethod(l) ((l)->dup)
#define listGetFreeMethod(l) ((l)->free)
#define listGetMatchMethod(l) ((l)->match)

list *listCreate(void);
void listRelease(list *list);
list *listAddNodeHead(list *list, void *value);
list *listAddNodeTail(list *list, void *value);
list *listInsertNode(list *list, listNode *oldNode, void *value, int after);
void listDeleteNode(list *list, listNode *node);

listIterator *listGetIterator(list *list, int direction);
void listReleaseIterator(listIterator *iter);
void listRewindHead(list *list, listIterator *iter);
void listRewindTail(list *list, listIterator *iter);
listNode *listNext(listIterator *iter);
list *listDup(list *originList);
listNode *listSearchKey(list *list, void *key);
listNode *listIndex(list *list, long index);
void listRotate(list *list);

#define AL_START_HEAD 0
#define AL_START_TAIL 1
