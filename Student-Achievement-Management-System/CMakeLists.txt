# CMakeList.txt: Student-Achievement-Management-System 的 CMake 项目
# 在此处包括源代码并定义项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.10)

# 1. 项目名称和语言
project(StudentSystem C)

# 2. 强制要求 C11 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 3. 指定所有源文件
# 建议直接列出你 src 目录下的所有 .c 文件
set(SOURCES 
    src/main.c 
    src/list.c 
    src/student.c
    # 如果还有其他 .c 文件，继续在这里列出来
)

# 4. 创建可执行文件
add_executable(StudentSystem ${SOURCES})

# 5. 跨平台强制 UTF-8 编码 (MSVC 编译器支持)
if(MSVC)
    target_compile_options(StudentSystem PRIVATE /utf-8)
endif()

