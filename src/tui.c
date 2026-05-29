/**
 * @brief 专业 TUI 框架实现：渲染、输入、布局
 * @date 2026-05-29
 */

#include "tui.h"
#include "student.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

/* ========== 菜单数据 ========== */

const char* const MENU_LABELS[MENU_COUNT] = {
	"查看", "添加", "删除", "搜索", "导出", "导入", "退出"
};
const int MENU_KEYS[MENU_COUNT] = { 1, 2, 3, 4, 5, 6, 0 };

/* ========== 平台终端状态保存 ========== */

#ifdef _WIN32
static DWORD g_old_stdin_mode;
static DWORD g_old_stdout_mode;
#else
static struct termios g_old_termios;
#endif

/* ========== 光标控制 ========== */

void tui_goto(int row, int col) {
	printf("\033[%d;%dH", row + 1, col + 1);
}

void tui_hide_cursor(void) {
	printf("\033[?25l");
}

void tui_show_cursor(void) {
	printf("\033[?25h");
}

/* ========== 生命周期 ========== */

void tui_init(void) {
	printf("\033[?1049h");   /* alternate screen */
	printf("\033[2J\033[H");  /* clear */

#ifdef _WIN32
	HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(h_out, &g_old_stdout_mode);
	DWORD mode_out = g_old_stdout_mode;
	mode_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(h_out, mode_out);

	HANDLE h_err = GetStdHandle(STD_ERROR_HANDLE);
	DWORD mode_err;
	GetConsoleMode(h_err, &mode_err);
	SetConsoleMode(h_err, mode_err | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	HANDLE h_in = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(h_in, &g_old_stdin_mode);
	DWORD mode_in = g_old_stdin_mode;
	mode_in &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
	mode_in |= ENABLE_VIRTUAL_TERMINAL_INPUT;
	SetConsoleMode(h_in, mode_in);

	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#else
	struct termios raw;
	tcgetattr(STDIN_FILENO, &g_old_termios);
	raw = g_old_termios;
	raw.c_lflag &= ~(ECHO | ICANON);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif
}

void tui_cleanup(void) {
	printf("\033[?1049l");   /* restore main screen */
	tui_show_cursor();

#ifdef _WIN32
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), g_old_stdin_mode);
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), g_old_stdout_mode);
#else
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_old_termios);
#endif
}

/* ========== 终端尺寸检测 ========== */

bool tui_get_size(TUILayout* layout) {
	int rows = 0, cols = 0;

#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}
#else
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) {
		cols = ws.ws_col;
		rows = ws.ws_row;
	}
#endif

	if (rows < TUI_MIN_ROWS || cols < TUI_MIN_COLS) return false;

	layout->rows = rows;
	layout->cols = cols;
	layout->content_y0 = 3;
	layout->content_y1 = rows - 5;
	layout->content_rows = layout->content_y1 - layout->content_y0 + 1;
	return true;
}

/* ========== 键盘输入 (跨平台 VT 序列) ========== */

KeyEvent tui_read_key(void) {
	KeyEvent ev = { KEY_NONE, 0, -1 };
	unsigned char buf[8];
	int n;

#ifdef _WIN32
	n = _read(STDIN_FILENO, buf, sizeof(buf));
#else
	n = read(STDIN_FILENO, buf, sizeof(buf));
#endif

	if (n <= 0) return ev;

	/* Escape sequence: arrow keys, function keys */
	if (buf[0] == '\033') {
		if (n == 1) { ev.type = KEY_ESC; return ev; }
		if (n >= 3 && buf[1] == '[') {
			switch (buf[2]) {
			case 'A': ev.type = KEY_UP;    return ev;
			case 'B': ev.type = KEY_DOWN;  return ev;
			case 'C': ev.type = KEY_RIGHT; return ev;
			case 'D': ev.type = KEY_LEFT;  return ev;
			case 'H': ev.type = KEY_HOME;  return ev;
			case 'F': ev.type = KEY_END;   return ev;
			case '1': ev.type = (n >= 5 && buf[3] == ';') ? KEY_F1 + (buf[4] - '1') : KEY_NONE; return ev;
			default:  return ev;
			}
		}
		return ev;
	}

	/* Printable / control characters */
	switch (buf[0]) {
	case '\r': case '\n': ev.type = KEY_ENTER; break;
	case 127:  case '\b': ev.type = KEY_BACKSPACE; break;
	case '\t':             ev.type = KEY_TAB; break;
	default:
		ev.type = KEY_CHAR;
		ev.ch = (char)buf[0];
		if (ev.ch >= '0' && ev.ch <= '9') ev.digit = ev.ch - '0';
		break;
	}
	return ev;
}

/* ========== 行输入 ========== */

bool tui_read_line(int row, int col, const char* prompt,
                   char* buffer, int max_len, bool* cancelled) {
	int pos = 0;
	buffer[0] = '\0';
	*cancelled = false;

	while (true) {
		/* Draw prompt + current content */
		tui_goto(row, col);
		printf("\033[K");  /* clear line */
		printf("%s", prompt);
		for (int i = 0; i < pos; i++) putchar(buffer[i]);
		tui_show_cursor();
		fflush(stdout);

		KeyEvent key = tui_read_key();

		if (key.type == KEY_ENTER) {
			buffer[pos] = '\0';
			tui_hide_cursor();
			return true;
		}
		if (key.type == KEY_ESC) {
			*cancelled = true;
			buffer[0] = '\0';
			tui_hide_cursor();
			return false;
		}
		if (key.type == KEY_BACKSPACE) {
			if (pos > 0) pos--;
			continue;
		}
		if (key.type == KEY_CHAR && pos < max_len - 1) {
			if (key.ch >= 32 && key.ch < 127) {  /* printable ASCII */
				buffer[pos++] = key.ch;
			}
		}
	}
}

int tui_read_int(int row, int col, const char* prompt,
                 int min, int max, bool* cancelled) {
	char buf[16];
	while (true) {
		tui_read_line(row, col, prompt, buf, sizeof(buf), cancelled);
		if (*cancelled) return 0;

		char* endptr;
		long val = strtol(buf, &endptr, 10);
		if (*endptr == '\0' && endptr != buf && val >= min && val <= max) {
			return (int)val;
		}
		/* Show error below */
		tui_goto(row + 1, col);
		printf(CLR_ERROR "输入无效: 请输入 %d-%d" CLR_RESET, min, max);
	}
}

/* ========== 面板渲染 ========== */

void tui_draw_borders(const TUILayout* layout) {
	int w = layout->cols;
	int h = layout->rows;

	/* Top border */
	tui_goto(0, 0);
	printf(CLR_BORDER "┌");
	for (int i = 1; i < w - 1; i++) printf("─");
	printf("┐" CLR_RESET);

	/* Vertical borders for content rows */
	for (int y = 1; y < h - 1; y++) {
		tui_goto(y, 0);
		printf(CLR_BORDER "│" CLR_RESET);
		tui_goto(y, w - 1);
		printf(CLR_BORDER "│" CLR_RESET);
	}

	/* Bottom border */
	tui_goto(h - 1, 0);
	printf(CLR_BORDER "└");
	for (int i = 1; i < w - 1; i++) printf("─");
	printf("┘" CLR_RESET);
}

void tui_draw_header(const TUILayout* layout, int count) {
	int w = layout->cols;

	/* Title line */
	tui_goto(1, 1);
	printf(CLR_TITLE " 学生成绩管理信息系统 v2.0 " CLR_RESET);

	/* Right-side status */
	tui_goto(1, w - 20);
	printf("%s[运行中]%s " ANSI_BRIGHT_WHITE "%-4d" CLR_RESET,
	       CLR_SUCCESS, CLR_RESET, count);
}

void tui_draw_sidebar(const TUILayout* layout, int focus) {
	int x = 1;
	int y = layout->content_y0;

	/* Sidebar label */
	tui_goto(y, x);
	printf(CLR_INFO " 导航菜单 " CLR_RESET);
	y++;

	for (int i = 0; i < MENU_COUNT && y <= layout->content_y1; i++) {
		tui_goto(y, x);
		if (i == focus) {
			printf("\033[7m\033[97m\033[44m");  /* reverse: white on blue */
			printf(" ➜ [%d] %-10s ", MENU_KEYS[i], MENU_LABELS[i]);
			printf(CLR_RESET);
		}
		else {
			printf(CLR_MENU_TEXT "   [%d] %-10s " CLR_RESET,
			       MENU_KEYS[i], MENU_LABELS[i]);
		}
		y++;
	}

	while (y <= layout->content_y1) {
		tui_goto(y, x);
		y++;
	}
}

void tui_draw_dashboard(const TUILayout* layout, List* list, const char* msg) {
	int x = SIDEBAR_WIDTH + 1;
	int y = layout->content_y0;
	int w = layout->cols - x - 1;

	/* Dashboard title bar */
	tui_goto(y, x);
	printf(CLR_BORDER "╔══ " CLR_TITLE "仪表盘" CLR_BORDER " ");
	int remaining = w - 15;
	for (int i = 0; i < remaining; i++) printf("═");
	printf("╗" CLR_RESET);
	y++;

	/* Inner top spacing */
	tui_goto(y, x);
	printf(CLR_BORDER "║" CLR_RESET);
	y++;

	/* Compute stats */
	int total = list ? list->size : 0;
	int pass = 0;
	if (list && list->size > 0) {
		Node* cur = list->head->next;
		while (cur != list->tail) {
			Student* s = (Student*)cur->data;
			if (s->scores[CHINESE] >= 60 &&
			    s->scores[MATH] >= 60 &&
			    s->scores[ENGLISH] >= 60) {
				pass++;
			}
			cur = cur->next;
		}
	}
	int fail = total - pass;

	/* Stat cards (3 cards, 12 cols each) */
	int card_x = x + 4;
	tui_goto(y,   card_x);      printf(CLR_INFO "┌──────────┐" CLR_RESET);
	tui_goto(y+1, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_YELLOW "%-8d" CLR_RESET " " CLR_INFO "│" CLR_RESET, total);
	tui_goto(y+2, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_WHITE "%-8s" CLR_RESET " " CLR_INFO "│" CLR_RESET, "总人数");
	tui_goto(y+3, card_x);      printf(CLR_INFO "└──────────┘" CLR_RESET);

	card_x += 14;
	tui_goto(y,   card_x);      printf(CLR_INFO "┌──────────┐" CLR_RESET);
	tui_goto(y+1, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_GREEN "%-8d" CLR_RESET " " CLR_INFO "│" CLR_RESET, pass);
	tui_goto(y+2, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_WHITE "%-8s" CLR_RESET " " CLR_INFO "│" CLR_RESET, "通过");
	tui_goto(y+3, card_x);      printf(CLR_INFO "└──────────┘" CLR_RESET);

	card_x += 14;
	tui_goto(y,   card_x);      printf(CLR_INFO "┌──────────┐" CLR_RESET);
	tui_goto(y+1, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_RED "%-8d" CLR_RESET " " CLR_INFO "│" CLR_RESET, fail);
	tui_goto(y+2, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_WHITE "%-8s" CLR_RESET " " CLR_INFO "│" CLR_RESET, "未通过");
	tui_goto(y+3, card_x);      printf(CLR_INFO "└──────────┘" CLR_RESET);
	y += 5;

	/* Message area */
	if (msg && msg[0]) {
		tui_goto(y, x);
		printf(CLR_BORDER "║" CLR_RESET "  " CLR_WARNING "%s" CLR_RESET, msg);
		y++;
	}

	/* Spacing before table */
	tui_goto(y, x);
	printf(CLR_BORDER "║" CLR_RESET);
	y++;

	/* Student table */
	int table_w = w - 4;
	tui_goto(y, x);
	printf(CLR_BORDER "║" CLR_RESET " " CLR_INFO "┌");
	for (int i = 0; i < table_w - 2; i++) printf("─");
	printf("┐" CLR_RESET);
	y++;

	/* Table header */
	tui_goto(y, x);
	printf(CLR_BORDER "║" CLR_RESET " " CLR_INFO "│" CLR_RESET
	       " " ANSI_BRIGHT_BLUE "%-5s %-8s %4s %4s %4s %4s" CLR_RESET " " CLR_INFO "│" CLR_RESET,
	       "学号", "姓名", "语文", "数学", "英语", "总分");
	y++;

	/* Table data */
	int max_rows = layout->content_y1 - y - 2;
	if (max_rows < 0) max_rows = 0;

	if (list && list->size > 0) {
		Node* cur = list->head->next;
		int row_i = 0;
		while (cur != list->tail && row_i < max_rows) {
			Student* s = (Student*)cur->data;
			int tot = s->scores[CHINESE] + s->scores[MATH] + s->scores[ENGLISH];
			tui_goto(y, x);
			printf(CLR_BORDER "║" CLR_RESET " " CLR_INFO "│" CLR_RESET
			       " " ANSI_BRIGHT_WHITE "%-5s %-8s" CLR_RESET, s->id, s->name);
			for (int sj = 0; sj < SUBJECT_COUNT; sj++) {
				const char* sc = s->scores[sj] >= 90 ? CLR_SCORE_HIGH :
				                 s->scores[sj] >= 60 ? CLR_SCORE_MED :
				                                       CLR_SCORE_LOW;
				printf(" %s%4d" CLR_RESET, sc, s->scores[sj]);
			}
			printf(" " CLR_INFO "%-4d" CLR_RESET " " CLR_INFO "│" CLR_RESET, tot);
			y++;
			cur = cur->next;
			row_i++;
		}
		if (cur != list->tail) {
			tui_goto(y, x);
			printf(CLR_BORDER "║" CLR_RESET " " CLR_INFO "│" CLR_RESET
			       " " CLR_INFO "    ... 还有 %d 条 ..." CLR_RESET "                     " CLR_INFO "│" CLR_RESET,
			       list->size - row_i);
			y++;
		}
	}
	else {
		tui_goto(y, x);
		printf(CLR_BORDER "║" CLR_RESET " " CLR_INFO "│" CLR_RESET
		       " " CLR_INFO "(暂无数据)" CLR_RESET "                                  " CLR_INFO "│" CLR_RESET);
		y++;
	}

	/* Table bottom */
	tui_goto(y, x);
	printf(CLR_BORDER "║" CLR_RESET " " CLR_INFO "└");
	for (int i = 0; i < table_w - 2; i++) printf("─");
	printf("┘" CLR_RESET);
	y++;

	/* Dashboard bottom */
	tui_goto(y, x);
	printf(CLR_BORDER "╚");
	for (int i = 0; i < w - 2; i++) printf("═");
	printf("╝" CLR_RESET);
	y++;

	/* Clear remaining rows */
	while (y <= layout->content_y1) {
		tui_goto(y, x);
		printf("\033[K");
		y++;
	}
}

void tui_draw_operation_frame(const TUILayout* layout, const char* title) {
	int x = SIDEBAR_WIDTH + 1;
	int y = layout->content_y0;
	int w = layout->cols - x - 1;

	tui_goto(y, x);
	printf(CLR_BORDER "╔══ " CLR_TITLE "%s" CLR_BORDER, title);
	int remaining = w - 6 - (int)strlen(title);
	for (int i = 0; i < remaining; i++) printf("═");
	printf("╗" CLR_RESET);
	y++;

	while (y < layout->content_y1) {
		tui_goto(y, x);
		printf(CLR_BORDER "║" CLR_RESET);
		printf("\033[K");
		y++;
	}

	tui_goto(y, x);
	printf(CLR_BORDER "╚");
	for (int i = 0; i < w - 2; i++) printf("═");
	printf("╝" CLR_RESET);
}

void tui_draw_choice_menu(const TUILayout* layout, int y_off,
                          const char* items[], int count) {
	int cx = SIDEBAR_WIDTH + 4;
	int cy = layout->content_y0 + y_off;

	for (int i = 0; i < count; i++) {
		tui_goto(cy, cx);
		printf(CLR_MENU_NUM "[%d]" CLR_RESET " " ANSI_BRIGHT_WHITE "%s" CLR_RESET,
		       i + 1, items[i]);
		cy++;
	}
	tui_goto(cy, cx);
	printf(CLR_INFO "[ESC] 返回" CLR_RESET);
}

void tui_draw_confirm(const TUILayout* layout, const char* message) {
	tui_draw_operation_frame(layout, "确认");
	int cy = layout->content_y0 + 3;
	int cx = SIDEBAR_WIDTH + 4;
	tui_goto(cy, cx);
	printf(CLR_WARNING "%s" CLR_RESET, message);
}

void tui_draw_footer(const TUILayout* layout, const char* prompt) {
	int f_row = layout->rows - 4;
	int w = layout->cols;

	/* Footer divider */
	tui_goto(f_row, 0);
	printf(CLR_BORDER "├");
	for (int i = 1; i < SIDEBAR_WIDTH; i++) printf("─");
	printf("┴");
	for (int i = SIDEBAR_WIDTH + 1; i < w - 1; i++) printf("─");
	printf("┤" CLR_RESET);
	f_row++;

	/* Shortcut bar */
	tui_goto(f_row, 1);
	printf("\033[100m");  /* dark gray background */
	printf(" " ANSI_BRIGHT_YELLOW "[1]" ANSI_BRIGHT_WHITE "查看 "
	       ANSI_BRIGHT_YELLOW "[2]" ANSI_BRIGHT_WHITE "添加 "
	       ANSI_BRIGHT_YELLOW "[3]" ANSI_BRIGHT_WHITE "删除 "
	       ANSI_BRIGHT_YELLOW "[4]" ANSI_BRIGHT_WHITE "搜索 "
	       ANSI_BRIGHT_YELLOW "[5]" ANSI_BRIGHT_WHITE "导出 "
	       ANSI_BRIGHT_YELLOW "[6]" ANSI_BRIGHT_WHITE "导入 "
	       ANSI_BRIGHT_YELLOW "[0]" ANSI_BRIGHT_WHITE "退出 ");
	printf(CLR_RESET);
	f_row++;

	/* Input prompt */
	tui_goto(f_row, 1);
	printf(CLR_INFO ">" CLR_RESET " " ANSI_BRIGHT_GREEN "%s" CLR_RESET,
	       prompt ? prompt : "");
}
