#pragma once

#include <windows.h>

namespace ui {

// FR-10：模态关于对话框（确定 / 标题栏关闭 / Esc 均可关闭；发布页可点击）
void ShowAboutDialog(HWND owner, HINSTANCE instance);

}  // namespace ui
