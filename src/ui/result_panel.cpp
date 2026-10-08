#include "ui/result_panel.h"

#include <commctrl.h>

#include "ui/control_ids.h"
#include "ui/i18n.h"

namespace ui {
namespace {

HWND CreateChild(HWND parent, HINSTANCE instance, const wchar_t* className, const wchar_t* text,
                 DWORD style, DWORD exStyle, int id) {
    return CreateWindowExW(exStyle, className, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0,
                           parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance,
                           nullptr);
}

void InsertColumn(HWND list, int index, const wchar_t* text, int width, int format) {
    LVCOLUMNW column = {};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
    column.pszText = const_cast<wchar_t*>(text);
    column.cx = width;
    column.fmt = format;
    column.iSubItem = index;
    ListView_InsertColumn(list, index, &column);
}

}  // namespace

void ResultPanelCreate(ResultPanelControls& controls, HWND parent, HINSTANCE instance, HFONT font,
                       HFONT monoFont) {
    controls.title =
        CreateChild(parent, instance, L"STATIC", T(StrId::ResultTitle), SS_LEFT, 0, kIdResultTitle);
    controls.btnCopy =
        CreateChild(parent, instance, L"BUTTON", T(StrId::BtnCopy), WS_TABSTOP | BS_PUSHBUTTON, 0,
                    kIdBtnCopy);
    // FR-6.2：提示文字始终占位，仅切换可见性，避免按钮位置抖动
    controls.copyMsg =
        CreateChild(parent, instance, L"STATIC", T(StrId::CopyMsg), SS_LEFT, 0, kIdCopyMsg);
    controls.statOut = CreateChild(parent, instance, L"STATIC", L"—", SS_RIGHT, 0, kIdStatOut);
    controls.list = CreateChild(parent, instance, WC_LISTVIEWW, L"",
                                WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS |
                                    LVS_NOSORTHEADER,
                                WS_EX_CLIENTEDGE, kIdResultList);

    SendMessageW(controls.list, LVM_SETEXTENDEDLISTVIEWSTYLE,
                 LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
                 LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    SendMessageW(controls.title, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SendMessageW(controls.btnCopy, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SendMessageW(controls.copyMsg, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SendMessageW(controls.statOut, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SendMessageW(controls.list, WM_SETFONT, reinterpret_cast<WPARAM>(monoFont), TRUE);

    ResultPanelShowCopyMsg(controls, false);
    ResultPanelSetCopyEnabled(controls, false);
}

void ResultPanelSetCopyEnabled(const ResultPanelControls& controls, bool enabled) {
    EnableWindow(controls.btnCopy, enabled ? TRUE : FALSE);
}

void ResultPanelShowCopyMsg(const ResultPanelControls& controls, bool visible) {
    ShowWindow(controls.copyMsg, visible ? SW_SHOW : SW_HIDE);
}

void ResultPanelSetStat(const ResultPanelControls& controls, const std::wstring& text) {
    SetWindowTextW(controls.statOut, text.c_str());
}

void ResultPanelSetColumns(const ResultPanelControls& controls, ResultView mode, int listWidth) {
    HWND list = controls.list;
    while (ListView_DeleteColumn(list, 0)) {
    }

    const int indexWidth = 44;
    if (mode == ResultView::Empty) {
        InsertColumn(list, 0, L"", listWidth > 0 ? listWidth - 4 : 400, LVCFMT_LEFT);
        return;
    }

    InsertColumn(list, 0, L"#", indexWidth, LVCFMT_RIGHT);
    if (mode == ResultView::Plain) {
        InsertColumn(list, 1, T(StrId::ColAddress), listWidth - indexWidth - 8, LVCFMT_LEFT);
        return;
    }

    const int slotWidth = listWidth >= 640 ? 190 : (listWidth - indexWidth) / 2;
    InsertColumn(list, 1, T(StrId::ColSequence), slotWidth, LVCFMT_LEFT);
    InsertColumn(list, 2, T(StrId::ColYourList), listWidth - indexWidth - slotWidth - 12,
                 LVCFMT_LEFT);
}

void ResultPanelApplyLanguage(const ResultPanelControls& controls) {
    SetWindowTextW(controls.title, T(StrId::ResultTitle));
    SetWindowTextW(controls.btnCopy, T(StrId::BtnCopy));
    SetWindowTextW(controls.copyMsg, T(StrId::CopyMsg));
}

}  // namespace ui
