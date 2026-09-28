#include "ui/result_render.h"

#include <commctrl.h>

#include "ui/control_ids.h"
#include "ui/theme_colors.h"

namespace ui {
namespace {

// 序号列：与预览版一致，按总行数的位数补零（如 001 / 002）
std::wstring IndexText(int index, int total) {
    std::wstring text = std::to_wstring(index);
    int digits = 1;
    for (int value = total; value >= 10; value /= 10) ++digits;
    while (static_cast<int>(text.size()) < digits) text.insert(text.begin(), L'0');
    return text;
}

std::wstring SlotText(const core::SlotRow& row) {
    std::wstring text;
    for (int i = 0; i < core::kSegmentCount; ++i) {
        if (i) text += L".";
        text += std::to_wstring(row.slot[i]);
    }
    return text;
}

void InsertRow(HWND list, int row, const std::wstring& first, const std::wstring& second,
               const std::wstring& third) {
    LVITEMW item = {};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = 0;
    item.pszText = const_cast<wchar_t*>(first.c_str());
    const int inserted = ListView_InsertItem(list, &item);
    if (inserted < 0) return;

    if (!second.empty()) {
        ListView_SetItemText(list, inserted, 1, const_cast<wchar_t*>(second.c_str()));
    }
    if (!third.empty()) {
        ListView_SetItemText(list, inserted, 2, const_cast<wchar_t*>(third.c_str()));
    }
}

// 超限提示条是否占据列表首行（FR-4.7）
bool HasNotice(const UiState& state) {
    return state.fillChecked && state.hasView && state.view.fill.tooLarge;
}

// 列表行 → 该行的性质（用于着色与字体）
struct RowInfo {
    bool notice = false;  // FR-4.7 超限提示条
    bool hint = false;    // 空态提示行（尚未排序）
    int dataRow = 0;      // 数据行序号（用于斑马纹）
};

RowInfo InfoOfRow(const UiState& state, int row) {
    RowInfo info;
    if (HasNotice(state)) {
        if (row == 0) {
            info.notice = true;
            return info;
        }
        info.dataRow = row - 1;
    } else {
        info.dataRow = row;
    }
    if (!state.hasView) info.hint = true;  // 空态提示行：用界面字体、弱化颜色
    return info;
}

int ListClientWidth(HWND list) {
    RECT rect;
    GetClientRect(list, &rect);
    return rect.right - rect.left;
}

// 行的背景色：偶数行浅黄斑马纹、超限提示条为信息栏色
// 返回 false 表示使用列表默认底色（白）。空缺行与命中行同样式，不做额外区分。
bool RowBackground(const ThemeColors& colors, const RowInfo& info, COLORREF* color) {
    if (info.notice) {
        *color = colors.infoBack;
        return true;
    }
    if (info.dataRow % 2 == 1) {
        *color = colors.stripeRowBack;
        return true;
    }
    return false;
}

}  // namespace

void ResultRenderRebuild(const ResultPanelControls& panel, const UiState& state) {
    HWND list = panel.list;
    const int width = ListClientWidth(list);

    ResultView mode = ResultView::Empty;
    if (state.hasView) mode = state.view.useFill ? ResultView::Fill : ResultView::Plain;

    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(list);
    ResultPanelSetColumns(panel, mode, width);

    int row = 0;
    if (mode == ResultView::Empty) {
        // 空态：没有有效地址时不产生任何行（粘贴方式提示在输入框下方，FR-1.7）；
        // 已有有效地址但尚未排序时给一行操作提示。
        if (!state.valid.empty()) {
            InsertRow(list, row++, L"点「升序排列」或「降序排列」开始。", L"", L"");
        }
    } else {
        if (HasNotice(state)) {
            std::wstring text = L"按当前段位要铺 " + std::to_wstring(state.view.fill.estimate) +
                                L" 个槽位，超过上限 8,192 个，本次未补位，已按普通列表显示。";
            text += L"把输入收窄到更少的网段后即可补位。";
            InsertRow(list, row++, L"", text, L"");
        }

        if (mode == ResultView::Fill) {
            const int total = state.view.fill.total;
            for (size_t i = 0; i < state.view.fill.rows.size(); ++i) {
                const core::SlotRow& slotRow = state.view.fill.rows[i];
                const std::wstring index = IndexText(static_cast<int>(i) + 1, total);
                const std::wstring slot = slotRow.showSlot ? SlotText(slotRow) : std::wstring();
                const std::wstring entry =
                    slotRow.entryIndex >= 0
                        ? state.view.sorted[static_cast<size_t>(slotRow.entryIndex)].display
                        : std::wstring(kGapText);
                InsertRow(list, row++, index, slot, entry);
            }
        } else {
            const int total = static_cast<int>(state.view.sorted.size());
            for (size_t i = 0; i < state.view.sorted.size(); ++i) {
                InsertRow(list, row++, IndexText(static_cast<int>(i) + 1, total),
                          state.view.sorted[i].display, L"");
            }
        }
    }

    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    SendMessageW(list, WM_VSCROLL, SB_TOP, 0);
    InvalidateRect(list, nullptr, TRUE);
}

DWORD ResultRenderCustomDraw(LPARAM lparam, HFONT uiFont) {
    const NMLVCUSTOMDRAW* draw = reinterpret_cast<const NMLVCUSTOMDRAW*>(lparam);
    const UiState& state = State();
    const ThemeColors& colors = Colors();

    switch (draw->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT: {
            NMLVCUSTOMDRAW* mutableDraw = const_cast<NMLVCUSTOMDRAW*>(draw);
            const RowInfo info = InfoOfRow(state, static_cast<int>(draw->nmcd.dwItemSpec));

            COLORREF background = 0;
            if (RowBackground(colors, info, &background)) mutableDraw->clrTextBk = background;

            if (info.notice) mutableDraw->clrText = colors.infoText;
            if (info.hint) {
                // 空态提示行：界面字体 + 弱化颜色（列表字体为等宽，此处单独替换）
                if (uiFont != nullptr) SelectObject(mutableDraw->nmcd.hdc, uiFont);
                mutableDraw->clrText = colors.dimText;
                return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
            }
            return CDRF_NOTIFYSUBITEMDRAW;
        }

        case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
            NMLVCUSTOMDRAW* mutableDraw = const_cast<NMLVCUSTOMDRAW*>(draw);
            const int subItem = mutableDraw->iSubItem;
            const RowInfo info = InfoOfRow(state, static_cast<int>(draw->nmcd.dwItemSpec));

            // 子项阶段必须重新设置底色：返回 CDRF_DODEFAULT 会丢弃行阶段设置的颜色
            COLORREF background = 0;
            if (RowBackground(colors, info, &background)) mutableDraw->clrTextBk = background;

            if (info.notice || info.hint) {
                mutableDraw->clrText = info.notice ? colors.infoText : colors.dimText;
                return CDRF_NEWFONT;
            }
            if (subItem == 0) {
                mutableDraw->clrText = colors.dimText;  // 序号列弱化
                return CDRF_NEWFONT;
            }
            if (state.hasView && state.view.useFill && subItem == 1) {
                mutableDraw->clrText = colors.dimText;  // 槽位地址弱化
                return CDRF_NEWFONT;
            }

            // 其余数据单元（含「IP地址空缺」）一律用默认文字色与行底色，
            // 空缺行与命中行样式完全一致
            mutableDraw->clrText = GetSysColor(COLOR_WINDOWTEXT);
            return background ? CDRF_NEWFONT : CDRF_DODEFAULT;
        }

        default:
            return CDRF_DODEFAULT;
    }
}

std::wstring ResultRenderCopyText(const UiState& state) {
    if (!state.hasView) return std::wstring();

    std::wstring text;
    if (state.view.useFill) {
        for (size_t i = 0; i < state.view.fill.rows.size(); ++i) {
            const core::SlotRow& row = state.view.fill.rows[i];
            if (!text.empty()) text += L"\n";
            text += SlotText(row);
            text += L"\t";
            text += row.entryIndex >= 0
                        ? state.view.sorted[static_cast<size_t>(row.entryIndex)].display
                        : std::wstring(kGapText);
        }
        return text;
    }

    for (size_t i = 0; i < state.view.sorted.size(); ++i) {
        if (!text.empty()) text += L"\n";
        text += state.view.sorted[i].display;
    }
    return text;
}

std::wstring ResultRenderGapText(const UiState& state, size_t* gapCount) {
    if (gapCount != nullptr) *gapCount = 0;
    if (!state.hasView || !state.view.useFill) return std::wstring();

    std::wstring text;
    size_t count = 0;
    for (size_t i = 0; i < state.view.fill.rows.size(); ++i) {
        const core::SlotRow& row = state.view.fill.rows[i];
        if (row.entryIndex >= 0) continue;
        if (!text.empty()) text += L"\n";
        text += SlotText(row);
        ++count;
    }
    if (gapCount != nullptr) *gapCount = count;
    return text;
}

}  // namespace ui
