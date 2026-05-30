# 学生成绩管理信息系统

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.txt)
[![Standard: C11](https://img.shields.io/badge/C-11-blue.svg)](https://en.cppreference.com/w/c/11)
[![Platform: Windows/Linux/macOS](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](#build)

基于 **C 语言** 的跨平台控制台应用，使用三层架构设计，提供学生成绩的增删查改、导入导出等功能。

---

## 功能一览

| 编号 | 功能 | 说明 |
|:---:|------|------|
| 1 | 初始化数据 | 批量录入多名学生 |
| 2 | 新增数据 | 逐条添加学生（支持连续添加） |
| 3 | 删除数据 | 按学号 / 按姓名删除，姓名匹配多删 |
| 4 | 查找数据 | 按学号 / 按姓名精确查询 |
| 5 | 导出数据 | 内存数据保存到 `students.dat`（二进制） |
| 6 | 导入数据 | 从 `students.dat` 恢复数据到内存 |
| 7 | 浏览数据 | 打印所有学生的成绩总表 |
| 0 | 退出系统 | 退出前提示保存，安全退出 |

### 学生信息字段

- **学号** — 最长 19 字符
- **姓名** — 最长 49 字符
- **三科成绩** — 语文、数学、英语（0–100 分）
- **自动计算** — 总分

### 界面效果

- ANSI 彩色终端输出（标题/成功/警告/错误/高亮各有独立配色）
- 备用屏幕缓冲区（退出后恢复原终端内容）

---

## Build

### 前置条件

- **CMake** ≥ 3.10
- **Ninja**（推荐生成器）
- **C11 编译器**
  - Windows: MSVC (`cl.exe`) 或 MinGW
  - Linux: `gcc` / `clang`
  - macOS: `clang`

### 配置与编译

项目通过 `CMakePresets.json` 提供多平台预设。

```sh
# === Windows (x64 Debug) ===
# 1. 先启动 MSVC 环境 (VS Developer Command Prompt 或运行 vcvars64.bat)
# 2. 配置
cmake --preset x64-debug

# 3. 编译
cmake --build out/build/x64-debug

# 可执行文件位于: out/build/x64-debug/StudentSystem.exe
```

```sh
# === Windows (x64 Release) ===
cmake --preset x64-release
cmake --build out/build/x64-release
```

```sh
# === Linux (Debug) ===
cmake --preset linux-debug
cmake --build out/build/linux-debug
```

```sh
# === macOS (Debug) ===
cmake --preset macos-debug
cmake --build out/build/macos-debug
```

### 可用预设一览

| 预设名 | 平台 | 架构 | 编译模式 |
|--------|------|:---:|----------|
| `x64-debug` | Windows | x64 | Debug |
| `x64-release` | Windows | x64 | Release |
| `x86-debug` | Windows | x86 | Debug |
| `x86-release` | Windows | x86 | Release |
| `linux-debug` | Linux | - | Debug |
| `macos-debug` | macOS | - | Debug |

### 手动配置（不使用预设）

```sh
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_STANDARD=11
cmake --build build
```

---

## 架构设计

```
┌─────────────────────────────────────┐
│           UI 层 (main.c)            │
│   菜单循环 · 用户交互 · 文件 I/O     │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      业务逻辑层 (student.c)          │
│   学生 CRUD · 查重 · 格式化输出      │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│     数据结构层 (list.c)              │
│   泛型双向链表 · 哨兵节点 · void*    │
└─────────────────────────────────────┘
```

### 数据结构层 — `list.c`

基于哨兵节点的**泛型双向链表**，通过 `void*` 承载任意数据类型。

- `list_create()` — 创建空链表（含 `head`/`tail` 哨兵节点）
- `list_append()` — 在尾部哨兵前插入节点
- `list_delete()` — 安全卸下并释放节点（**不释放** `node->data`，调用者负责）
- `list_destory()` — 销毁整条链表（需先调用 `student_clear_all_data()` 释放数据）

### 业务逻辑层 — `student.c`

| 函数 | 功能 |
|------|------|
| `student_add()` | 学号查重后追加（自动截断超长字符串） |
| `student_query_by_id()` | 按学号查找并格式化打印 |
| `student_query_by_name()` | 按姓名遍历，打印所有匹配项 |
| `student_delete_by_id()` | 按学号删除（先 `free` 数据再卸节点） |
| `student_delete_by_name()` | 按姓名批量删除 |
| `student_print_all()` | 打印成绩总表 |
| `student_clear_all_data()` | 释放链表中全部 `Student*` |

### UI 层 — `main.c`

- 主菜单循环 + 8 个功能分支
- ANSI 转义序列实现清屏、彩色打印、备用屏幕缓冲
- Windows 下自动启用 VT 处理 + UTF-8 I/O
- 健壮的输入校验（整数范围、非空姓名、学号格式）

---

## 数据持久化

- 格式：原始 `Student` 结构体以 **二进制** (`fwrite`/`fread`) 读写
- 文件：`students.dat`（与可执行文件同目录）
- 导出：`put()` — 遍历链表逐条写入，写完后清空内存
- 导入：`get()` — 逐条 `fread` 重建链表，若内存已有数据会提示确认覆盖
- 退出保护：退出时若内存有未存盘数据，提示用户导出

---

## 项目结构

```
Student-Achievement-Management-System/
├── CMakeLists.txt              # CMake 构建脚本
├── CMakePresets.json           # 多平台构建预设
├── LICENSE.txt                 # MIT 协议
├── README.md                   # 本文件
├── include/
│   ├── list.h                  # 链表接口
│   ├── student.h               # 学生结构体 & 业务接口
│   └── terminal_colors.h       # ANSI 颜色宏
└── src/
    ├── list.c                  # 链表实现
    ├── student.c               # 业务逻辑实现
    └── main.c                  # 主程序 & UI
```

---

## 技术要点

- **C11 标准** — 严格的 ANSI C，无平台扩展依赖
- **跨平台** — Windows / Linux / macOS 统一代码，通过条件编译处理平台差异
- **UTF-8** — Windows 下通过 manifest + `/utf-8` 编译选项保证中文正确显示
- **`snprintf` 防越界** — 替代不安全的 `strcpy`/`strncpy`，自动截断并保证 `\0` 结尾
- **哨兵节点** — 消除插入/删除时的边界判断，简化代码逻辑
- **ANSI 颜色** — 语义化颜色宏（`C_OK`、`C_ERR`、`C_HIGHLIGHT` 等），Windows 10+ 原生支持

---

## License

本项目基于 [MIT License](LICENSE.txt) 开源。
