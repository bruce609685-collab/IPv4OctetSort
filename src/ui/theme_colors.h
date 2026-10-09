#pragma once

#include <windows.h>

// 最小自绘（§3.3 第 5 条）所需的配色，一律取自系统，集中于此便于统一调整。
// 自绘仅出现在三处：排序按钮高亮、结果列表空缺着色、无效项标签。
namespace ui {

struct ThemeColors {
    COLORREF highlight;      // 排序按钮高亮底：COLOR_HIGHLIGHT
    COLORREF highlightEdge;  // 高亮按钮边框：高亮色加深
    COLORREF highlightText;  // 高亮按钮文字：COLOR_HIGHLIGHTTEXT

    COLORREF infoBack;  // 空缺标记 / 段位提示底：COLOR_INFOBK
    COLORREF infoText;  // 其上文字：COLOR_INFOTEXT

    COLORREF stripeRowBack;  // 结果列表偶数行底色：浅黄（固定取值，保证肉眼可分辨）
    COLORREF dimText;     // 序号列等弱化文字：COLOR_GRAYTEXT

    COLORREF faultBack;    // 无效项标签底（固定浅红）
    COLORREF faultText;    // 无效项标签文字（固定深红）
    COLORREF faultBorder;  // 无效项标签边框

    COLORREF appBack;       // 窗口背景：COLOR_BTNFACE
    COLORREF panelBack;     // 面板底：COLOR_WINDOW
    COLORREF panelHeadBack; // 面板标题行底：COLOR_BTNFACE
    COLORREF panelBorder;   // 面板边框：COLOR_3DSHADOW

    COLORREF buttonFace;   // 非高亮按钮底：COLOR_BTNFACE
    COLORREF buttonBorder; // 非高亮按钮边框：COLOR_3DSHADOW
    COLORREF buttonText;   // 非高亮按钮文字：COLOR_BTNTEXT
    COLORREF disabledText; // 禁用文字：COLOR_GRAYTEXT
};

// 按百分比混合两个颜色（percentA 为第一个颜色的权重）
inline COLORREF MixColor(COLORREF a, COLORREF b, int percentA) {
    const int weightA = percentA;
    const int weightB = 100 - percentA;
    const int red = (GetRValue(a) * weightA + GetRValue(b) * weightB) / 100;
    const int green = (GetGValue(a) * weightA + GetGValue(b) * weightB) / 100;
    const int blue = (GetBValue(a) * weightA + GetBValue(b) * weightB) / 100;
    return RGB(red, green, blue);
}

inline const ThemeColors& Colors() {
    static const ThemeColors colors = [] {
        ThemeColors c;
        c.highlight = GetSysColor(COLOR_HIGHLIGHT);
        c.highlightEdge = MixColor(GetSysColor(COLOR_HIGHLIGHT), RGB(0, 0, 0), 75);
        c.highlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);

        c.infoBack = GetSysColor(COLOR_INFOBK);
        c.infoText = GetSysColor(COLOR_INFOTEXT);

        c.stripeRowBack = RGB(255, 246, 204);  // 浅黄斑马纹（比信息栏色更深，确保看得清）
        c.dimText = GetSysColor(COLOR_GRAYTEXT);

        c.faultBack = RGB(253, 231, 233);
        c.faultText = RGB(196, 43, 28);
        c.faultBorder = RGB(241, 192, 196);

        c.appBack = GetSysColor(COLOR_BTNFACE);
        c.panelBack = GetSysColor(COLOR_WINDOW);
        c.panelHeadBack = GetSysColor(COLOR_BTNFACE);
        c.panelBorder = GetSysColor(COLOR_3DSHADOW);

        c.buttonFace = GetSysColor(COLOR_BTNFACE);
        c.buttonBorder = GetSysColor(COLOR_3DSHADOW);
        c.buttonText = GetSysColor(COLOR_BTNTEXT);
        c.disabledText = GetSysColor(COLOR_GRAYTEXT);
        return c;
    }();
    return colors;
}

}  // namespace ui
