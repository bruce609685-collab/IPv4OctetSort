#include "ui/layout.h"

// 布局计算与界面字体。
// 尺寸取自文档 §5.1 / §5.2 与预览版（.app / .panel / .app-body 的间距）。
namespace ui {
namespace {

constexpr int kButtonWidthSort = 68;   // 升序 / 降序（v0.3 缩窄，原 84）
constexpr int kButtonWidthWide = 88;   // 检查更新 / 语言（右侧一组同宽）
constexpr int kButtonWidthCopy = 84;   // 复制结果
constexpr int kButtonWidthClear = 56;  // 清空
constexpr int kSegLabelWidth = 52;
constexpr int kRadioWidth = 36;
constexpr int kCheckWidth = 86;
constexpr int kPanelPad = 12;   // 面板内边距（对应预览版 .panel-body padding）
constexpr int kBodyTopPad = 8;  // 标题行与面板内容之间的间距
constexpr int kToolbarPad = 11; // 工具栏上下留白
constexpr int kLabelWidth = 40; // 「输入」「结果」标题文字宽度
constexpr int kStatWidth = 280; // 面板标题行右侧统计区宽度

RECT MakeRect(int x, int y, int width, int height) {
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + width;
    rect.bottom = y + height;
    return rect;
}

int SystemDpi() {
    HDC dc = GetDC(nullptr);
    const int dpi = dc != nullptr ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc != nullptr) ReleaseDC(nullptr, dc);
    return dpi;
}

// 按钮 / 单选 / 复选的标准高度：9pt 文字 + 内边距
int ControlHeight() {
    return MulDiv(9, SystemDpi(), 72) + 12;
}

HFONT CreateFontFor(const wchar_t* face) {
    return CreateFontW(-MulDiv(9, SystemDpi(), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
}

}  // namespace

int DefaultClientHeight() {
    const int controlHeight = ControlHeight();
    const int toolbarHeight = controlHeight + kToolbarPad * 2;
    const int headHeight = controlHeight + kHeadExtra;
    const int inputPanelHeight = headHeight + kBodyTopPad + kInputEditHeight + kInputHintGap +
                                 kInputHintHeight + kPanelPad;
    const int resultPanelHeight = headHeight + kBodyTopPad + kResultListDefault;

    return toolbarHeight + kMargin + inputPanelHeight + kPanelGap + kKeyHintHeight + kPanelGap +
           resultPanelHeight + kPanelGap + kStatusBarHeight;
}

LayoutMetrics ComputeLayout(int clientWidth, int clientHeight, int faultsHeight, bool hintVisible) {
    LayoutMetrics m;
    m.clientWidth = clientWidth;
    m.clientHeight = clientHeight;
    m.controlHeight = ControlHeight();

    const int controlHeight = m.controlHeight;
    const int toolbarHeight = controlHeight + kToolbarPad * 2;
    const int headHeight = controlHeight + kHeadExtra;
    const int panelLeft = kMargin;
    const int panelWidth = clientWidth - 2 * kMargin;
    const int headTop = (headHeight - controlHeight) / 2;

    // ---- 工具栏（左簇：排序 + 段位 + 两个复选；右簇：语言 + 检查更新） ----
    // v0.3：排序按钮缩窄为 68 px，左侧簇整体左移；语言下拉位于缺位填充与检查更新之间。
    int x = panelLeft;
    const int rowTop = (toolbarHeight - controlHeight) / 2;
    m.btnAsc = MakeRect(x, rowTop, kButtonWidthSort, controlHeight);
    x += kButtonWidthSort + 6;
    m.btnDesc = MakeRect(x, rowTop, kButtonWidthSort, controlHeight);
    x += kButtonWidthSort + 12;

    m.segLabel = MakeRect(x, rowTop + 4, kSegLabelWidth, controlHeight - 8);
    x += kSegLabelWidth + 2;
    for (int i = 0; i < 4; ++i) {
        m.radio[i] = MakeRect(x, rowTop, kRadioWidth, controlHeight);
        x += kRadioWidth + 2;
    }
    x += 8;

    m.chkDedupe = MakeRect(x, rowTop, kCheckWidth, controlHeight);
    x += kCheckWidth + 6;
    m.chkFill = MakeRect(x, rowTop, kCheckWidth, controlHeight);

    m.btnUpdate = MakeRect(clientWidth - panelLeft - kButtonWidthWide, rowTop, kButtonWidthWide,
                           controlHeight);
    m.btnLang = MakeRect(m.btnUpdate.left - 6 - kButtonWidthWide, rowTop, kButtonWidthWide,
                         controlHeight);

    // ---- 输入面板 ----
    int y = toolbarHeight + kMargin;
    int panelHeight = headHeight + kBodyTopPad + kInputEditHeight + kInputHintGap +
                      kInputHintHeight + kPanelPad;
    if (faultsHeight > 0) panelHeight += faultsHeight + kFaultsGap;

    m.inputPanel = MakeRect(panelLeft, y, panelWidth, panelHeight);
    m.inputTitle = MakeRect(panelLeft + kPanelPad, y + 6, kLabelWidth, headHeight - 12);
    m.btnClear = MakeRect(panelLeft + kPanelPad + kLabelWidth + 4, y + headTop, kButtonWidthClear,
                          controlHeight);
    m.statIn = MakeRect(panelLeft + panelWidth - kPanelPad - kStatWidth, y + 6, kStatWidth,
                        headHeight - 12);
    m.inputEdit = MakeRect(panelLeft + kPanelPad, y + headHeight + kBodyTopPad,
                           panelWidth - 2 * kPanelPad, kInputEditHeight);
    // FR-1.7：粘贴方式提示常驻输入框下方
    m.inputHint = MakeRect(m.inputEdit.left, m.inputEdit.bottom + kInputHintGap,
                           m.inputEdit.right - m.inputEdit.left, kInputHintHeight);
    if (faultsHeight > 0) {
        m.faults = MakeRect(m.inputHint.left, m.inputHint.bottom + kFaultsGap,
                            m.inputHint.right - m.inputHint.left, faultsHeight);
    } else {
        m.faults = MakeRect(0, 0, 0, 0);
    }

    // ---- 段位判定说明（无有效地址时不占位） ----
    y = m.inputPanel.bottom + kPanelGap;
    if (hintVisible) {
        m.keyHint = MakeRect(panelLeft, y, panelWidth, kKeyHintHeight);
        y = m.keyHint.bottom + kPanelGap;
    } else {
        m.keyHint = MakeRect(0, 0, 0, 0);
    }

    // ---- 结果面板：结果列表区吸收全部垂直增量（§5.1 实现要点） ----
    const int resultTop = y;
    const int resultBottom = clientHeight - kStatusBarHeight - kPanelGap;
    int listHeight = resultBottom - resultTop - headHeight - kBodyTopPad;
    if (listHeight < kListMinHeight) listHeight = kListMinHeight;

    m.resultPanel = MakeRect(panelLeft, resultTop, panelWidth, headHeight + kBodyTopPad + listHeight);
    m.resultTitle = MakeRect(panelLeft + kPanelPad, resultTop + 6, kLabelWidth, headHeight - 12);
    m.btnCopy = MakeRect(panelLeft + kPanelPad + kLabelWidth + 4, resultTop + headTop,
                         kButtonWidthCopy, controlHeight);
    // FR-6.2：「已复制」提示始终占位，避免按钮位置抖动
    m.copyMsg = MakeRect(m.btnCopy.right + 8, resultTop + 6, 60, headHeight - 12);
    m.statOut = MakeRect(panelLeft + panelWidth - kPanelPad - kStatWidth - 40, resultTop + 6,
                         kStatWidth + 40, headHeight - 12);
    m.resultList = MakeRect(panelLeft + 1, resultTop + headHeight, panelWidth - 2,
                            m.resultPanel.bottom - (resultTop + headHeight) - 1);

    // ---- 状态栏 ----
    m.statusBar = MakeRect(0, clientHeight - kStatusBarHeight, clientWidth, kStatusBarHeight);

    return m;
}

HFONT CreateUiFont() {
    return CreateFontFor(L"Microsoft YaHei");
}

HFONT CreateMonoFont() {
    // 预览版为 Cascadia Mono（Win10 起才有），此处取 Win7 起即可用的 Consolas
    return CreateFontFor(L"Consolas");
}

}  // namespace ui
