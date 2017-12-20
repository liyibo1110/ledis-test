#include <stdlib.h>
#include "adlist.h"

/*
 *  新建一个空的链表列表
 */
list *listCreate(void){
    list *list;
    list = malloc(sizeof(*list));
    if(list == NULL){
        return NULL;
    }
    //初始化
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;
    return list;
}

/*
 *  释放指定的链表列表
 */
void listRelease(list *list){
    listNode *current = list->head;
    listNode *next;
    size_t len = list->len;

    while(len--){
        //先把下一个拿出来备用
        next = current->next;
        //如果list绑定了free函数，则要逐一对节点进行free处理
        if(list->free){
            list->free(current->value);
        }

        //释放节点
        free(current);
        //把下一个给当前
        current = next;
    }

    //最后还要释放链表列表
    free(list);
}

/*
 *  将value封装成listNode，放到list链表列表的首位头部
 */
list *listAddNodeHead(list *list, void *value){
    
    //新建listNode节点并分配内存
    listNode *node;
    node = malloc(sizeof(*node));
    if(node == NULL){
        return NULL;
    }
    //赋值value
    node->value = value;

    if(list->len == 0){ //如果list里面还没有任何节点
        list->head = node;
        list->tail = node;
        node->prev = NULL;
        node->next = NULL;
    }else{  //如果list里面已有节点
        node->prev = NULL;
        node->next = list->head;
        //注意赋值顺序，要先处理node本身
        list->head->prev = node;
        list->head = node;
    }
    //长度加1
    list->len = list->len+1;
    return list;
}

/*
 *  将value封装成listNode，放到list链表列表的尾部
 */
list *listAddNodeTail(list *list, void *value){
    
    //新建listNode节点并分配内存
    listNode *node;
    node = malloc(sizeof(*node));
    if(node == NULL){
        return NULL;
    }
    //赋值value
    node->value = value;

    if(list->len == 0){ //如果list里面还没有任何节点
        list->head = node;
        list->tail = node;
        node->prev = NULL;
        node->next = NULL;
    }else{  //如果list里面已有节点
        node->next = NULL;
        node->prev = list->tail;
        //注意赋值顺序，要先处理node本身
        list->tail->next = node;
        list->tail = node;
    }
    //长度加1
    list->len = list->len+1;
    return list;
}

/*
 *  将value封装成listNode，放到list链表列表指定节点前后位置
 *  after为0，则放到oldNode之前，否则放到oldNode之后
 *  实现流程和原版略有不同
 */
list *listInsertNode(list *list, listNode *oldNode, void *value, int after){

    //新建listNode节点并分配内存
    listNode *node;
    node = malloc(sizeof(*node));
    if(node == NULL){
        return NULL;
    }
    //赋值value
    node->value = value;

    if(after){  //之后
        //设定node本身的前后节点
        node->prev = oldNode;
        node->next = oldNode->next;
        //还要设定oldNode的next节点
        if(node->prev != NULL){
            node->prev->next = node;
        }

        //如果oldNode是最后一个节点，则要修改tail指向，否则不操作
        if(list->tail == oldNode){  
            list->tail = node;
        }
    }else{  //之前
        //设定node本身的前后节点
        node->next = oldNode;
        node->prev = oldNode->prev;
        //还要设定oldNode的prev节点
        if(node->next != NULL){
            node->next->prev = node;
        }
        //如果oldNode是第一个节点，则要修改head指向，否则不操作
        if(list->head == oldNode){
            list->head = node;
        }
    }

    //长度加1
    list->len = list->len+1;
    return list;
}

/*
 *  从list中删除指定的node节点
 */
void listDeleteNode(list *list, listNode *node){
    //调整删除节点的前置
    if(node->prev){
        node->prev->next = node->next;
    }else{
        list->head = node->next;
    }

    //调整删除节点的后置
    if(node->next){
        node->next->prev = node->prev;
    }else{
        list->tail = node->prev;
    }

    if(list->free){
        list->free(node);
    }

    free(node);

    list->len = list->len - 1;
}

/*
 *  为指定链表列表，创建一个迭代器
 */
listIterator *listGetIterator(list *list, int direction){
    //新建listNode节点并分配内存
    listIterator *iter;
    iter = malloc(sizeof(*iter));
    if(iter == NULL){
        return NULL;
    }

    //根据不同方向，设置不同的节点作为迭代起始头部
    if(direction == AL_START_HEAD){
        iter->next = list->head;
    }else{
        iter->next = list->tail;
    }
    //还要记录迭代的方向
    iter->direction = direction;
    return iter;
}

/*
 *  释放迭代器，啥也不干就一步free
 */
void listReleaseIterator(listIterator *iter){
    free(iter);
}

/*
 *  重置迭代器并强制设为正向
 */
void listRewindHead(list *list, listIterator *iter){
    iter->next = list->head;
    iter->direction = AL_START_HEAD;
}

/*
 *  重置迭代器并强制设为反向
 */
void listRewindTail(list *list, listIterator *iter){
    iter->next = list->tail;
    iter->direction = AL_START_TAIL;
}

/*
 *  返回指定迭代器当前的节点
 */
listNode *listNext(listIterator *iter){
    listNode *current = iter->next;
    if(current != NULL){
        //取得当前节点后，要修改当前节点，指向下一个节点，防止current被外部操作导致状态变化
        if(iter->direction == AL_START_HEAD){
            iter->next = current->next;
        }else{
            iter->next = current->prev;
        }
    }
    return current;
}

/*
 *  根据给定链表列表，复制生成一个新的副本
 */
list *listDup(list *originList){
    list *newList;
    listNode *node;
    newList = listCreate();
    if(newList == NULL){
        return NULL;
    }

    //先复制处理函数
    newList->dup = originList->dup;
    newList->free = originList->free;
    newList->match = originList->match;

    //开始迭代旧链表列表
    listIterator *iter = listGetIterator(originList, AL_START_HEAD);
    while((node = listNext(iter)) != NULL){
        void *value;
        if(newList->dup){   //如果旧list里定义了dup函数，那么将使用dup来复制值
            value = newList->dup(node->value);
            if(value == NULL){  //如果为NULL，说明dup不正常，直接在清理后退出
                listRelease(newList);
                listReleaseIterator(iter);
                return NULL;
            }
        }else{
            value = node->value;
        }

        //将节点加入新链表，如果返回NULL说明不正常，直接在清理后退出
        if(listAddNodeHead(newList, value) == NULL){
            listRelease(newList);
            listReleaseIterator(iter);
            return NULL;
        }
    }

    //到这里说明复制已成功完成，释放迭代器即可，不要再释放newList了
    listReleaseIterator(iter);
    return newList;
}

/*
 *  查找链表列表，匹配key参数的节点
 *  优先调用match函数来寻找，如果没有match函数定义，就默认使用指针来判断是否匹配
 *  最终如未找到，则返回NULL
 */
listNode *listSearchKey(list *list, void *key){
    listNode *node;
    //开始迭代旧链表列表
    listIterator *iter = listGetIterator(list, AL_START_HEAD);
    while((node = listNext(iter)) != NULL){
        if(list->match){
            if(list->match(node->value, key)){
                //进来意味着找到，清理迭代器然后返回即可
                listReleaseIterator(iter);
                return node;
            }
        }else{
            if(node->value == key){
                //进来意味着找到，清理迭代器然后返回即可
                listReleaseIterator(iter);
                return node;
            }
        }
    }
    //到这里说明都没找到，只能清理迭代器然后返回NULL
    listReleaseIterator(iter);
    return NULL;
}

/*
 *  根据索引index值，返回链表列表中相应位置的节点
 *  index可以为负数，如index为-2，则倒数第二个节点，如再超出范围，则返回NULL
 */
listNode *listIndex(list *list, long index){
    listNode *node;
    if(index >= 0){ //如果index为0或正数，则正向查找
        if(list->head == NULL){ 
            //连head都没有，也就不用找了
            return NULL;
        }else{
            //假定head就是默认元素，然后index递减并向前移动node指向，如果index为0了说明就是当前节点
            node = list->head;
            while(index--){
                node = node->next;
            }
        }
    }else{  //如果index为0或正数，则正向查找
        index = (-index) - 1;   //取反再减1，例如-1则变成0，之后从尾部反向迭代
        if(list->tail == NULL){
            //连tail都没有，也就不用找了
            return NULL;
        }else{
            //和上面是一样的逻辑，只是倒着遍历
            node = list->tail;
            while(index--){
                node = node->prev;
            }
        }
    }

    return node;
}

/*
 *  将链表列表中最后一个节点移除，并添加至头部
 */
void listRotate(list *list){
    if(list->len <= 1){ //如果只有1个节点，则不需要操作
        return;
    }    
    //暂存尾节点，一会儿添加到头部
    listNode *node = list->tail;
    
    //清理尾节点
    list->tail = list->tail->prev;
    list->tail->next = NULL;

    //添加到头部，注意顺序
    node->prev = NULL;
    node->next = list->head;
    list->head->prev = node;
    list->head = node;
}