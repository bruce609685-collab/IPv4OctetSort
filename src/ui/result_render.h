#pragma once

#include <windows.h>

#include <string>

#include "ui/result_panel.h"
#include "ui/ui_state.h"

namespace ui {

// 空缺标记文案（§5.5）
inline const wchar_t* const kGapText = L"IP地址空缺";

// 结果列表重建：普通单列 / 补位双列 / 空态引导（FR-4.1、FR-4.5、FR-4.7、§7.6）
void ResultRenderRebuild(const ResultPanelControls& panel, const UiState& state);

// 空缺行着色、结果行斑马纹与空态提示字体（自绘三处之一），
// 主窗口 WM_NOTIFY(NM_CUSTOMDRAW) 调用；uiFont 用于空态提示行（非等宽字体）。
// 返回值必须是 CDRF_* 标志本身（不可压成 bool：压缩会丢掉通知标志）。
DWORD ResultRenderCustomDraw(LPARAM lparam, HFONT uiFont);

// FR-6.1 复制内容：普通模式单列；补位模式两列（槽位地址 + TAB + 清单地址或空缺标记）
std::wstring ResultRenderCopyText(const UiState& state);

// FR-9「仅复制空缺地址」：只给出空缺的槽位地址（单列）
std::wstring ResultRenderGapText(const UiState& state, size_t* gapCount);

}  // namespace ui
