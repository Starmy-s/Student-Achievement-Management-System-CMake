/**
 * @brief 终端 UI 增强库：ANSI 颜色、Unicode 制表符、跨平台工具
 * @date 2026-05-29
 */

#ifndef UI_H
#define UI_H

#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

/* ========== ANSI 基础转义序列 ========== */

#define ANSI_RESET          "\033[0m"
#define ANSI_BOLD           "\033[1m"
#define ANSI_DIM            "\033[2m"

#define ANSI_RED            "\033[31m"
#define ANSI_GREEN          "\033[32m"
#define ANSI_YELLOW         "\033[33m"
#define ANSI_BLUE           "\033[34m"
#define ANSI_MAGENTA        "\033[35m"
#define ANSI_CYAN           "\033[36m"
#define ANSI_WHITE          "\033[37m"

#define ANSI_BRIGHT_RED     "\033[91m"
#define ANSI_BRIGHT_GREEN   "\033[92m"
#define ANSI_BRIGHT_YELLOW  "\033[93m"
#define ANSI_BRIGHT_BLUE    "\033[94m"
#define ANSI_BRIGHT_MAGENTA "\033[95m"
#define ANSI_BRIGHT_CYAN    "\033[96m"
#define ANSI_BRIGHT_WHITE   "\033[97m"

/* ========== 语义化颜色别名 ========== */

#define CLR_RESET           ANSI_RESET
#define CLR_TITLE           ANSI_BOLD ANSI_BRIGHT_CYAN
#define CLR_BORDER          ANSI_DIM ANSI_CYAN
#define CLR_MENU_NUM        ANSI_BRIGHT_CYAN
#define CLR_MENU_TEXT       ANSI_CYAN
#define CLR_SUCCESS         ANSI_BRIGHT_GREEN
#define CLR_ERROR           ANSI_BRIGHT_RED
#define CLR_WARNING         ANSI_BRIGHT_YELLOW
#define CLR_INFO            ANSI_DIM
#define CLR_PROMPT          ANSI_CYAN
#define CLR_SCORE_HIGH      ANSI_BRIGHT_GREEN
#define CLR_SCORE_MED       ANSI_BRIGHT_YELLOW
#define CLR_SCORE_LOW       ANSI_BRIGHT_RED

/* ========== Unicode 制表符 (双线) ========== */

#define BOX_DL_TL   "╔"
#define BOX_DL_TR   "╗"
#define BOX_DL_BL   "╚"
#define BOX_DL_BR   "╝"
#define BOX_DL_H    "═"
#define BOX_DL_V    "║"
#define BOX_DL_ML   "╠"
#define BOX_DL_MR   "╣"
#define BOX_DL_MT   "╦"
#define BOX_DL_MB   "╩"
#define BOX_DL_MC   "╬"

/* ========== Unicode 制表符 (单线) ========== */

#define BOX_LT_V    "│"
#define BOX_LT_H    "─"
#define BOX_LT_TL   "┌"
#define BOX_LT_TR   "┐"
#define BOX_LT_BL   "└"
#define BOX_LT_BR   "┘"
#define BOX_LT_ML   "├"
#define BOX_LT_MR   "┤"
#define BOX_LT_MT   "┬"
#define BOX_LT_MB   "┴"
#define BOX_LT_MC   "┼"

/* ========== 跨平台工具 ========== */

#ifdef _WIN32
#define SLEEP_MS(ms)  Sleep(ms)
#else
#define SLEEP_MS(ms)  usleep((ms) * 1000)
#endif

#endif /* UI_H */
