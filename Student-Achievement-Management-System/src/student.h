/**
 * @brief 学生成绩管理系统的核心数据结构
 * @date 2026-05-26
 */

#ifndef STUDENT_H
#define STUDENT_H

#include "list.h"
#define MAX_ID_LEN   20
#define MAX_NAME_LEN 50

 /*
 * @brief 学科枚举类型
 * @field CHINESE 语文
 * @field MATH 数学
 * @field ENGLIFH 英语
 * @field SUBJECT_COUNT 科目数量
 */
enum Subjects {
	CHINESE = 0,
	MATH,
	ENGLISH,
	SUBJECT_COUNT
};

/*
* @brief 学生信息结构体
* @field id 学号
* @field name 姓名
* @field score 成绩数组，包含3门课程的成绩
*/
typedef struct {
	// 学号
	char id[MAX_ID_LEN];
	char name[MAX_NAME_LEN];
	int scores[SUBJECT_COUNT];

} Student;

// 学生信息业务逻辑函数声明，需要地址返回地址，不需要返回bool，一定成功不用返回
bool student_add(List* list, const char* id, const char* name, int scores[]);
bool student_query_by_id(List* list, const char* id);
bool student_delete_by_id(List* list, const char* id);
int student_query_by_name(List* list, const char* name);
int student_delete_by_name(List* list, const char* name);
void student_print_all(List* list);
void student_clear_all_data(List* list);

#endif // !STUDENT_H