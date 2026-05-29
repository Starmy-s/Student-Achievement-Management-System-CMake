/**
 * @brief 通用双向链表驱动库
 * @date 2026-05-28
 */

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

 /*
 * @brief 链表节点
 */
typedef struct Node {
	void* data;
	struct Node* prev;
	struct Node* next;
} Node;

/*
* @brief 链表控制头
*/
typedef struct List {
	Node* head;
	Node* tail;
	int size;
} List;

// 链表操作函数声明，需要地址返回地址，不需要返回bool，一定成功不用返回
List* list_create();
bool list_append(List* list, void* data);
bool list_delete(List* list, Node* node);
void list_destory(List* list);

#endif // !LIST_H
