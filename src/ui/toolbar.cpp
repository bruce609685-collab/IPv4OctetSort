#include "ui/toolbar.h"

#include "ui/control_ids.h"
#include "ui/i18n.h"
#include "ui/theme_colors.h"
#include "ui/ui_state.h"

namespace ui {
namespace {

HWND CreateChild(HWND parent, HINSTANCE instance, const wchar_t* className, const wchar_t* text,
                 DWORD style, int id) {
    return CreateWindowExW(0, className, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, parent,
                           reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
}

void ApplyFont(HWND control, HFONT font) {
    if (control != nullptr) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

}  // namespace

void ToolbarCreate(ToolbarControls& controls, HWND parent, HINSTANCE instance, HFONT font) {
    controls.btnAsc = CreateChild(parent, instance, L"BUTTON", T(StrId::BtnAsc),
                                  WS_TABSTOP | BS_OWNERDRAW, kIdBtnAsc);
    controls.btnDesc = CreateChild(parent, instance, L"BUTTON", T(StrId::BtnDesc),
                                   WS_TABSTOP | BS_OWNERDRAW, kIdBtnDesc);
    controls.segLabel =
        CreateChild(parent, instance, L"STATIC", T(StrId::SegLabel), SS_LEFT, kIdSegLabel);

    static const wchar_t* const kNames[4] = {L"A", L"B", L"C", L"D"};
    for (int i = 0; i < 4; ++i) {
        // 单选按钮同一组：仅第一个带 WS_GROUP，箭头键可在组内移动
        const DWORD group = (i == 0) ? WS_GROUP : 0;
        controls.radio[i] = CreateChild(parent, instance, L"BUTTON", kNames[i],
                                        WS_TABSTOP | BS_AUTORADIOBUTTON | group, kIdRadioA + i);
    }

    controls.chkDedupe =
        CreateChild(parent, instance, L"BUTTON", T(StrId::ChkDedupe),
                    WS_TABSTOP | WS_GROUP | BS_AUTOCHECKBOX, kIdChkDedupe);
    controls.chkFill = CreateChild(parent, instance, L"BUTTON", T(StrId::ChkFill),
                                   WS_TABSTOP | WS_GROUP | BS_AUTOCHECKBOX, kIdChkFill);
    // FR-11：语言下拉，位于缺位填充与检查更新之间
    const std::wstring langText = LangButtonText();
    controls.btnLang = CreateChild(parent, instance, L"BUTTON", langText.c_str(),
                                   WS_TABSTOP | BS_PUSHBUTTON, kIdBtnLang);
    controls.btnUpdate =
        CreateChild(parent, instance, L"BUTTON", T(StrId::BtnUpdate), WS_TABSTOP | BS_PUSHBUTTON,
                    kIdBtnUpdate);

    ApplyFont(controls.btnAsc, font);
    ApplyFont(controls.btnDesc, font);
    ApplyFont(controls.segLabel, font);
    for (int i = 0; i < 4; ++i) ApplyFont(controls.radio[i], font);
    ApplyFont(controls.chkDedupe, font);
    ApplyFont(controls.chkFill, font);
    ApplyFont(controls.btnLang, font);
    ApplyFont(controls.btnUpdate, font);
}

void ToolbarApplyLanguage(const ToolbarControls& controls) {
    SetWindowTextW(controls.btnAsc, T(StrId::BtnAsc));
    SetWindowTextW(controls.btnDesc, T(StrId::BtnDesc));
    SetWindowTextW(controls.segLabel, T(StrId::SegLabel));
    SetWindowTextW(controls.chkDedupe, T(StrId::ChkDedupe));
    SetWindowTextW(controls.chkFill, T(StrId::ChkFill));
    const std::wstring langText = LangButtonText();
    SetWindowTextW(controls.btnLang, langText.c_str());
    SetWindowTextW(controls.btnUpdate, T(StrId::BtnUpdate));
    // 排序按钮为自绘，文字改变后重绘
    ToolbarSyncSortButtons(controls);
}

void ToolbarSyncSortButtons(const ToolbarControls& controls) {
    if (controls.btnAsc != nullptr) InvalidateRect(controls.btnAsc, nullptr, TRUE);
    if (controls.btnDesc != nullptr) InvalidateRect(controls.btnDesc, nullptr, TRUE);
}

void ToolbarSyncSegments(const ToolbarControls& controls, bool hasData, int diffSeg) {
    for (int i = 0; i < 4; ++i) {
        HWND radio = controls.radio[i];
        if (radio == nullptr) continue;
        const bool isDiff = hasData && i == diffSeg;
        SendMessageW(radio, BM_SETCHECK, isDiff ? BST_CHECKED : BST_UNCHECKED, 0);
        EnableWindow(radio, isDiff);
    }
}

void ToolbarEnableSortButtons(const ToolbarControls& controls, bool enabled) {
    EnableWindow(controls.btnAsc, enabled ? TRUE : FALSE);
    EnableWindow(controls.btnDesc, enabled ? TRUE : FALSE);
}

void ToolbarDrawSortButton(const ToolbarControls& controls, const DRAWITEMSTRUCT& item) {
    const ThemeColors& colors = Colors();
    const bool isAsc = (item.CtlID == static_cast<UINT>(kIdBtnAsc));
    const core::SortDir highlightedDir = isAsc ? core::SortDir::Asc : core::SortDir::Desc;
    const bool highlighted = (State().mode == highlightedDir);
    const bool disabled = (item.itemState & ODS_DISABLED) != 0;
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;

    wchar_t text[64] = {0};
    GetWindowTextW(item.hwndItem, text, 64);

    COLORREF face = colors.buttonFace;
    COLORREF border = colors.buttonBorder;
    COLORREF textColor = disabled ? colors.disabledText : colors.buttonText;

    if (highlighted && !disabled) {
        face = pressed ? MixColor(colors.highlight, RGB(0, 0, 0), 80) : colors.highlight;
        border = colors.highlightEdge;
        textColor = colors.highlightText;
    } else if (pressed) {
        face = MixColor(colors.buttonFace, RGB(0, 0, 0), 92);
    }

    RECT rect = item.rcItem;
    HBRUSH brush = CreateSolidBrush(face);
    FillRect(item.hDC, &rect, brush);
    DeleteObject(brush);

    brush = CreateSolidBrush(border);
    FrameRect(item.hDC, &rect, brush);
    DeleteObject(brush);

    HFONT font = reinterpret_cast<HFONT>(SendMessageW(item.hwndItem, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = font != nullptr ? SelectObject(item.hDC, font) : nullptr;
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, textColor);
    DrawTextW(item.hDC, text, -1, &rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    if (oldFont != nullptr) SelectObject(item.hDC, oldFont);

    if ((item.itemState & ODS_FOCUS) != 0) {
        RECT focus = rect;
        InflateRect(&focus, -3, -3);
        DrawFocusRect(item.hDC, &focus);
    }
    (void)controls;
}

}  // namespace ui
