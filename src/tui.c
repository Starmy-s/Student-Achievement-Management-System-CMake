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
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
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

	/* 全缓冲 stdout — 整帧绘制完再一次性刷到终端, 消除闪烁 */
	static char stdout_buf[65536];
	setvbuf(stdout, stdout_buf, _IOFBF, sizeof(stdout_buf));

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
	/* 禁用行缓冲/回显/控制键处理；ReadConsoleInputW 读取原始事件 */
	mode_in &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
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

/* ========== 键盘输入 (跨平台) ========== */

#ifdef _WIN32

KeyEvent tui_read_key(void) {
	KeyEvent ev = { KEY_NONE, 0, -1 };
	HANDLE h_in = GetStdHandle(STD_INPUT_HANDLE);

	while (1) {
		INPUT_RECORD rec;
		DWORD nread;
		if (!ReadConsoleInputW(h_in, &rec, 1, &nread) || nread == 0)
			return ev;

		/* 跳过非键盘事件 (鼠标 / 窗口大小 / 焦点) */
		if (rec.EventType != KEY_EVENT)
			continue;

		KEY_EVENT_RECORD* k = &rec.Event.KeyEvent;

		/* 只响应按下事件, 忽略松开 */
		if (!k->bKeyDown)
			continue;

		WORD  vk   = k->wVirtualKeyCode;
		WCHAR wch  = k->uChar.UnicodeChar;

		/* ---- 方向键 ---- */
		switch (vk) {
		case VK_UP:     ev.type = KEY_UP;    return ev;
		case VK_DOWN:   ev.type = KEY_DOWN;  return ev;
		case VK_LEFT:   ev.type = KEY_LEFT;  return ev;
		case VK_RIGHT:  ev.type = KEY_RIGHT; return ev;

		case VK_RETURN: ev.type = KEY_ENTER;      return ev;
		case VK_ESCAPE: ev.type = KEY_ESC;        return ev;
		case VK_TAB:    ev.type = KEY_TAB;        return ev;
		case VK_BACK:   ev.type = KEY_BACKSPACE;  return ev;
		case VK_HOME:   ev.type = KEY_HOME;       return ev;
		case VK_END:    ev.type = KEY_END;        return ev;

		case VK_F1: case VK_F2: case VK_F3: case VK_F4:
		case VK_F5: case VK_F6: case VK_F7: case VK_F8:
			ev.type = KEY_F1 + (vk - VK_F1);
			return ev;

		case VK_DELETE:
			ev.type = KEY_BACKSPACE;  /* 映射为退格 */
			return ev;

		default: break;
		}

		/* ---- 可打印 ASCII 字符 ---- */
		if (wch >= 32 && wch < 127) {
			ev.type = KEY_CHAR;
			ev.ch   = (char)wch;
			if (ev.ch >= '0' && ev.ch <= '9')
				ev.digit = ev.ch - '0';
			return ev;
		}

		/* ---- Ctrl+字母 (Ctrl+A=1 .. Ctrl+Z=26) ---- */
		if (wch >= 1 && wch <= 26) {
			ev.type = KEY_CHAR;
			ev.ch   = (char)(wch + 'a' - 1);
			return ev;
		}

		/* ---- 回车有时以 \r 出现 (wch == 0 但 vk == VK_RETURN 已处理) ---- */
		if (wch == '\r' || wch == '\n') {
			ev.type = KEY_ENTER;
			return ev;
		}

		/* 无法识别的按键 → 继续等待 */
	}
}

#else /* ========== Unix / POSIX (VT 序列) ========== */

KeyEvent tui_read_key(void) {
	KeyEvent ev = { KEY_NONE, 0, -1 };
	unsigned char buf[8];
	int n;

	n = read(STDIN_FILENO, buf, sizeof(buf));

	if (n == 1 && buf[0] == '\033') {
		fd_set fds;
		struct timeval tv = {0, 15000}; /* 15 ms */
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
			int n2 = read(STDIN_FILENO, buf + n, sizeof(buf) - n);
			if (n2 > 0) n += n2;
		}
	}

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

#endif /* _WIN32 */

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
	for (int i = 1; i < w - 2; i++) printf("─");
	printf("┐" CLR_RESET);

	/* Vertical borders — 右边线只在非内容区画, 内容区的右边线由 main 循环统一补绘 */
	for (int y = 1; y < h - 1; y++) {
		tui_goto(y, 0);
		printf(CLR_BORDER "│" CLR_RESET);
		if (y < layout->content_y0 || y > layout->content_y1) {
			tui_goto(y, w - 2);
			printf(CLR_BORDER "│" CLR_RESET);
		}
	}

	/* Bottom border */
	tui_goto(h - 1, 0);
	printf(CLR_BORDER "└");
	for (int i = 1; i < w - 2; i++) printf("─");
	printf("┘" CLR_RESET "\033[K");
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
	printf(CLR_INFO " 导航菜单 " CLR_RESET "\033[K");
	y++;

	for (int i = 0; i < MENU_COUNT && y <= layout->content_y1; i++) {
		tui_goto(y, x);
		if (i == focus) {
			/* 反色高亮: 白字蓝底 */
			printf("\033[7m\033[97m\033[44m"
			       " ➜ [%d] %-10s " CLR_RESET "\033[K",
			       MENU_KEYS[i], MENU_LABELS[i]);
		}
		else {
			printf(CLR_MENU_TEXT "   [%d] %-10s " CLR_RESET "\033[K",
			       MENU_KEYS[i], MENU_LABELS[i]);
		}
		y++;
	}

	/* 清空侧边栏剩余行 */
	while (y <= layout->content_y1) {
		tui_goto(y, x);
		printf("\033[K");
		y++;
	}
}

void tui_draw_dashboard(const TUILayout* layout, List* list, const char* msg) {
	int x = SIDEBAR_WIDTH + 1;
	int y = layout->content_y0;
	int w = layout->cols - x - 4;  /* 右侧留 3 列: 1 间隙 + 1 外框 + 1 边距 */        /* dashboard 可用宽度 */

	/* ---- Dashboard 标题栏 ---- */
	/* 固定内容: "╔══ "(4) + "仪表盘"(6) + " "(1) + "╗"(1) = 12 */
	tui_goto(y, x);
	printf(CLR_BORDER "╔══ " CLR_TITLE "仪表盘" CLR_BORDER " ");
	int remaining = w - 12;
	if (remaining < 0) remaining = 0;
	for (int i = 0; i < remaining; i++) printf("═");
	printf("╗" CLR_RESET);
	y++;

	/* ---- 内部上边距 ---- */
	tui_goto(y, x);
	printf(CLR_BORDER "║" CLR_RESET "\033[K");
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
	tui_goto(y+1, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_YELLOW "%-8d" CLR_RESET " " CLR_BORDER "│" CLR_RESET, total);
	tui_goto(y+2, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_WHITE "%-11s" CLR_RESET " " CLR_BORDER "│" CLR_RESET, "总人数");
	tui_goto(y+3, card_x);      printf(CLR_INFO "└──────────┘" CLR_RESET);

	card_x += 14;
	tui_goto(y,   card_x);      printf(CLR_INFO "┌──────────┐" CLR_RESET);
	tui_goto(y+1, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_GREEN "%-8d" CLR_RESET " " CLR_BORDER "│" CLR_RESET, pass);
	tui_goto(y+2, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_WHITE "%-10s" CLR_RESET " " CLR_BORDER "│" CLR_RESET, "通过");
	tui_goto(y+3, card_x);      printf(CLR_INFO "└──────────┘" CLR_RESET);

	card_x += 14;
	tui_goto(y,   card_x);      printf(CLR_INFO "┌──────────┐" CLR_RESET);
	tui_goto(y+1, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_RED "%-8d" CLR_RESET " " CLR_BORDER "│" CLR_RESET, fail);
	tui_goto(y+2, card_x);      printf(CLR_INFO "│" CLR_RESET " " ANSI_BRIGHT_WHITE "%-11s" CLR_RESET " " CLR_BORDER "│" CLR_RESET, "未通过");
	tui_goto(y+3, card_x);      printf(CLR_INFO "└──────────┘" CLR_RESET);
	y += 5;

	/* Message area */
	if (msg && msg[0]) {
		tui_goto(y, x);
		printf(CLR_BORDER "║" CLR_RESET "  " CLR_WARNING "%s" CLR_RESET "\033[K", msg);
		y++;
	}

	/* Spacing before table */
	tui_goto(y, x);
	printf(CLR_BORDER "║" CLR_RESET "\033[K");
	y++;

		/* Student table — 固定列位对齐, 右边框定位到 table 右边缘 */
		int table_w = w - 2;
		int col_rb  = x + table_w - 1;   /* 表格右边框固定列 */
		int col_id  = x + 3;             /* 学号起始 */
		int col_nm  = col_id + 13;       /* 姓名起始 */
		int col_cn  = col_nm + 9;        /* 语文起始 */
		int col_ma  = col_cn + 5;        /* 数学起始 */
		int col_en  = col_ma + 5;        /* 英语起始 */
		int col_tot = col_en + 5;        /* 总分起始 */

		/* -- 表格顶部 -- */
		tui_goto(y, x);
		printf(CLR_BORDER "║ " CLR_BORDER "┌");
		for (int i = 0; i < table_w - 4; i++) printf("─");
		printf("┐"" ║" CLR_RESET "\033[K");
		y++;

		/* -- 表头 -- */
		tui_goto(y, x);      printf(CLR_BORDER "║ " CLR_BORDER "│" CLR_RESET);
		tui_goto(y, col_id); printf(ANSI_BRIGHT_BLUE "%-12s" CLR_RESET, "学号");
		tui_goto(y, col_nm); printf(ANSI_BRIGHT_BLUE "%-8s"  CLR_RESET, "姓名");
		tui_goto(y, col_cn); printf(ANSI_BRIGHT_BLUE "%4s"   CLR_RESET, "语文");
		tui_goto(y, col_ma); printf(ANSI_BRIGHT_BLUE "%4s"   CLR_RESET, "数学");
		tui_goto(y, col_en); printf(ANSI_BRIGHT_BLUE "%4s"   CLR_RESET, "英语");
		tui_goto(y, col_tot);printf(ANSI_BRIGHT_BLUE "%4s"   CLR_RESET, "总分");
		tui_goto(y, col_rb); printf(CLR_BORDER "│"" ║" CLR_RESET "\033[K");
		y++;

		/* -- 数据行 -- */
		int max_rows = layout->content_y1 - y - 2;
		if (max_rows < 0) max_rows = 0;

		if (list && list->size > 0) {
			Node* cur = list->head->next;
			int row_i = 0;
			while (cur != list->tail && row_i < max_rows) {
				Student* s = (Student*)cur->data;
				int tot = s->scores[CHINESE] + s->scores[MATH] + s->scores[ENGLISH];
				tui_goto(y, x);      printf(CLR_BORDER "║ " CLR_BORDER "│" CLR_RESET);
				tui_goto(y, col_id); printf(ANSI_BRIGHT_WHITE "%-12s" CLR_RESET, s->id);
				tui_goto(y, col_nm); printf(ANSI_BRIGHT_WHITE "%-8s"  CLR_RESET, s->name);
				{
					const char* sc_cn = s->scores[CHINESE] >= 90 ? CLR_SCORE_HIGH :
					                    s->scores[CHINESE] >= 60 ? CLR_SCORE_MED : CLR_SCORE_LOW;
					tui_goto(y, col_cn); printf("%s%4d" CLR_RESET, sc_cn, s->scores[CHINESE]);
				}
				{
					const char* sc_ma = s->scores[MATH] >= 90 ? CLR_SCORE_HIGH :
					                    s->scores[MATH] >= 60 ? CLR_SCORE_MED : CLR_SCORE_LOW;
					tui_goto(y, col_ma); printf("%s%4d" CLR_RESET, sc_ma, s->scores[MATH]);
				}
				{
					const char* sc_en = s->scores[ENGLISH] >= 90 ? CLR_SCORE_HIGH :
					                    s->scores[ENGLISH] >= 60 ? CLR_SCORE_MED : CLR_SCORE_LOW;
					tui_goto(y, col_en); printf("%s%4d" CLR_RESET, sc_en, s->scores[ENGLISH]);
				}
				tui_goto(y, col_tot);printf(CLR_INFO "%-4d" CLR_RESET, tot);
				tui_goto(y, col_rb); printf(CLR_BORDER "│" CLR_RESET "\033[K");
				y++;
				cur = cur->next;
				row_i++;
			}
			if (cur != list->tail) {
				tui_goto(y, x);      printf(CLR_BORDER "║ " CLR_BORDER "│" CLR_RESET);
				tui_goto(y, col_id); printf(CLR_INFO "... 还有 %d 条 ..." CLR_RESET, list->size - row_i);
				tui_goto(y, col_rb); printf(CLR_BORDER "│" CLR_RESET "\033[K");
				y++;
			}
		}
		else {
			tui_goto(y, x);      printf(CLR_BORDER "║ " CLR_BORDER "│" CLR_RESET);
			tui_goto(y, col_id); printf(CLR_INFO "(暂无数据)" CLR_RESET);
			tui_goto(y, col_rb); printf(CLR_BORDER "│"" ║" CLR_RESET "\033[K");
			y++;
		}

		/* -- 表格底部 -- */
		tui_goto(y, x);
		printf(CLR_BORDER "║ " CLR_BORDER "└");
		for (int i = 0; i < table_w - 4; i++) printf("─");
		printf("┘"" ║" CLR_RESET "\033[K");
		y++;

		/* -- Dashboard 底部 -- */
		tui_goto(y, x);
		printf(CLR_BORDER "╚");
		for (int i = 0; i < w - 2; i++) printf("═");
		printf("╝" CLR_RESET "\033[K");
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
	int w = layout->cols - x - 4;  /* 右侧留 3 列: 1 间隙 + 1 外框 + 1 边距 */

	tui_goto(y, x);
	/* 固定: "╔══ "(4) + title(display) + "╗"(1) = 5 + display_w */
	printf(CLR_BORDER "╔══ " CLR_TITLE "%s" CLR_BORDER, title);
	int remaining = w - 5 - (int)strlen(title);
	if (remaining < 0) remaining = 0;
	for (int i = 0; i < remaining; i++) printf("═");
	printf("╗" CLR_RESET "\033[K");
	y++;

	while (y < layout->content_y1) {
		tui_goto(y, x);
		printf(CLR_BORDER "║" CLR_RESET "\033[K");
		y++;
	}

	tui_goto(y, x);
	printf(CLR_BORDER "╚");
	for (int i = 0; i < w - 2; i++) printf("═");
	printf("╝" CLR_RESET "\033[K");
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
	for (int i = SIDEBAR_WIDTH + 1; i < w - 2; i++) printf("─");
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
