#pragma once

#include <windows.h>

#include "ui/i18n.h"

// 菜单栏（FR-9）自 v0.3 起由代码构建：文字随语言切换后整菜单重建。
// 菜单项一律使用系统标准绘制（与 v0.2 的资源菜单外观一致）：
// 主题选中配色、顶层项无箭头、& 助记符与 \t 快捷键由系统原生处理。
namespace ui {

// 构建主菜单并挂到窗口（替换已有菜单）
void MenuBarAttach(HWND window, HINSTANCE instance);

// 语言切换后重建整个菜单（重建后启用状态由调用方重新施加）
void MenuBarRebuild(HWND window, HINSTANCE instance);

// 加速键翻译；调用方需在输入框获得焦点时不翻译，保证 Ctrl+C / Ctrl+V 为编辑框原生行为
bool MenuBarTranslate(HWND window, MSG* message);

// 按当前状态启用 / 置灰菜单项
void MenuBarEnableItems(HWND window, bool hasValid, bool sorted, bool fillChecked);

// 语言下拉菜单（两项：简体中文 / English，当前语言带选中标记）。
// 调用方在 TrackPopupMenu 结束后用 MenuBarFreeLangMenu 释放。
HMENU MenuBarBuildLangMenu(Lang current);
void MenuBarFreeLangMenu(HMENU menu);

}  // namespace ui
