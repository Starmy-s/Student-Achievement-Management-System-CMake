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

# 关键：告诉编译器，在编译 StudentSystem 时，额外去 "include" 目录下找头文件
target_include_directories(StudentSystem PRIVATE 
    include
)

# 5. 跨平台强制 UTF-8 编码
if(WIN32)
    # 指定 manifest 文件的绝对路径
    set(MANIFEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/app.manifest")

    if(MSVC)
        target_sources(StudentSystem PRIVATE ${MANIFEST_FILE})
        target_compile_options(StudentSystem PRIVATE /utf-8)
    else()
        # 需要通过资源文件 (.rc) 编译。我们让 CMake 在构建目录中自动生成它。
        set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/app_manifest.rc")

        # 写入 RC 语法: 1 代表 EXE 的 Manifest ID，24 代表 RT_MANIFEST 类型
        file(WRITE ${RC_FILE} "1 24 \"${MANIFEST_FILE}\"\n")

        # 将生成的 .rc 文件添加到目标源码中，MinGW 会自动调用 windres 编译它
        target_sources(StudentSystem PRIVATE ${RC_FILE})
    endif()
endif()

