#pragma once

// 产品信息常量（文档 §9：单一来源，编译期固化进 exe，运行时不读外部文件）。
// 同时被 resource/app.rc 引用（VERSIONINFO 使用其中的 ANSI / 数值形式）。

// 显示名（简体中文界面与标题栏）
#define APP_NAME_CN L"IPv4网络地址排序器"
// 英文名 / 进程名
#define APP_NAME_EN L"IPv4OctetSort"
// 版本号（文档 §9；关于对话框按 "v" + APP_VERSION 显示）
#define APP_VERSION L"0.3"
#define APP_VERSION_ANSI "0.3"
#define APP_VERSION_STR "0.3.0"
#define APP_VERSION_CSV 0, 3, 0, 0
#define APP_VERSION_CSV_STR "0, 3, 0, 0"

// 发布页（FR-8 / 关于对话框的发布页链接）
#define APP_RELEASE_URL L"https://github.com/bruce609685-collab/IPv4OctetSort"

// 构建日期（§9：编译时自动生成）。
// 两种构建方式都支持：
//   · build/build.bat 通过 -DAPP_BUILD_DATE=\"YYYY-MM-DD\" 传入；
//   · CMake 在构建目录生成 build_date.h（优先采用）。
#if defined(__has_include)
#  if __has_include("build_date.h")
#    include "build_date.h"
#  endif
#endif
#ifndef APP_BUILD_DATE
#define APP_BUILD_DATE "未指定"
#endif
#define APP_BUILD_DATE_W L"" APP_BUILD_DATE

// 主窗口类名（单实例激活与窗口查找共用）
#define APP_WINDOW_CLASS L"IPv4OctetSortMainWindow"
