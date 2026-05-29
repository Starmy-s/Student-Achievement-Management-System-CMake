/**
 * @brief 专业 TUI 框架：多面板布局、键盘导航、光标定位渲染
 * @date 2026-05-29
 */

#ifndef TUI_H
#define TUI_H

#include "list.h"
#include "ui.h"
#include <stdbool.h>

/* ========== 布局常量 ========== */

#define TUI_MIN_COLS     80
#define TUI_MIN_ROWS     24
#define SIDEBAR_WIDTH    18

/* ========== 键盘事件 ========== */

typedef enum {
	KEY_NONE = 0,
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
	KEY_ENTER, KEY_ESC, KEY_TAB, KEY_BACKSPACE,
	KEY_HOME, KEY_END,
	KEY_CHAR,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4,
	KEY_F5, KEY_F6, KEY_F7, KEY_F8
} KeyType;

typedef struct {
	KeyType type;
	char    ch;
	int     digit;   /* 0-9 if ch is a digit, -1 otherwise */
} KeyEvent;

/* ========== 布局几何 ========== */

typedef struct {
	int rows, cols;
	int content_y0, content_y1;  /* dashboard + sidebar area */
	int content_rows;
} TUILayout;

/* ========== 生命周期 ========== */

void tui_init(void);
void tui_cleanup(void);
bool tui_get_size(TUILayout* layout);

/* ========== 光标控制 ========== */

void tui_goto(int row, int col);
void tui_hide_cursor(void);
void tui_show_cursor(void);

/* ========== 输入 ========== */

KeyEvent tui_read_key(void);
bool     tui_read_line(int row, int col, const char* prompt,
                       char* buffer, int max_len, bool* cancelled);
int      tui_read_int(int row, int col, const char* prompt,
                      int min, int max, bool* cancelled);

/* ========== 面板渲染 ========== */

void tui_draw_borders(const TUILayout* layout);
void tui_draw_header(const TUILayout* layout, int count);
void tui_draw_sidebar(const TUILayout* layout, int focus);
void tui_draw_dashboard(const TUILayout* layout, List* list, const char* msg);
void tui_draw_operation_frame(const TUILayout* layout, const char* title);
void tui_draw_choice_menu(const TUILayout* layout, int y_off,
                          const char* items[], int count);
void tui_draw_confirm(const TUILayout* layout, const char* message);
void tui_draw_footer(const TUILayout* layout, const char* prompt);

/* ========== 菜单数据 ========== */

#define MENU_COUNT 7
extern const char* const MENU_LABELS[MENU_COUNT];
extern const int         MENU_KEYS[MENU_COUNT];

#endif /* TUI_H */
