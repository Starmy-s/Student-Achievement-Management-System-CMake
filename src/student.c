/**
 * @brief 学生成绩管理系统的业务逻辑层实现
 * @date 2026-05-28
 */

#ifdef _WIN32

#define _CRT_SECURE_NO_WARNINGS
#endif

#include"list.h"
#include"student.h"
#include"terminal_colors.h"

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
 * @param op 0->不含标头，1->含标头
 */
static void student_printf_single(const Student* stu, int op) {
	// 防御一下
	if (stu == NULL) return;

	int total_score = 0;
	for (int i = 0; i < SUBJECT_COUNT; i++) {
		total_score += stu->scores[i];
	}

	if(op == 0) {
		printf("%-12s\t%-12s\t%-6d\t%-6d\t%-6d\t%-6d\n",
			stu->id, stu->name,
			stu->scores[CHINESE], stu->scores[MATH], stu->scores[ENGLISH],
			total_score);
	}
	else if (op == 1) {
		printf(C_BORDER "|" COLOR_RESET " " C_LABEL "学号:" COLOR_RESET " " C_VALUE "%-10s" COLOR_RESET " " C_BORDER "|" COLOR_RESET " " C_LABEL "姓名:" COLOR_RESET " " C_VALUE "%-10s" COLOR_RESET " " C_BORDER "|" COLOR_RESET " " C_LABEL "语文:" COLOR_RESET " " C_HIGHLIGHT "%-3d" COLOR_RESET " " C_BORDER "|" COLOR_RESET " " C_LABEL "数学:" COLOR_RESET " " C_HIGHLIGHT "%-3d" COLOR_RESET " " C_BORDER "|" COLOR_RESET " " C_LABEL "英语:" COLOR_RESET " " C_HIGHLIGHT "%-3d" COLOR_RESET " " C_BORDER "|" COLOR_RESET " " C_LABEL "总分:" COLOR_RESET " " C_HIGHLIGHT "%-3d" COLOR_RESET " " C_BORDER "|\n" COLOR_RESET,
			stu->id, stu->name,
			stu->scores[CHINESE], stu->scores[MATH], stu->scores[ENGLISH],
			total_score);
	}
}

/*
* @brief 添加新学生
* @note 结构体参数较少，直接传参数，不传结构体，方便调用，不用自己拼
*/
bool student_add(List* list, const char* id, const char* name, int scores[]) {
	if (list == NULL || id == NULL || name == NULL) return false;
	// 学号查重
	if (find_student_node_by_id(list, id) != NULL) {
		printf(C_ERR "添加失败：学号 [" C_HIGHLIGHT "%s" C_ERR "] 已存在！\n" COLOR_RESET, id);
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
		printf(C_WARN "查询失败：未找到学号为 [" C_HIGHLIGHT "%s" C_WARN "] 的学生！\n" COLOR_RESET, id);
		return false;
	}

	Student* student = (Student*)target_node->data;
	student_printf_single(student, 1);
	return true;
}

/*
* @brief 根据学号删除学生
*/
bool student_delete_by_id(List* list, const char* id) {
	Node* target_node = find_student_node_by_id(list, id);
	if (target_node == NULL) {
		printf(C_WARN "删除失败：未找到学号为 [" C_HIGHLIGHT "%s" C_WARN "] 的学生！\n" COLOR_RESET, id);
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
	printf("\n" C_SUBTITLE "--- 开始按姓名 [" C_HIGHLIGHT "%s" C_SUBTITLE "] 搜索 ---\n" COLOR_RESET, name);

	while (current != list->tail) {
		Student* student = (Student*)current->data;

		if (strcmp(student->name, name) == 0) {
			student_printf_single(student, 1);
			match_count++;
		}
		current = current->next;
	}

	if (match_count == 0) {
		printf(C_WARN "提示：未找到任何姓名匹配为 [" C_HIGHLIGHT "%s" C_WARN "] 的学生。\n" COLOR_RESET, name);
	}
	else {
		printf(C_OK "搜索完毕，共找到 " C_HIGHLIGHT "%d" C_OK " 名学生。\n" COLOR_RESET, match_count);
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
		printf(C_OK "成功：已从系统中批量删除 " C_HIGHLIGHT "%d" C_OK " 名姓名为 [" C_HIGHLIGHT "%s" C_OK "] 的学生。\n" COLOR_RESET, delete_count, name);
	}
	else {
		printf(C_WARN "提示：未找到姓名匹配为 [" C_HIGHLIGHT "%s" C_WARN "] 的学生，无数据被删除。\n" COLOR_RESET, name);
	}

	return delete_count;
}

/*
* @brief 打印所有学生的成绩
*/
void student_print_all(List* list) {
	if (list == NULL || list->size == 0) {
		printf(C_WARN "系统提示：当前没有任何学生数据\n" COLOR_RESET);
		return;
	}

	//printf("\n" C_TITLE "【学生成绩总表】" COLOR_RESET "(总人数: " C_HIGHLIGHT "%d" COLOR_RESET ")\n", list->size);
	//printf(C_BORDER "------------------------------------------------------------\n" COLOR_RESET);
	//printf(C_LABEL "%-12s\t%-12s\t%-6s\t%-6s\t%-6s\t%-6s\n" COLOR_RESET, "学号", "姓名", "语文", "数学", "英语", "总分");
	//printf(C_BORDER "------------------------------------------------------------\n" COLOR_RESET);

	// 添加分页显示，每20条记录提示用户按 Enter 键继续浏览
	Node* current = list->head->next;
	int count = 0;
	while (current != list->tail) {
		if (count % 20 == 0) {
			if (count != 0) {
				printf(C_HINT "提示：已显示 %d 条记录，按 Enter 键继续浏览..." COLOR_RESET, count);
				if (getchar() != '\n') return;
			}
			printf("\n" C_TITLE "【学生成绩总表】" COLOR_RESET "(总人数: " C_HIGHLIGHT "%d" COLOR_RESET ")\n", list->size);
			printf(C_BORDER "------------------------------------------------------------\n" COLOR_RESET);
			printf(C_LABEL "%-12s\t%-12s\t%-6s\t%-6s\t%-6s\t%-6s\n" COLOR_RESET, "学号", "姓名", "语文", "数学", "英语", "总分");
			printf(C_BORDER "------------------------------------------------------------\n" COLOR_RESET);
		}
		Student* student = (Student*)current->data;
		student_printf_single(student, 0);
		current = current->next;
		count++;
	}
	printf(C_BORDER "------------------------------------------------------------\n" COLOR_RESET);
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