#pragma once

#include <windows.h>

#include "core/types.h"

namespace ui {

struct ToolbarControls {
    HWND btnAsc = nullptr;
    HWND btnDesc = nullptr;
    HWND segLabel = nullptr;
    HWND radio[4] = {nullptr, nullptr, nullptr, nullptr};
    HWND chkDedupe = nullptr;
    HWND chkFill = nullptr;
    HWND btnLang = nullptr;  // 语言下拉（FR-11）
    HWND btnUpdate = nullptr;
};

// 创建工具栏控件（位置由 layout 统一安排）
void ToolbarCreate(ToolbarControls& controls, HWND parent, HINSTANCE instance, HFONT font);

// FR-11：语言切换后刷新各控件文字（位置不变，控件尺寸两种语言通用）
void ToolbarApplyLanguage(const ToolbarControls& controls);

// FR-2.6：排序按钮高亮跟随 state.mode（最小自绘，见 ToolbarDrawSortButton）
void ToolbarSyncSortButtons(const ToolbarControls& controls);

// FR-3.3：仅差异段可选可勾选；无差异段时全部置灰且不选中
void ToolbarSyncSegments(const ToolbarControls& controls, bool hasData, int diffSeg);

// FR-2.6：无有效地址时两按钮置灰禁用
void ToolbarEnableSortButtons(const ToolbarControls& controls, bool enabled);

// 排序按钮的自绘（主窗口 WM_DRAWITEM 调用）
void ToolbarDrawSortButton(const ToolbarControls& controls, const DRAWITEMSTRUCT& item);

}  // namespace ui
