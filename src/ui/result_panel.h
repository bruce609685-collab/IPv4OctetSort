#pragma once

#include <windows.h>

#include <string>

namespace ui {

struct ResultPanelControls {
    HWND title = nullptr;
    HWND btnCopy = nullptr;
    HWND copyMsg = nullptr;
    HWND statOut = nullptr;
    HWND list = nullptr;
};

// 创建结果面板控件（FR-4.8 / FR-6.2）
void ResultPanelCreate(ResultPanelControls& controls, HWND parent, HINSTANCE instance, HFONT font,
                       HFONT monoFont);

// FR-6.3：无有效地址或尚未排序时置灰
void ResultPanelSetCopyEnabled(const ResultPanelControls& controls, bool enabled);

// FR-6.2：复制提示的显示与隐藏（文字始终占位）
void ResultPanelShowCopyMsg(const ResultPanelControls& controls, bool visible);

void ResultPanelSetStat(const ResultPanelControls& controls, const std::wstring& text);

// 结果列表的表头随模式变化（普通单列 / 补位双列 / 空态）
enum class ResultView { Empty, Plain, Fill };
void ResultPanelSetColumns(const ResultPanelControls& controls, ResultView mode, int listWidth);

// FR-11：语言切换后刷新本面板的静态文字（标题/按钮/复制提示）
void ResultPanelApplyLanguage(const ResultPanelControls& controls);

}  // namespace ui
