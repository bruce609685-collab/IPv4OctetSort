#include "ui/about_dialog.h"

#include <string>

#include "app/app_info.h"
#include "ui/control_ids.h"
#include "ui/i18n.h"

namespace ui {
namespace {

constexpr int kAboutDialogId = 101;  // IDD_ABOUT

void SetField(HWND dialog, int id, const std::wstring& text) {
    SetDlgItemTextW(dialog, id, text.c_str());
}

INT_PTR CALLBACK AboutDialogProc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_INITDIALOG: {
            // 标题、字段标签与确定按钮随语言切换（FR-11）；字段值为产品常量，两种语言相同
            SetWindowTextW(dialog, T(StrId::AboutCaption));
            SetField(dialog, kIdAboutLblNameCn, T(StrId::AboutNameCn));
            SetField(dialog, kIdAboutLblNameEn, T(StrId::AboutNameEn));
            SetField(dialog, kIdAboutLblVersion, T(StrId::AboutVersion));
            SetField(dialog, kIdAboutLblDate, T(StrId::AboutDate));
            SetField(dialog, IDOK, T(StrId::AboutOk));
            SetField(dialog, kIdAboutNameCn, APP_NAME_CN);
            SetField(dialog, kIdAboutNameEn, APP_NAME_EN);
            SetField(dialog, kIdAboutVersion, std::wstring(L"v") + APP_VERSION);
            SetField(dialog, kIdAboutDate, APP_BUILD_DATE_W);
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(dialog, LOWORD(wparam));
                    return TRUE;
                default:
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(dialog, IDCANCEL);
            return TRUE;

        default:
            break;
    }
    (void)lparam;
    return FALSE;
}

}  // namespace

void ShowAboutDialog(HWND owner, HINSTANCE instance) {
    DialogBoxW(instance, MAKEINTRESOURCEW(kAboutDialogId), owner, AboutDialogProc);
}

}  // namespace ui
