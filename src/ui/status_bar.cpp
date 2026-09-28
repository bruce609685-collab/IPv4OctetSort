#include "ui/status_bar.h"

#include <commctrl.h>

#include "ui/control_ids.h"

namespace ui {

void StatusBarCreate(StatusBarControls& controls, HWND parent, HINSTANCE instance, HFONT font) {
    controls.bar = CreateWindowExW(0, STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                   parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdStatusBar)),
                                   instance, nullptr);
    if (font != nullptr) SendMessageW(controls.bar, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void StatusBarSetText(const StatusBarControls& controls, const std::wstring& left,
                      const std::wstring& middle, const std::wstring& right) {
    if (controls.bar == nullptr) return;
    SendMessageW(controls.bar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(left.c_str()));
    SendMessageW(controls.bar, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(middle.c_str()));
    SendMessageW(controls.bar, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(right.c_str()));
}

void StatusBarResize(const StatusBarControls& controls, int clientWidth) {
    if (controls.bar == nullptr) return;

    int parts[3];
    parts[0] = clientWidth * 34 / 100;
    parts[1] = clientWidth * 62 / 100;
    parts[2] = -1;  // 余下全部
    SendMessageW(controls.bar, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(parts));
    SendMessageW(controls.bar, WM_SIZE, 0, 0);
}

}  // namespace ui
