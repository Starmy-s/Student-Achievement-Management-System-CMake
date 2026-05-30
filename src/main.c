/**
 * @brief 学生成绩管理系统的主程序入口
 * @date 2026-05-26
 */

#ifdef _WIN32
#pragma execution_character_set("utf-8")
#define _CRT_SECURE_NO_WARNINGS

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#endif

#include "list.h"
#include "student.h"
#include "terminal_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>




 /**
  * @brief 清除输入流残留数据
  */
static void clear_buffer() {
	int c;
	// TODO: 如果缓冲区什么都没有，还得手动打一个回车
	while ((c = getchar()) != '\n' && c != EOF);
}


/**
 * @brief 读取整数输入的函数，确保用户输入的是一个有效的整数
 * @param prompt 提示用户输入的字符串
 * @return 读取到的整数值
 * @note 如果用户输入无效，会提示重新输入，直到输入一个有效的整数为止
 */
static int get_int(const char* prompt, int min, int max) {
	// TODO: 输入的是小数怎么办
	int value;
	while (true) {
		printf("%s", prompt);
		if (scanf("%d", &value) == 1) {
			clear_buffer();
			if (value < min) {
				fprintf(stderr, C_ERR "输入无效: 输入的数至少为 %d\n" COLOR_RESET, min);
				continue;
			}
			else if (value > max) {
				fprintf(stderr, C_ERR "输入无效: 输入的数最大为 %d\n" COLOR_RESET, max);
				continue;
			}
			return value;
		}
		fprintf(stderr, C_ERR "输入错误: 请输入一个整数\n" COLOR_RESET);
		clear_buffer();
	}
}



/**
 * @brief 收集一个学生数据的内部辅助函数
 */
static bool collect_student_input(char* out_id, char* out_name, int out_scores[]) {
	printf(C_HINT "请输入学号: " COLOR_RESET);
	// 懒得管了，不是数字就不是数字吧
	if (scanf("%19s", out_id) != 1) {
		clear_buffer();
		return false;
	}
	clear_buffer();

	// 防止误触回车
	while (true) {
		printf(C_HINT "请输入姓名: " COLOR_RESET);
		int res = scanf("%49[^\n]", out_name);
		clear_buffer();
		if (res == 1) break;
		printf(C_ERR "输入错误: 姓名不能为空，请重新输入！\n" COLOR_RESET);
	}

	out_scores[CHINESE] = get_int("请输入语文成绩 [0-100]: ", 0, 100);
	out_scores[MATH] = get_int("请输入数学成绩 [0-100]: ", 0, 100);
	out_scores[ENGLISH] = get_int("请输入英语成绩 [0-100]: ", 0, 100);
	return true;
}


/**
 * @brief 数据初始化函数/添加多个学生数据函数
 */
void create(List* list) {
	// 保证不会出现相同的学号
	int student_count = get_int("请输入要初始化的学生个数: ", 0, INT_MAX);
	char id[MAX_ID_LEN], name[MAX_NAME_LEN];
	int scores[SUBJECT_COUNT];
	for (int i = 0; i < student_count; i++) {
		printf("\n" C_SUBTITLE "--- 录入第 " C_HIGHLIGHT "%d" C_SUBTITLE "/" C_HIGHLIGHT "%d" C_SUBTITLE " 个学生 ---\n" COLOR_RESET, i + 1, student_count);
		if (collect_student_input(id, name, scores)) {
			student_add(list, id, name, scores);
		}
	}
}


/**
 * @brief 添加学生数据的函数，用户输入学生信息并将其添加到链表末尾
 */
void add(List* list) {
	char choice;
	char id[MAX_ID_LEN], name[MAX_NAME_LEN];
	int scores[SUBJECT_COUNT];
	do {
		printf("\n" C_SUBTITLE "--- 开始录入新增学生信息 ---\n" COLOR_RESET);
		if (collect_student_input(id, name, scores)) {
			if (student_add(list, id, name, scores)) {
				printf(C_OK "系统提示: 学生数据添加成功！\n" COLOR_RESET);
			}
		}
		printf(C_HINT "是否继续添加数据？(y/n)" COLOR_RESET);
		if (scanf("%c", &choice) == 1) {
			clear_buffer();
		}
		else {
			printf(C_ERR "输入无效，请重试！\n" COLOR_RESET);
			clear_buffer();
			return;
		}
	} while (choice == 'y' || choice == 'Y');
}


/**
 * @brief 数据删除函数，用户输入学号，找到对应学生并从链表中删除
 */
void del(List* list) {
	if (list == NULL || list->size == 0) {
		printf(C_WARN "链表为空，不能删除数据。\n" COLOR_RESET);
		printf(C_HINT "请先使用初始化功能、新增数据功能或导入数据功能！\n" COLOR_RESET);
		return;
	}
	printf(C_SUBTITLE "数据删除 子菜单\n" COLOR_RESET);
	printf(" " C_HIGHLIGHT "1" COLOR_RESET " 按学号删除\n");
	printf(" " C_HIGHLIGHT "2" COLOR_RESET " 按姓名删除\n");
	printf(" " C_HIGHLIGHT "0" COLOR_RESET " 返回系统主菜单\n");
	int choice = get_int("请输入您的选择: ", 0, 2);

	if (choice == 1) {
		char id[MAX_ID_LEN];
		printf(C_HINT "请输入要删除的学号: " COLOR_RESET);
		if (scanf("%19s", id) != 1) {
			clear_buffer();
			return;
		}
		clear_buffer();
		if (student_delete_by_id(list, id)) {
			printf(C_OK "系统提示: 该学生已被成功删除！\n" COLOR_RESET);
		}
	}
	else if (choice == 2) {
		char name[MAX_NAME_LEN];
		printf(C_HINT "请输入要删除的姓名: " COLOR_RESET);
		if (scanf("%49s", name) != 1) {
			clear_buffer();
			return;
		}
		clear_buffer();
		student_delete_by_name(list, name);
	}
}


/**
 * @brief 数据查找函数，用户输入学号，找到对应学生并显示其信息
 */
void find(List* list) {
	if (list == NULL || list->size == 0) {
		printf(C_WARN "链表为空，不能查找数据。\n" COLOR_RESET);
		printf(C_HINT "请先使用初始化功能、新增数据功能或导入数据功能！\n" COLOR_RESET);
		return;
	}
	printf("\n" C_SUBTITLE "==== 数据查询 子菜单 ====\n" COLOR_RESET);
	printf("  " C_HIGHLIGHT "1" COLOR_RESET " 按学号查询\n");
	printf("  " C_HIGHLIGHT "2" COLOR_RESET " 按姓名查询\n");
	printf("  " C_HIGHLIGHT "0" COLOR_RESET " 返回系统主菜单\n");
	printf(C_SUBTITLE "=========================\n" COLOR_RESET);
	int choice = get_int("请输入您的选择: ", 0, 2);


	if (choice == 1) {
		char id[MAX_ID_LEN];
		printf(C_HINT "请输入要查询的学号: " COLOR_RESET);
		if (scanf("%19s", id) != 1) {
			clear_buffer();
			return;
		}
		clear_buffer();
		student_query_by_id(list, id);
	}
	else if (choice == 2) {
		char name[MAX_NAME_LEN];
		printf(C_HINT "请输入要查询的姓名: " COLOR_RESET);
		if (scanf("%49s", name) != 1) {
			clear_buffer();
			return;
		}
		clear_buffer();
		student_query_by_name(list, name);
	}
}


/**
 * @brief 数据导出函数，将链表中的学生数据写入文件保存
 */
void put(List* list) {
	// TODO: 可以考虑添加文件头
	if (list == NULL || list->size == 0) {
		printf(C_WARN "链表为空，不需要导出数据！\n" COLOR_RESET);
		return;
	}
	FILE* fp = fopen("students.dat", "wb");
	if (fp == NULL) {
		printf(C_ERR "错误: 无法创建或打开数据保存文件！\n" COLOR_RESET);
		return;
	}
	Node* current = list->head->next;
	while (current != list->tail) {
		Student* student = (Student*)current->data;
		fwrite(student, sizeof(Student), 1, fp);
		current = current->next;
	}
	fclose(fp);
	printf(C_OK "数据已成功安全导出至本地 [students.dat] 文件！\n" COLOR_RESET);
	student_clear_all_data(list);
}


/**
 * @brief 数据导入函数，从文件中读取学生数据并构建链表，注意避免重复导入
 */
void get(List* list) {
	if (list == NULL) return;
	if (list->size > 0) {
		printf(C_WARN "警告: 当前内存中有已有数据，导入后原数据将完全丢失!\n" COLOR_RESET);
		printf(C_WARN "是否确认覆盖并继续导入数据？(确认 - y):" COLOR_RESET);
		char choice;
		if (scanf("%c", &choice) == 1) {
			clear_buffer();
		}
		else {
			printf(C_ERR "输入无效，请重试！\n" COLOR_RESET);
			clear_buffer();
			return;
		}
		if (choice == 'y' || choice == 'Y') {
			student_clear_all_data(list);
		}
		else {
			return;
		}
	}
	FILE* fp = fopen("students.dat", "rb");
	if (fp == NULL) {
		printf(C_ERR "错误: 没有找到本地 [students.dat] 数据文件，请先创建并导出数据！\n" COLOR_RESET);
		return;
	}

	printf("\n" C_SUBTITLE "正在安全读取文件并还原链表结构...\n" COLOR_RESET);
	while (1) {
		Student* current_student = (Student*)malloc(sizeof(Student));
		if (current_student == NULL) {
			printf(C_ERR "系统级内存不足！\n" COLOR_RESET);
			fclose(fp);
			return;
		}
		if (fread(current_student, sizeof(Student), 1, fp) != 1) {
			free(current_student);
			break;
		}

		list_append(list, current_student);
	}
	fclose(fp);
	printf(C_OK "成功: 已成功从文件读取并加载了 %d 条历史数据！\n" COLOR_RESET, list->size);
}


/**
 * @brief 数据浏览函数，显示链表中所有学生的信息
 */
void read(List* list) {
	student_print_all(list);
}


/**
 * @brief 退出函数，在退出前询问用户是否需要将数据导入文件，如果需要则将链表中的数据写入文件保存
 */
void my_exit(List* list) {
	if (list != NULL && list->size > 0) {
		char choice;
		printf(C_WARN "检测到当前内存存在未存盘数据，是否在退出前导出到文件？(y/n):" COLOR_RESET);
		if (scanf("%c", &choice) == 1) {
			clear_buffer();
		}
		else {
			printf(C_ERR "输入无效，请重试！\n" COLOR_RESET);
			clear_buffer();
			return;
		}
		if (choice == 'y' || choice == 'Y') {
			put(list);
		}
	}
	if (list != NULL) {
		student_clear_all_data(list);
		list_destory(list);
	}
    // 关闭备用屏幕缓冲区
    //printf("\033[?1049l");
	printf(C_TITLE "\n感谢使用学生成绩管理信息系统，再见！\n" COLOR_RESET);
	exit(0);
}

/*
* @brief 打印主菜单
* @param choice_ptr 功能选择的变量地址，用户输入后会修改该变量的值
*/
void print_menu(List* list, int* choice_ptr)
{
	// 清屏
    printf("\033[3J\033[2J\033[H");
	printf(C_BORDER "========================================\n" COLOR_RESET);
	printf(C_TITLE  "        学生成绩管理信息系统 v2.0        \n" COLOR_RESET);
	printf("       [ 当前在册学生总数: " C_HIGHLIGHT "%-3d" COLOR_RESET " 人 ]      \n", list ? list->size : 0);
	printf(C_BORDER "========================================\n" COLOR_RESET);
	printf("  [" C_HIGHLIGHT "1" COLOR_RESET "] 初始化数据    [" C_HIGHLIGHT "5" COLOR_RESET "] 导出数据        \n");
	printf("  [" C_HIGHLIGHT "2" COLOR_RESET "] 新增数据      [" C_HIGHLIGHT "6" COLOR_RESET "] 导入数据        \n");
	printf("  [" C_HIGHLIGHT "3" COLOR_RESET "] 删除数据      [" C_HIGHLIGHT "7" COLOR_RESET "] 浏览数据        \n");
	printf("  [" C_HIGHLIGHT "4" COLOR_RESET "] 查找数据      [" C_HIGHLIGHT "0" COLOR_RESET "] 退出系统        \n");
	printf(C_BORDER "========================================\n" COLOR_RESET);
	*choice_ptr = get_int(C_HINT "请选择功能编号 [0-7]: " COLOR_RESET, 0, 7);
}

/*
* @brief 主循环，负责显示菜单并调用对应的功能函数
*/
void mainpage(List* list)
{
    // 开启备用屏幕缓冲区
    //printf("\033[?1049h");
	while (1) {
		int choice;
		print_menu(list, &choice);
		switch (choice)
		{
		case 1:create(list); break;
		case 2:add(list); break;
		case 3:del(list); break;
		case 4:find(list); break;
		case 5:put(list); break;
		case 6:get(list); break;
		case 7:read(list); break;
		case 0:my_exit(list); break;
		default:printf(C_ERR "不是有效的功能，请重新选择\n" COLOR_RESET);
		}
		printf(C_HINT "按回车键继续..." COLOR_RESET);
		if (getchar() != 1) {
			;
		}
	}
}

/*
* @brief 主函数，初始化链表头尾指针，并进入主菜单循环
*/
int main() {
#ifdef _WIN32
    DWORD mode;
    HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(h_stdout, &mode);
    SetConsoleMode(h_stdout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
	List* student_list = list_create();
	if (student_list == NULL) {
		fprintf(stderr, C_ERR "核心故障: 初始化基础数据结构驱动失败！\n" COLOR_RESET);
		return 1;
	}

	mainpage(student_list);
	return 0;
}