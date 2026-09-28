#pragma once

#include <windows.h>

namespace ui {

// 菜单来自资源（FR-9），加速键表见 resource/app.rc
void MenuBarAttach(HWND window, HINSTANCE instance);

// 加速键翻译；调用方需在输入框获得焦点时不翻译，保证 Ctrl+C / Ctrl+V 为编辑框原生行为
bool MenuBarTranslate(HWND window, MSG* message);

// 按当前状态启用 / 置灰菜单项
void MenuBarEnableItems(HWND window, bool hasValid, bool sorted, bool fillChecked);

}  // namespace ui
