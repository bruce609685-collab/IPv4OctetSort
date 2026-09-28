#include "ui/menu_bar.h"

#include "ui/control_ids.h"

namespace ui {
namespace {

constexpr int kMenuResourceId = 100;   // IDR_MAIN_MENU
constexpr int kAccelResourceId = 102;  // IDR_ACCEL

void EnableItem(HWND window, int command, bool enabled) {
    HMENU menu = GetMenu(window);
    if (menu == nullptr) return;
    EnableMenuItem(menu, static_cast<UINT>(command), enabled ? MF_ENABLED : MF_GRAYED);
}

}  // namespace

void MenuBarAttach(HWND window, HINSTANCE instance) {
    HMENU menu = LoadMenuW(instance, MAKEINTRESOURCEW(kMenuResourceId));
    if (menu != nullptr) SetMenu(window, menu);
}

bool MenuBarTranslate(HWND window, MSG* message) {
    static HACCEL accel =
        LoadAcceleratorsW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(kAccelResourceId));
    if (accel == nullptr) return false;
    return TranslateAcceleratorW(window, accel, message) != 0;
}

void MenuBarEnableItems(HWND window, bool hasValid, bool sorted, bool fillChecked) {
    EnableItem(window, kCmdAsc, hasValid);
    EnableItem(window, kCmdDesc, hasValid);
    EnableItem(window, kCmdCopyResult, sorted);
    // FR-9：「仅复制空缺地址」在未勾选缺位填充时仍可点击，由状态栏给出提示
    EnableItem(window, kCmdCopyGaps, sorted);
    (void)fillChecked;

    HMENU menu = GetMenu(window);
    if (menu != nullptr) DrawMenuBar(window);
}

}  // namespace ui
