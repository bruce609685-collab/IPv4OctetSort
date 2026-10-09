# IPv4OctetSort 编译配置（文档 §8.2 build/CMakeLists.txt）
#
# 用法（已安装 MinGW-w64 与 Ninja 时）：
#   cmake -G Ninja -S build -B build/ninja
#   cmake --build build/ninja
# 不使用 CMake 时可直接运行 build/build.bat（效果相同，且可在中文路径下直接编译）。
#
# 路径要求：CMake/Ninja 生成的目标文件路径包含源码目录名，而 MinGW 工具链在
# 非 ASCII 路径下会失败。本工程目录名含中文，因此请二选一：
#   · 用 build\build_cmake.bat（在英文临时目录建 junction 后编译，推荐）
#   · 或把工程目录改为纯 ASCII 名称
#
# 关键编译选项（§3.3）：
#   · _WIN32_WINNT=0x0601 目标系统 Windows 7 SP1
#   · -static 静态链接运行时，满足「零依赖」，等价于 MSVC 的 /MT
#   · 子系统最低版本 6.01

cmake_minimum_required(VERSION 3.16)

# 默认优先使用 32 位工具链：32 位产物可在 32 位与 64 位 Windows 上运行（§2.1）。
# 可用环境变量 MINGW32_BIN 覆盖工具链目录。
if(NOT DEFINED CMAKE_CXX_COMPILER)
    if(DEFINED ENV{MINGW32_BIN})
        set(_mingw32 "$ENV{MINGW32_BIN}")
    else()
        set(_mingw32 "C:/msys64/mingw32/bin")
    endif()
    # 统一成正斜杠：反斜杠会被写入生成的 .cmake 文件并触发转义错误
    file(TO_CMAKE_PATH "${_mingw32}" _mingw32)
    if(EXISTS "${_mingw32}/i686-w64-mingw32-g++.exe")
        set(CMAKE_CXX_COMPILER "${_mingw32}/i686-w64-mingw32-g++.exe")
        set(CMAKE_RC_COMPILER "${_mingw32}/windres.exe")
        message(STATUS "toolchain: 32-bit (${_mingw32})")
    else()
        message(STATUS "toolchain: default compiler; a 64-bit exe will not run on 32-bit Windows")
    endif()
endif()

project(IPv4OctetSort CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "IPv4OctetSort 仅支持 Windows")
endif()

# 输出目录与产物名（§2.3：单个 exe，程序显示名 IPv4OctetSort）
set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../dist)

# 按目标架构区分产物名，避免 32/64 位互相覆盖
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(APP_OUTPUT_NAME IPv4OctetSort)
    set(TEST_OUTPUT_NAME test_core)
else()
    set(APP_OUTPUT_NAME IPv4OctetSort-x64)
    set(TEST_OUTPUT_NAME test_core-x64)
endif()

add_compile_definitions(UNICODE _UNICODE _WIN32_WINNT=0x0601 WINVER=0x0601 $<$<CONFIG:Release>:NDEBUG>)

# 构建日期（§9：编译时自动生成，写入生成的 build_date.h）
string(TIMESTAMP APP_BUILD_DATE "%Y-%m-%d")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/build_date.h
     "/* 由 CMake 生成：构建日期 */\n#define APP_BUILD_DATE \"${APP_BUILD_DATE}\"\n")

set(COMMON_OPTIONS -Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8)

# ---- 语言包内嵌生成器（FR-12）：tools/lng2cpp.cpp 扫描 languages/*.lng，
# 生成 languages.cpp（定义 ui::kEmbeddedLanguages），打进主程序 ----
file(GLOB LNG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../languages/*.lng)
add_executable(lng2cpp EXCLUDE_FROM_ALL ../tools/lng2cpp.cpp)
target_compile_definitions(lng2cpp PRIVATE UNICODE _UNICODE)
target_compile_options(lng2cpp PRIVATE -O2)
target_link_options(lng2cpp PRIVATE -municode -static -static-libgcc -static-libstdc++ -s)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/languages.cpp
    COMMAND lng2cpp ${CMAKE_CURRENT_BINARY_DIR}/languages.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../languages
    DEPENDS lng2cpp ${LNG_FILES}
    COMMENT "Embedding language packs (languages/*.lng)")

# ---- 主程序 ----
set(APP_SOURCES
    ../src/main.cpp
    ../src/app/single_instance.cpp
    ../src/app/dpi_aware.cpp
    ../src/core/tokenizer.cpp
    ../src/core/parser.cpp
    ../src/core/comparator.cpp
    ../src/core/sorter.cpp
    ../src/core/segment.cpp
    ../src/core/dedupe.cpp
    ../src/core/slot_filler.cpp
    ../src/core/pipeline.cpp
    ../src/core/lng_file.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/languages.cpp
    ../src/ui/main_window.cpp
    ../src/ui/layout.cpp
    ../src/ui/toolbar.cpp
    ../src/ui/i18n.cpp
    ../src/ui/input_panel.cpp
    ../src/ui/result_panel.cpp
    ../src/ui/result_render.cpp
    ../src/ui/status_bar.cpp
    ../src/ui/menu_bar.cpp
    ../src/ui/about_dialog.cpp
    ../src/util/text_convert.cpp
    ../src/util/clipboard.cpp
    ../src/util/logger.cpp
    ../src/util/shell_open.cpp)

add_executable(IPv4OctetSort WIN32 ${APP_SOURCES} ../src/resource/app.rc)

target_include_directories(IPv4OctetSort PRIVATE ../src ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(IPv4OctetSort PRIVATE ${COMMON_OPTIONS} -O2)
target_link_options(IPv4OctetSort PRIVATE
    -municode -static -static-libgcc -static-libstdc++ -s
    -Wl,--major-subsystem-version=6 -Wl,--minor-subsystem-version=1)
target_link_libraries(IPv4OctetSort PRIVATE comctl32 shell32 gdi32 user32)

set_target_properties(IPv4OctetSort PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    OUTPUT_NAME ${APP_OUTPUT_NAME})

# ---- 核心逻辑测试（命令行运行，不依赖界面） ----
# 测试会校验 languages/*.lng 语言包（键齐全、格式符一致），运行目录传 languages/
set(TEST_SOURCES
    ../tests/test_core.cpp
    ../src/core/tokenizer.cpp
    ../src/core/parser.cpp
    ../src/core/comparator.cpp
    ../src/core/sorter.cpp
    ../src/core/segment.cpp
    ../src/core/dedupe.cpp
    ../src/core/slot_filler.cpp
    ../src/core/pipeline.cpp
    ../src/core/lng_file.cpp)

add_executable(test_core ${TEST_SOURCES})
target_include_directories(test_core PRIVATE ../src ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(test_core PRIVATE ${COMMON_OPTIONS} -O2)
target_link_options(test_core PRIVATE -static -static-libgcc -static-libstdc++ -s)
set_target_properties(test_core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    OUTPUT_NAME ${TEST_OUTPUT_NAME})

# 资源编译（windres）：源文件为 UTF-8，需指定代码页
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} --codepage=65001")
endif()
