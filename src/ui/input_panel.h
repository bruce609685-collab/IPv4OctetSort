#pragma once

#include <windows.h>

#include <string>
#include <vector>

#include "core/types.h"

namespace ui {

struct InputPanelControls {
    HWND title = nullptr;
    HWND btnClear = nullptr;
    HWND statIn = nullptr;
    HWND edit = nullptr;
    HWND hint = nullptr;  // FR-1.7：输入框下方的粘贴方式提示（界面字体、灰色）
};

// 创建输入面板控件（FR-1 / FR-7）
void InputPanelCreate(InputPanelControls& controls, HWND parent, HINSTANCE instance, HFONT font,
                      HFONT monoFont);

std::wstring InputPanelGetText(const InputPanelControls& controls);
void InputPanelSetText(const InputPanelControls& controls, const std::wstring& text);

// 面板标题行右侧统计（§5.3：左操作、右数据）
void InputPanelUpdateStat(const InputPanelControls& controls, size_t validCount, size_t invalidCount,
                          bool deduped);

// FR-11：语言切换后刷新本面板的静态文字（标题/按钮/提示；统计随 ApplyInput 更新）
void InputPanelApplyLanguage(const InputPanelControls& controls);

// 无效项标签区（FR-1.5，自绘三处之一）：
// 先在布局阶段测量所需高度，再于主窗口 WM_PAINT 中绘制。
int FaultsMeasure(const std::vector<core::InvalidItem>& invalid, int width, HFONT monoFont);
void FaultsPaint(HDC dc, const RECT& area, const std::vector<core::InvalidItem>& invalid,
                 HFONT monoFont);

}  // namespace ui
