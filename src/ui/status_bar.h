#pragma once

#include <windows.h>

#include <string>

namespace ui {

struct StatusBarControls {
    HWND bar = nullptr;
};

// 状态栏三个分区（§5.2、§5.4）
void StatusBarCreate(StatusBarControls& controls, HWND parent, HINSTANCE instance, HFONT font);
void StatusBarSetText(const StatusBarControls& controls, const std::wstring& left,
                      const std::wstring& middle, const std::wstring& right);
void StatusBarResize(const StatusBarControls& controls, int clientWidth);

}  // namespace ui
