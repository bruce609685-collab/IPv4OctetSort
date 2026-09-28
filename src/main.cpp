// 程序入口（文档 §8.4.5）：
// DPI 声明 → 单实例检测 → 创建主窗口 → 消息循环 → 退出清理
#include <windows.h>

#include <commctrl.h>

#include <string>

#include "app/app_info.h"
#include "app/dpi_aware.h"
#include "app/single_instance.h"
#include "ui/main_window.h"
#include "util/logger.h"

namespace {

// 日志文件位置取自主程序所在目录（NFR-4）
std::wstring ExecutablePath() {
    wchar_t buffer[MAX_PATH] = {0};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::wstring(buffer, length);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    app::EnableDpiAwareness();
    util::InitLogger(ExecutablePath());
    util::LogLine(L"INFO", L"启动 " APP_NAME_EN L" v" APP_VERSION);

    if (!app::ClaimSingleInstance(ui::MainWindowClassName())) {
        util::LogLine(L"INFO", L"已有实例在运行，本次启动退出");
        util::ShutdownLogger();
        return 0;
    }

    INITCOMMONCONTROLSEX commonControls = {};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&commonControls);

    if (!ui::CreateMainWindow(instance, showCommand)) {
        util::LogLine(L"ERROR", L"主窗口创建失败");
        MessageBoxW(nullptr, L"程序启动失败：主窗口创建失败。", APP_NAME_EN, MB_OK | MB_ICONERROR);
        util::ShutdownLogger();
        return 1;
    }

    const int exitCode = ui::RunMessageLoop();

    util::LogLine(L"INFO", L"退出");
    util::ShutdownLogger();
    return exitCode;
}
