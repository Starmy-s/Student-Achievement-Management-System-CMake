/**
 * @brief 学生成绩管理系统的业务逻辑层实现
 * @date 2026-05-28
 */

#ifdef _WIN32

#define _CRT_SECURE_NO_WARNINGS
#endif

#include"list.h"
#include"student.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

 /*
 * @brief 辅助函数：根据学号查找节点
 */
static Node* find_student_node_by_id(List* list, const char* id) {
	// 防御一下
	if (list == NULL || id == NULL) return NULL;

	Node* current = list->head->next;
	while (current != list->tail) {
		Student* student = (Student*)current->data;
		if (strcmp(student->id, id) == 0) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

/**
 * @brief 打印单个学生数据
 * @param stu 指向需要打印的学生结构体常量指针
 */
static void student_printf_single(const Student* stu) {
	// 防御一下
	if (stu == NULL) return;

	int total_score = 0;
	for (int i = 0; i < SUBJECT_COUNT; i++) {
		total_score += stu->scores[i];
	}

	printf("| %-10s | %-10s | 语文: %-3d | 数学: %-3d | 英语: %-3d | 总分: %-3d |\n",
		stu->id, stu->name,
		stu->scores[CHINESE], stu->scores[MATH], stu->scores[ENGLISH],
		total_score);
}

/*
* @brief 添加新学生
* @note 结构体参数较少，直接传参数，不传结构体，方便调用，不用自己拼
*/
bool student_add(List* list, const char* id, const char* name, int scores[]) {
	if (list == NULL || id == NULL || name == NULL) return false;
	// 学号查重
	if (find_student_node_by_id(list, id) != NULL) {
		printf("添加失败：学号 [%s] 已存在！\n", id);
		return false;
	}

	// 打包
	Student* new_student = (Student*)malloc(sizeof(Student));
	if (new_student == NULL) return false;
	// 保证不会越界
	// 原本写法：strcpy(dest, src); 或 strncpy(new_student->name, name, MAX_NAME_LEN - 1);
	// strncpy：如果外部输入的 name 长度刚好大于或等于 MAX_NAME_LEN - 1，它在复制完规定的字节数后，是不会在末尾自动补 \0 的
	// snprintf 会自动算好空间、截断，并且【100% 自动在末尾补 '\0'】，只需要告诉它内存的总大小是多少，它会自己留出 \0
	snprintf(new_student->id, MAX_ID_LEN, "%s", id);
	snprintf(new_student->name, MAX_NAME_LEN, "%s", name);
	for (int i = 0; i < SUBJECT_COUNT; i++) {
		new_student->scores[i] = scores[i];
	}
	if (!list_append(list, new_student)) {
		free(new_student);
		return false;
	}
	return true;
}

/*
* @brief 根据学号查询学生信息并打印
*/
bool student_query_by_id(List* list, const char* id) {
	Node* target_node = find_student_node_by_id(list, id);
	if (target_node == NULL) {
		printf("查询失败：未找到学号为 [%s] 的学生！\n", id);
		return false;
	}

	Student* student = (Student*)target_node->data;
	student_printf_single(student);
	return true;
}

/*
* @brief 根据学号删除学生
*/
bool student_delete_by_id(List* list, const char* id) {
	Node* target_node = find_student_node_by_id(list, id);
	if (target_node == NULL) {
		printf("删除失败：未找到学号为 [%s] 的学生！\n", id);
		return false;
	}
	// 别忘了把结构体里的结构体指针所指的结构体释放，要不然就是垃圾数据了
	free(target_node->data);
	return list_delete(list, target_node);
}

/*
* @brief 根据姓名查找并打印所有匹配的学生
*/
int student_query_by_name(List* list, const char* name) {
	// 这块不特判没有数据，为了格式和下面统一
	if (list == NULL || name == NULL) return 0;

	int match_count = 0;
	Node* current = list->head->next;
	printf("\n--- 开始按姓名 [%s] 搜索 ---\n", name);

	while (current != list->tail) {
		Student* student = (Student*)current->data;

		if (strcmp(student->name, name) == 0) {
			student_printf_single(student);
			match_count++;
		}
		current = current->next;
	}

	if (match_count == 0) {
		printf("提示：未找到任何姓名匹配为 [%s] 的学生。\n", name);
	}
	else {
		printf("搜索完毕，共找到 %d 名学生。\n", match_count);
	}

	return match_count;
}

/*
* @brief 根据姓名删除所有匹配学生
*/
int student_delete_by_name(List* list, const char* name) {
	if (list == NULL || name == NULL || list->size == 0) return 0;

	int delete_count = 0;

	Node* current = list->head->next;
	while (current != list->tail) {
		Node* next_node = current->next;
		Student* student = (Student*)current->data;

		if (strcmp(student->name, name) == 0) {
			free(current->data);
			list_delete(list, current);
			delete_count++;
		}

		current = next_node;
	}

	if (delete_count > 0) {
		printf("成功：已从系统中批量删除 %d 名姓名为 [%s] 的学生。\n", delete_count, name);
	}
	else {
		printf("提示：未找到姓名匹配为 [%s] 的学生，无数据被删除。\n", name);
	}

	return delete_count;
}

/*
* @brief 打印所有学生的成绩
*/
void student_print_all(List* list) {
	if (list == NULL || list->size == 0) {
		printf("系统提示：当前没有任何学生数据\n");
		return;
	}

	printf("\n【学生成绩总表】(总人数: %d)\n", list->size);
	printf("--------------------------------------------------\n");
	printf("%-12s\t%-12s\t%-6s\t%-6s\t%-6s\t%-6s\n", "学号", "姓名", "语文", "数学", "英语", "总分");
	printf("--------------------------------------------------\n");

	Node* current = list->head->next;
	while (current != list->tail) {
		Student* student = (Student*)current->data;
		student_printf_single(student);
		current = current->next;
	}
	printf("--------------------------------------------------\n");
}

/*
* @brief 释放所有学生成绩结构体
* @note 释放链表前必做
*/
void student_clear_all_data(List* list) {
	if (list == NULL) return;
	Node* next_node = NULL;

	Node* current = list->head->next;
	while (current != list->tail) {
		next_node = current->next;
		// 怎么知道释放多大的呢？是操作系统和 malloc/free 这一对搭档在底层通过“暗号”默默完成的
		free(current->data);
		list_delete(list, current);
		current = next_node;
	}
}