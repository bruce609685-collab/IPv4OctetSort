#pragma once

#include <windows.h>

namespace ui {

// 主窗口类名（单实例激活与窗口查找共用）
const wchar_t* MainWindowClassName();

// 注册窗口类并创建主窗口；成功返回 true
bool CreateMainWindow(HINSTANCE instance, int showCommand);

// 消息循环：加速键翻译 + 分发；返回进程退出码
int RunMessageLoop();

}  // namespace ui
