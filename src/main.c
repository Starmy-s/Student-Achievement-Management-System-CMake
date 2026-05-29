/**
 * @brief 学生成绩管理系统 — TUI 主程序
 * @date 2026-05-29
 */

#include "tui.h"
#include "student.h"
#include "list.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ========== 操作状态 ========== */

typedef enum {
	OP_NONE = 0,
	OP_ADD_FORM,
	OP_DEL_CHOOSE,
	OP_DEL_ENTER_ID,
	OP_DEL_ENTER_NAME,
	OP_SRCH_CHOOSE,
	OP_SRCH_ENTER_ID,
	OP_SRCH_ENTER_NAME,
	OP_EXPORT_DONE,
	OP_IMPORT_CONFIRM,
	OP_IMPORT_DOING,
	OP_EXIT_CONFIRM,
} OpState;

/* ========== 应用状态 ========== */

typedef struct {
	List*   list;
	int     focus;
	bool    running;
	OpState op;
	char    msg[256];
	int     msg_frames;
} AppState;

/* ========== 前向声明 ========== */

static void draw_content(const TUILayout* L, AppState* S);
static void nav_key(AppState* S, KeyEvent key);
static void op_key(AppState* S, KeyEvent key);
static const char* prompt_for(AppState* S);

/* ========== 操作实现 ========== */

static void op_add(AppState* S, const TUILayout* L) {
	tui_draw_operation_frame(L, "添加学生");
	bool canc = false;
	char id[20] = {0}, name[50] = {0};
	int sc[3];
	int fy = L->content_y0 + 2, fx = SIDEBAR_WIDTH + 4;

	tui_read_line(fy,   fx, "学号:            ", id,   sizeof(id),   &canc);
	if (canc) return;
	tui_read_line(fy+1, fx, "姓名:            ", name, sizeof(name), &canc);
	if (canc) return;
	sc[CHINESE] = tui_read_int(fy+2, fx, "语文成绩 [0-100]: ", 0, 100, &canc);
	if (canc) return;
	sc[MATH]    = tui_read_int(fy+3, fx, "数学成绩 [0-100]: ", 0, 100, &canc);
	if (canc) return;
	sc[ENGLISH] = tui_read_int(fy+4, fx, "英语成绩 [0-100]: ", 0, 100, &canc);
	if (canc) return;

	if (student_add(S->list, id, name, sc))
		snprintf(S->msg, sizeof(S->msg), "学生 [%s] %s 添加成功。", id, name);
	else
		snprintf(S->msg, sizeof(S->msg), "添加失败：学号 [%s] 已存在。", id);
	S->msg_frames = 2;
}

static void op_del_id(AppState* S, const TUILayout* L) {
	tui_draw_operation_frame(L, "按学号删除");
	bool canc = false; char id[20] = {0};
	tui_read_line(L->content_y0 + 3, SIDEBAR_WIDTH + 4,
	              "请输入学号: ", id, sizeof(id), &canc);
	if (canc) return;
	if (student_delete_by_id(S->list, id))
		snprintf(S->msg, sizeof(S->msg), "已删除学生 [%s]。", id);
	else
		snprintf(S->msg, sizeof(S->msg), "未找到学号为 [%s] 的学生。", id);
	S->msg_frames = 2;
}

static void op_del_name(AppState* S, const TUILayout* L) {
	tui_draw_operation_frame(L, "按姓名删除");
	bool canc = false; char name[50] = {0};
	tui_read_line(L->content_y0 + 3, SIDEBAR_WIDTH + 4,
	              "请输入姓名: ", name, sizeof(name), &canc);
	if (canc) return;
	int n = student_delete_by_name(S->list, name);
	snprintf(S->msg, sizeof(S->msg), "已删除 %d 名姓名为 [%s] 的学生。", n, name);
	S->msg_frames = 2;
}

static void op_srch_id(AppState* S, const TUILayout* L) {
	tui_draw_operation_frame(L, "按学号搜索");
	bool canc = false; char id[20] = {0};
	tui_read_line(L->content_y0 + 3, SIDEBAR_WIDTH + 4,
	              "请输入学号: ", id, sizeof(id), &canc);
	if (canc) return;

	tui_draw_operation_frame(L, "搜索结果");
	Node* cur = S->list->head->next;
	while (cur != S->list->tail) {
		Student* s = (Student*)cur->data;
		if (strcmp(s->id, id) == 0) {
			int fy = L->content_y0 + 2, fx = SIDEBAR_WIDTH + 4;
			int tot = s->scores[0]+s->scores[1]+s->scores[2];
			tui_goto(fy, fx);
			printf(ANSI_BRIGHT_WHITE "学号: %s  姓名: %s" CLR_RESET, s->id, s->name);
			tui_goto(fy+1, fx);
			printf("语文: %d  数学: %d  英语: %d  总分: %d",
			       s->scores[0], s->scores[1], s->scores[2], tot);
			goto wait_key;
		}
		cur = cur->next;
	}
	tui_goto(L->content_y0 + 3, SIDEBAR_WIDTH + 4);
	printf(CLR_ERROR "未找到学号为 [%s] 的学生。" CLR_RESET, id);

wait_key:
	tui_goto(L->rows - 3, 3);
	printf(CLR_INFO "按任意键继续..." CLR_RESET);
	tui_read_key();
}

static void op_srch_name(AppState* S, const TUILayout* L) {
	tui_draw_operation_frame(L, "按姓名搜索");
	bool canc = false; char name[50] = {0};
	tui_read_line(L->content_y0 + 3, SIDEBAR_WIDTH + 4,
	              "请输入姓名: ", name, sizeof(name), &canc);
	if (canc) return;

	tui_draw_operation_frame(L, "搜索结果");
	int cnt = 0, ry = L->content_y0 + 2, rx = SIDEBAR_WIDTH + 4;
	Node* cur = S->list->head->next;
	while (cur != S->list->tail && ry < L->content_y1) {
		Student* s = (Student*)cur->data;
		if (strcmp(s->name, name) == 0) {
			int tot = s->scores[0]+s->scores[1]+s->scores[2];
			tui_goto(ry, rx);
			printf(ANSI_BRIGHT_WHITE "%-5s  %-8s" CLR_RESET
			       "  语文:%-3d  数学:%-3d  英语:%-3d  总分:%-3d",
			       s->id, s->name, s->scores[0], s->scores[1], s->scores[2], tot);
			ry++; cnt++;
		}
		cur = cur->next;
	}
	if (cnt == 0) {
		tui_goto(L->content_y0 + 3, rx);
		printf(CLR_WARNING "未找到姓名为 [%s] 的学生。" CLR_RESET, name);
	}
	tui_goto(L->rows - 3, 3);
	printf(CLR_INFO "共找到 %d 条匹配。按任意键继续..." CLR_RESET, cnt);
	tui_read_key();
}

static void op_export(AppState* S) {
	if (!S->list || S->list->size == 0) {
		snprintf(S->msg, sizeof(S->msg), "没有数据可以导出。");
		S->msg_frames = 2;
		return;
	}
	int n = S->list->size;
	FILE* fp = fopen("students.dat", "wb");
	if (!fp) {
		snprintf(S->msg, sizeof(S->msg), "错误：无法写入文件。");
		S->msg_frames = 2;
		return;
	}
	Node* cur = S->list->head->next;
	while (cur != S->list->tail) { fwrite(cur->data, sizeof(Student), 1, fp); cur = cur->next; }
	fclose(fp);
	student_clear_all_data(S->list);
	snprintf(S->msg, sizeof(S->msg), "已导出 %d 名学生数据到 students.dat。", n);
	S->msg_frames = 2;
}

static void op_import(AppState* S, const TUILayout* L) {
	if (S->list->size > 0) {
		tui_draw_confirm(L, "导入将覆盖现有数据，确认继续？(y/N)");
		KeyEvent key = tui_read_key();
		if (key.type != KEY_CHAR || (key.ch != 'y' && key.ch != 'Y')) return;
		student_clear_all_data(S->list);
	}

	FILE* fp = fopen("students.dat", "rb");
	if (!fp) {
		snprintf(S->msg, sizeof(S->msg), "错误：未找到 students.dat 文件，请先导出数据。");
		S->msg_frames = 2;
		return;
	}

	tui_draw_operation_frame(L, "导入数据");
	tui_goto(L->content_y0 + 3, SIDEBAR_WIDTH + 4);
	printf(CLR_INFO "正在读取 students.dat... " CLR_RESET);
	fflush(stdout);

	static const char* sp = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏";
	int si = 0;
	while (true) {
		Student* s = (Student*)malloc(sizeof(Student));
		if (!s) { fclose(fp); return; }
		if (fread(s, sizeof(Student), 1, fp) != 1) { free(s); break; }
		list_append(S->list, s);
		tui_goto(L->content_y0 + 3, SIDEBAR_WIDTH + 4);
		printf(CLR_INFO "正在读取 students.dat... %c" CLR_RESET, sp[si++ % 10]);
		fflush(stdout);
	}
	fclose(fp);
	snprintf(S->msg, sizeof(S->msg), "成功导入 %d 名学生数据。", S->list->size);
	S->msg_frames = 2;
}

/* ========== 内容绘制 ========== */

static void draw_content(const TUILayout* L, AppState* S) {
	switch (S->op) {
	case OP_NONE:
	case OP_EXPORT_DONE:
		tui_draw_dashboard(L, S->list, S->msg_frames > 0 ? S->msg : NULL);
		return;

	case OP_ADD_FORM:
		/* drawn inside op_add */
		return;

	case OP_DEL_CHOOSE: {
		tui_draw_operation_frame(L, "删除学生");
		const char* its[] = {"按学号删除", "按姓名删除"};
		tui_draw_choice_menu(L, 3, its, 2);
		return;
	}
	case OP_SRCH_CHOOSE: {
		tui_draw_operation_frame(L, "搜索学生");
		const char* its[] = {"按学号搜索", "按姓名搜索"};
		tui_draw_choice_menu(L, 3, its, 2);
		return;
	}
	case OP_DEL_ENTER_ID:
	case OP_DEL_ENTER_NAME:
	case OP_SRCH_ENTER_ID:
	case OP_SRCH_ENTER_NAME:
	case OP_IMPORT_DOING:
		return;  /* drawn in handlers */

	case OP_IMPORT_CONFIRM:
		tui_draw_confirm(L, "导入将覆盖现有数据，确认继续？(y/N)");
		return;

	case OP_EXIT_CONFIRM:
		tui_draw_confirm(L, "确认退出程序？(y/N)");
		return;
	}
}

/* ========== 提示文本 ========== */

static const char* prompt_for(AppState* S) {
	switch (S->op) {
	case OP_DEL_CHOOSE:
	case OP_SRCH_CHOOSE:
		return "按 [1-2] 选择, ESC 返回";
	case OP_IMPORT_CONFIRM:
	case OP_EXIT_CONFIRM:
		return "按 Y 确认, N 取消";
	case OP_EXPORT_DONE:
		return "按任意键继续";
	default:
		return "↑↓ 导航  Enter 选择  ESC 退出";
	}
}

/* ========== 按键分发 (OP_NONE 时) ========== */

static void nav_key(AppState* S, KeyEvent key) {
	switch (key.type) {
	case KEY_UP:
		S->focus = (S->focus - 1 + MENU_COUNT) % MENU_COUNT;
		break;
	case KEY_DOWN:
		S->focus = (S->focus + 1) % MENU_COUNT;
		break;
	case KEY_ENTER:
		switch (S->focus) {
		case 0: break;                           /* VIEW */
		case 1: S->op = OP_ADD_FORM; break;      /* ADD */
		case 2: S->op = OP_DEL_CHOOSE; break;    /* DELETE */
		case 3: S->op = OP_SRCH_CHOOSE; break;   /* SEARCH */
		case 4: op_export(S); S->op = OP_EXPORT_DONE; break;
		case 5: S->op = OP_IMPORT_CONFIRM; break;
		case 6: S->op = OP_EXIT_CONFIRM; break;
		}
		break;
	case KEY_CHAR:
		if (key.digit >= 0) {
			int act = -1;
			for (int i = 0; i < MENU_COUNT; i++) {
				if (MENU_KEYS[i] == key.digit) { S->focus = i; act = i; break; }
			}
			switch (act) {
			case 0: break;
			case 1: S->op = OP_ADD_FORM; break;
			case 2: S->op = OP_DEL_CHOOSE; break;
			case 3: S->op = OP_SRCH_CHOOSE; break;
			case 4: op_export(S); S->op = OP_EXPORT_DONE; break;
			case 5: S->op = OP_IMPORT_CONFIRM; break;
			case 6: S->op = OP_EXIT_CONFIRM; break;
			}
		} else if (key.ch == 'q' || key.ch == 'Q') {
			S->op = OP_EXIT_CONFIRM;
		}
		break;
	case KEY_ESC:
		S->op = OP_EXIT_CONFIRM;
		break;
	default: break;
	}
}

/* ========== 按键分发 (操作模式) ========== */

static void op_key(AppState* S, KeyEvent key) {
	switch (S->op) {
	case OP_DEL_CHOOSE:
		if (key.type == KEY_CHAR && key.ch == '1')      S->op = OP_DEL_ENTER_ID;
		else if (key.type == KEY_CHAR && key.ch == '2') S->op = OP_DEL_ENTER_NAME;
		else if (key.type == KEY_ESC)                   S->op = OP_NONE;
		break;

	case OP_SRCH_CHOOSE:
		if (key.type == KEY_CHAR && key.ch == '1')      S->op = OP_SRCH_ENTER_ID;
		else if (key.type == KEY_CHAR && key.ch == '2') S->op = OP_SRCH_ENTER_NAME;
		else if (key.type == KEY_ESC)                   S->op = OP_NONE;
		break;

	case OP_IMPORT_CONFIRM:
		if (key.type == KEY_CHAR && (key.ch == 'y' || key.ch == 'Y'))
			S->op = OP_IMPORT_DOING;
		else
			S->op = OP_NONE;
		break;

	case OP_EXIT_CONFIRM:
		if (key.type == KEY_CHAR && (key.ch == 'y' || key.ch == 'Y'))
			S->running = false;
		else
			S->op = OP_NONE;
		break;

	case OP_EXPORT_DONE:
		S->msg[0] = '\0'; S->msg_frames = 0; S->op = OP_NONE;
		break;

	default:
		if (key.type == KEY_ESC) S->op = OP_NONE;
		break;
	}
}

/* ========== 主循环 ========== */

int main(void) {
	List* list = list_create();
	if (!list) {
		fprintf(stderr, CLR_ERROR "致命错误：数据结构初始化失败。\n" CLR_RESET);
		return 1;
	}

	tui_init();

	AppState S = {0};
	S.list = list;
	S.running = true;
	S.focus = 0;

	while (S.running) {
		TUILayout L;
		if (!tui_get_size(&L)) {
			printf("\033[2J\033[H");
			printf(CLR_ERROR "终端窗口太小！至少需要 %dx%d。\n" CLR_RESET,
			       TUI_MIN_COLS, TUI_MIN_ROWS);
			printf("按任意键退出...");
			tui_read_key();
			break;
		}

		/* ---- handle operations that need layout (modal) ---- */
		bool did_modal = false;
		switch (S.op) {
		case OP_ADD_FORM:
			op_add(&S, &L); S.op = OP_EXPORT_DONE; did_modal = true; break;
		case OP_DEL_ENTER_ID:
			op_del_id(&S, &L); S.op = OP_EXPORT_DONE; did_modal = true; break;
		case OP_DEL_ENTER_NAME:
			op_del_name(&S, &L); S.op = OP_EXPORT_DONE; did_modal = true; break;
		case OP_SRCH_ENTER_ID:
			op_srch_id(&S, &L); S.op = OP_NONE; did_modal = true; break;
		case OP_SRCH_ENTER_NAME:
			op_srch_name(&S, &L); S.op = OP_NONE; did_modal = true; break;
		case OP_IMPORT_DOING:
			op_import(&S, &L); S.op = OP_EXPORT_DONE; did_modal = true; break;
		default: break;
		}

		/* ---- message timeout ---- */
		if (S.msg_frames > 0) { S.msg_frames--; if (S.msg_frames == 0) S.msg[0] = '\0'; }

		/* ---- DRAW ---- */
		tui_hide_cursor();
		tui_draw_borders(&L);
		tui_draw_header(&L, list->size);
		tui_draw_sidebar(&L, S.focus);
		draw_content(&L, &S);
		tui_draw_footer(&L, prompt_for(&S));
		/* Clear any leftover content below footer */
		tui_goto(L.rows - 1, 0);
		printf("\033[J");
		tui_goto(L.rows - 3, 3);
		tui_show_cursor();
		fflush(stdout);

		/* ---- INPUT (skip if modal just handled everything) ---- */
		if (!did_modal) {
			KeyEvent key = tui_read_key();
			if (S.op == OP_NONE) nav_key(&S, key);
			else                 op_key(&S, key);
		}
	}

	/* ---- cleanup ---- */
	if (list->size > 0) {
		FILE* fp = fopen("students.dat", "wb");
		if (fp) {
			Node* cur = list->head->next;
			while (cur != list->tail) { fwrite(cur->data, sizeof(Student), 1, fp); cur = cur->next; }
			fclose(fp);
		}
	}
	student_clear_all_data(list);
	list_destory(list);
	tui_cleanup();

	printf(CLR_TITLE "\n感谢使用，再见！\n" CLR_RESET);
	return 0;
}
