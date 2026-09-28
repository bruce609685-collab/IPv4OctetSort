#pragma once

// 全部控件 ID、菜单命令 ID 与定时器 ID 的集中定义。
// 菜单命令 ID、关于对话框控件 ID 必须与 src/resource/app.rc 保持一致。
namespace ui {

enum ControlId {
    // 工具栏
    kIdBtnAsc = 1001,
    kIdBtnDesc = 1002,
    kIdSegLabel = 1003,
    kIdRadioA = 1010,
    kIdRadioB = 1011,
    kIdRadioC = 1012,
    kIdRadioD = 1013,
    kIdChkDedupe = 1020,
    kIdChkFill = 1021,
    kIdBtnUpdate = 1030,

    // 输入面板
    kIdInputTitle = 1100,
    kIdBtnClear = 1101,
    kIdStatIn = 1102,
    kIdInputEdit = 1103,
    kIdInputHint = 1105,

    // 段位判定提示
    kIdKeyHint = 1200,

    // 结果面板
    kIdResultTitle = 1300,
    kIdBtnCopy = 1301,
    kIdCopyMsg = 1302,
    kIdStatOut = 1303,
    kIdResultList = 1304,

    // 状态栏
    kIdStatusBar = 1400,
};

// 菜单命令（对应 app.rc 中的 IDM_*）
enum MenuCmd {
    kCmdClear = 2001,
    kCmdPasteHint = 2002,
    kCmdExit = 2003,
    kCmdCopyResult = 2004,
    kCmdCopyGaps = 2005,
    kCmdAsc = 2006,
    kCmdDesc = 2007,
    kCmdAbout = 2009,
};

// 关于对话框控件（对应 app.rc 中的 IDC_ABOUT_*）
enum AboutId {
    kIdAboutNameCn = 3001,
    kIdAboutNameEn = 3002,
    kIdAboutVersion = 3003,
    kIdAboutDate = 3004,
};

// 定时器
enum TimerId {
    kTimerCopyMsg = 1,  // FR-6.2：复制提示 1.6 秒后隐藏
};

// 定时器间隔（毫秒）
constexpr unsigned int kCopyMsgMilliseconds = 1600;

}  // namespace ui
