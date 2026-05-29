/**
 * @brief 通用双向链表驱动库的实现
 * @date 2026-05-28
 */

#include "list.h"
#include <stdlib.h>

 /*
 * @brief 初始化并创建一个空链表控制头
 */
List* list_create() {
	List* list = (List*)malloc(sizeof(List));
	if (list == NULL) return NULL;

	// 哨兵节点
	list->head = (Node*)malloc(sizeof(Node));
	list->tail = (Node*)malloc(sizeof(Node));
	if (list->head == NULL || list->tail == NULL) {
		free(list->head);
		free(list->tail);
		free(list);
		return NULL;
	}
	list->head->data = NULL;
	list->head->prev = NULL;
	list->head->next = list->tail;

	list->tail->data = NULL;
	list->tail->prev = list->head;
	list->tail->next = NULL;

	list->size = 0;
	return list;
}

/*
* @brief 在链表末尾追加节点
*/
bool list_append(List* list, void* data) {
	if (list == NULL) return false;

	Node* new_node = (Node*)malloc(sizeof(Node));
	if (new_node == NULL) return false;
	new_node->data = data;

	Node* tail_prev = list->tail->prev;
	// 一定要先加新的链，再改已有的链，先改没用的链，再改有用的链
	new_node->next = list->tail;
	new_node->prev = tail_prev;
	tail_prev->next = new_node;
	//tail_prev = new_node; 这么写不对
	list->tail->prev = new_node;

	list->size++;
	return true;
}

/*
* @brief 从链表中安全地删除一个指定的节点
*/
bool list_delete(List* list, Node* node) {
	// 别删哨兵
	if (list == NULL || node == NULL || node == list->head || node == list->tail) {
		return false;
	}

	// 绝对不会出现左值node->prev与node->next为NULL的可能
	node->prev->next = node->next;
	node->next->prev = node->prev;

	free(node);
	list->size--;
	return true;
}

/*
* 销毁链表
*/
void list_destory(List* list) {
	if (list == NULL) return;

	Node* current = list->head->next;
	Node* next_node = NULL;

	while (current != list->tail) {
		next_node = current->next;
		free(current);
		current = next_node;
	}

	free(list->head);
	free(list->tail);
	free(list);
}