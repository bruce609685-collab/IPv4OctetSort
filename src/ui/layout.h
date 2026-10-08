#pragma once

#include <windows.h>

// 固定宽度（780 px，含边框）布局计算（§5.1 / §5.2）。
// 垂直拉伸产生的增量全部由结果列表区吸收。
namespace ui {

constexpr int kWindowWidth = 780;      // §5.1：窗口宽固定 780 px（含边框）
constexpr int kMinWindowHeight = 640;  // §5.1：最小高度 640 px
constexpr int kMargin = 12;            // 面板左右边距
constexpr int kHeadExtra = 6;          // 面板标题行比控件高出的部分
constexpr int kInputEditHeight = 168;  // §5.4：输入框高度固定
constexpr int kInputHintHeight = 18;   // FR-1.7 输入框下方的粘贴提示行
constexpr int kInputHintGap = 6;       // 输入框与提示行的间距
constexpr int kFaultsGap = 8;          // 输入框与无效项标签的间距
constexpr int kKeyHintHeight = 34;     // FR-3.4 判定说明条
constexpr int kStatusBarHeight = 24;
constexpr int kResultListDefault = 290;  // 结果列表默认高度
constexpr int kListMinHeight = 120;      // 结果列表最小高度
constexpr int kPanelGap = 12;            // 面板之间与末尾留白

struct LayoutMetrics {
    int clientWidth = 0;
    int clientHeight = 0;
    int controlHeight = 0;  // 按钮 / 单选 / 复选 / 标题行内控件的标准高度

    RECT btnAsc, btnDesc, segLabel;
    RECT radio[4];
    RECT chkDedupe, chkFill, btnLang, btnUpdate;

    RECT inputPanel, inputTitle, btnClear, statIn, inputEdit, inputHint, faults;

    RECT keyHint;

    RECT resultPanel, resultTitle, btnCopy, copyMsg, statOut, resultList;

    RECT statusBar;
};

// 依据客户区尺寸算出各控件矩形。
// faultsHeight > 0 时在输入框下方留出无效项标签区；hintVisible 为假时判定说明条不占高度。
LayoutMetrics ComputeLayout(int clientWidth, int clientHeight, int faultsHeight, bool hintVisible);

// 初始客户区高度（按内容确定，§5.1）
int DefaultClientHeight();

// 界面字体与等宽字体（由主窗口创建并持有，供各面板使用）
HFONT CreateUiFont();
HFONT CreateMonoFont();

}  // namespace ui
