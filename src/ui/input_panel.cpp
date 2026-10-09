#include "ui/input_panel.h"

#include "ui/control_ids.h"
#include "ui/i18n.h"
#include "ui/theme_colors.h"

namespace ui {
namespace {

constexpr int kTagGapX = 6;      // 标签水平间距
constexpr int kTagGapY = 6;      // 标签行间距
constexpr int kTagPadX = 8;      // 标签内左右留白
constexpr int kTagHeight = 20;   // 单个标签高度
constexpr int kTagMaxRows = 3;   // 最多占几行，避免把面板撑得过高

// FR-1.5：最多显示 6 条，超出部分合并为一条
std::vector<std::wstring> FaultLabels(const std::vector<core::InvalidItem>& invalid) {
    std::vector<std::wstring> labels;
    const size_t shown =
        invalid.size() < static_cast<size_t>(core::kMaxFaultsShown) ? invalid.size()
                                                                   : static_cast<size_t>(core::kMaxFaultsShown);
    for (size_t i = 0; i < shown; ++i) {
        const std::wstring why = ReasonText(invalid[i].reasonKind, invalid[i].reasonArg);
        labels.push_back(
            Tf(StrId::FaultFmt, invalid[i].line, invalid[i].raw.c_str(), why.c_str()));
    }
    if (invalid.size() > shown) {
        labels.push_back(Tf(StrId::FaultMore, static_cast<int>(invalid.size() - shown)));
    }
    return labels;
}

// 流式排布（左→右，超出宽度换行），同时给出所需高度
std::vector<RECT> LayoutTags(HDC dc, const std::vector<std::wstring>& labels, int width,
                             int* neededHeight) {
    std::vector<RECT> rects;
    int x = 0;
    int y = 0;
    int row = 0;

    for (size_t i = 0; i < labels.size(); ++i) {
        SIZE size = {0, 0};
        GetTextExtentPoint32W(dc, labels[i].c_str(), static_cast<int>(labels[i].size()), &size);
        const int tagWidth = size.cx + kTagPadX * 2;

        if (x > 0 && x + tagWidth > width) {
            x = 0;
            y += kTagHeight + kTagGapY;
            ++row;
        }
        if (row >= kTagMaxRows) break;

        RECT rect;
        rect.left = x;
        rect.top = y;
        rect.right = x + tagWidth;
        rect.bottom = y + kTagHeight;
        rects.push_back(rect);

        x += tagWidth + kTagGapX;
    }

    *neededHeight = rects.empty() ? 0 : y + kTagHeight;
    return rects;
}

HWND CreateChild(HWND parent, HINSTANCE instance, const wchar_t* className, const wchar_t* text,
                 DWORD style, DWORD exStyle, int id) {
    return CreateWindowExW(exStyle, className, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0,
                           parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance,
                           nullptr);
}

}  // namespace

void InputPanelCreate(InputPanelControls& controls, HWND parent, HINSTANCE instance, HFONT font,
                      HFONT monoFont) {
    controls.title =
        CreateChild(parent, instance, L"STATIC", T(StrId::InputTitle), SS_LEFT, 0, kIdInputTitle);
    controls.btnClear =
        CreateChild(parent, instance, L"BUTTON", T(StrId::BtnClear), WS_TABSTOP | BS_PUSHBUTTON, 0,
                    kIdBtnClear);
    controls.statIn = CreateChild(parent, instance, L"STATIC", Tf(StrId::StatEntries, 0).c_str(),
                                  SS_RIGHT, 0, kIdStatIn);
    controls.edit = CreateChild(parent, instance, L"EDIT", L"",
                                WS_TABSTOP | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
                                    ES_WANTRETURN | WS_VSCROLL,
                                WS_EX_CLIENTEDGE, kIdInputEdit);
    // FR-1.7：粘贴方式提示，与界面其它文字同字体（非等宽），颜色在 WM_CTLCOLORSTATIC 中弱化
    controls.hint = CreateChild(parent, instance, L"STATIC", T(StrId::InputHint), SS_LEFT, 0,
                                kIdInputHint);

    // 输入框等宽字体（§5.5：地址与列表使用等宽字体）
    SendMessageW(controls.edit, WM_SETFONT, reinterpret_cast<WPARAM>(monoFont), TRUE);
    SendMessageW(controls.title, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SendMessageW(controls.btnClear, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SendMessageW(controls.statIn, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SendMessageW(controls.hint, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

std::wstring InputPanelGetText(const InputPanelControls& controls) {
    const int length = GetWindowTextLengthW(controls.edit);
    if (length <= 0) return std::wstring();

    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    const int copied = GetWindowTextW(controls.edit, &text[0], length + 1);
    text.resize(copied > 0 ? static_cast<size_t>(copied) : 0);
    return text;
}

void InputPanelSetText(const InputPanelControls& controls, const std::wstring& text) {
    SetWindowTextW(controls.edit, text.c_str());
}

void InputPanelUpdateStat(const InputPanelControls& controls, size_t validCount, size_t invalidCount,
                          bool deduped) {
    std::wstring stat = Tf(StrId::StatEntries, static_cast<int>(validCount));
    if (invalidCount > 0) {
        stat += L" · " + Tf(StrId::StatUnrecognized, static_cast<int>(invalidCount));
    }
    if (deduped) stat += std::wstring(L" · ") + T(StrId::StatMerged);
    SetWindowTextW(controls.statIn, stat.c_str());
}

void InputPanelApplyLanguage(const InputPanelControls& controls) {
    SetWindowTextW(controls.title, T(StrId::InputTitle));
    SetWindowTextW(controls.btnClear, T(StrId::BtnClear));
    SetWindowTextW(controls.hint, T(StrId::InputHint));
}

int FaultsMeasure(const std::vector<core::InvalidItem>& invalid, int width, HFONT monoFont) {
    if (invalid.empty() || width <= 0) return 0;

    HDC dc = GetDC(nullptr);
    if (dc == nullptr) return kTagHeight;
    HGDIOBJ oldFont = monoFont != nullptr ? SelectObject(dc, monoFont) : nullptr;

    int height = 0;
    LayoutTags(dc, FaultLabels(invalid), width, &height);

    if (oldFont != nullptr) SelectObject(dc, oldFont);
    ReleaseDC(nullptr, dc);
    return height;
}

void FaultsPaint(HDC dc, const RECT& area, const std::vector<core::InvalidItem>& invalid,
                 HFONT monoFont) {
    if (invalid.empty()) return;

    HGDIOBJ oldFont = monoFont != nullptr ? SelectObject(dc, monoFont) : nullptr;
    const std::vector<std::wstring> labels = FaultLabels(invalid);
    int height = 0;
    const std::vector<RECT> rects = LayoutTags(dc, labels, area.right - area.left, &height);
    const ThemeColors& colors = Colors();

    for (size_t i = 0; i < rects.size() && i < labels.size(); ++i) {
        RECT rect = rects[i];
        OffsetRect(&rect, area.left, area.top);

        HBRUSH brush = CreateSolidBrush(colors.faultBack);
        FillRect(dc, &rect, brush);
        DeleteObject(brush);
        brush = CreateSolidBrush(colors.faultBorder);
        FrameRect(dc, &rect, brush);
        DeleteObject(brush);

        RECT textRect = rect;
        textRect.left += kTagPadX - 2;
        textRect.right -= kTagPadX - 2;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, colors.faultText);
        DrawTextW(dc, labels[i].c_str(), -1, &textRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    if (oldFont != nullptr) SelectObject(dc, oldFont);
}

}  // namespace ui
