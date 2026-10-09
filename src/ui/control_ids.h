#pragma once

// 全部控件 ID、菜单命令 ID 与定时器 ID 的集中定义。
// 菜单自 v0.3 起由 menu_bar 代码构建（多语言，FR-9/FR-12），
// 仅加速键表与关于对话框仍在 app.rc 中，其中的 IDM_CLEAR / IDM_COPY_RESULT
// 与本文件的菜单命令值保持一致。
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
    kIdBtnLang = 1031,

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

// 菜单命令（2001/2004 与 app.rc 加速键表一致）
enum MenuCmd {
    kCmdClear = 2001,
    kCmdPasteHint = 2002,
    kCmdExit = 2003,
    kCmdCopyResult = 2004,
    kCmdCopyGaps = 2005,
    kCmdAsc = 2006,
    kCmdDesc = 2007,
    kCmdAbout = 2009,
    // 语言下拉（FR-12）：命令 ID 自 kCmdLangFirst 连续编号，第 i 项 = kCmdLangFirst + i，
    // 与 ui/i18n 的 Languages() 下标对应（v0.4 起支持任意数量语言）
    kCmdLangFirst = 2010,
};

// 关于对话框控件（对应 app.rc 中的 IDC_ABOUT_*；3011+ 为左侧字段标签，
// v0.3 起标签文字也随语言切换，故不再用 IDC_STATIC）
enum AboutId {
    kIdAboutNameCn = 3001,
    kIdAboutNameEn = 3002,
    kIdAboutVersion = 3003,
    kIdAboutDate = 3004,
    kIdAboutLblNameCn = 3011,
    kIdAboutLblNameEn = 3012,
    kIdAboutLblVersion = 3013,
    kIdAboutLblDate = 3014,
};

// 定时器
enum TimerId {
    kTimerCopyMsg = 1,  // FR-6.2：复制提示 1.6 秒后隐藏
};

// 定时器间隔（毫秒）
constexpr unsigned int kCopyMsgMilliseconds = 1600;

}  // namespace ui
