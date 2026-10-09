#include "ui/main_window.h"

#include <commctrl.h>

#include <cwctype>
#include <string>

#include "app/app_info.h"
#include "core/pipeline.h"
#include "core/segment.h"
#include "ui/about_dialog.h"
#include "ui/control_ids.h"
#include "ui/i18n.h"
#include "ui/input_panel.h"
#include "ui/layout.h"
#include "ui/menu_bar.h"
#include "ui/result_panel.h"
#include "ui/result_render.h"
#include "ui/status_bar.h"
#include "ui/theme_colors.h"
#include "ui/toolbar.h"
#include "ui/ui_state.h"
#include "util/clipboard.h"
#include "util/shell_open.h"

namespace ui {
namespace {

// FR-11：语言切换经消息转交，避免在语言菜单的模态循环里重建菜单
constexpr UINT kMsgApplyLang = WM_APP + 1;

// 主窗口持有的全部界面对象与资源；各面板模块之间不互相调用，由本文件协调
struct MainWindow {
    HINSTANCE instance = nullptr;
    HWND hwnd = nullptr;
    HFONT uiFont = nullptr;
    HFONT monoFont = nullptr;
    HBRUSH appBrush = nullptr;
    HBRUSH panelBrush = nullptr;
    HBRUSH headBrush = nullptr;
    HBRUSH infoBrush = nullptr;
    HBRUSH borderBrush = nullptr;

    ToolbarControls toolbar;
    InputPanelControls input;
    ResultPanelControls result;
    StatusBarControls status;
    HWND keyHint = nullptr;

    LayoutMetrics layout;
    bool suppressChange = false;  // 程序化改动输入框时抑制 EN_CHANGE 递归
};

MainWindow* g_window = nullptr;

MainWindow* FromWindow(HWND hwnd) {
    return reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

void Place(HWND control, const RECT& rect) {
    if (control == nullptr) return;
    SetWindowPos(control, nullptr, rect.left, rect.top, rect.right - rect.left,
                 rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
}

RECT MakeRect(int x, int y, int width, int height) {
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + width;
    rect.bottom = y + height;
    return rect;
}

void SetFontOn(HWND control, HFONT font) {
    if (control != nullptr) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

// 界面字体下发到全部使用界面字体的控件（输入框与结果列表保持等宽字体）
void ApplyUiFont(MainWindow& w, HFONT font) {
    SetFontOn(w.toolbar.btnAsc, font);
    SetFontOn(w.toolbar.btnDesc, font);
    SetFontOn(w.toolbar.segLabel, font);
    for (int i = 0; i < 4; ++i) SetFontOn(w.toolbar.radio[i], font);
    SetFontOn(w.toolbar.chkDedupe, font);
    SetFontOn(w.toolbar.chkFill, font);
    SetFontOn(w.toolbar.btnLang, font);
    SetFontOn(w.toolbar.btnUpdate, font);

    SetFontOn(w.input.title, font);
    SetFontOn(w.input.btnClear, font);
    SetFontOn(w.input.statIn, font);
    SetFontOn(w.input.hint, font);

    SetFontOn(w.keyHint, font);

    SetFontOn(w.result.title, font);
    SetFontOn(w.result.btnCopy, font);
    SetFontOn(w.result.copyMsg, font);
    SetFontOn(w.result.statOut, font);

    SetFontOn(w.status.bar, font);
}

int MeasureTextWidth(HDC dc, HWND control) {
    wchar_t text[160] = {0};
    GetWindowTextW(control, text, 160);
    SIZE size = {0, 0};
    if (!GetTextExtentPoint32W(dc, text, lstrlenW(text), &size)) return 0;
    return size.cx;
}

// FR-12：工具栏按当前语言的实测文字宽度布局。多语言文本长短不一
//（如俄语「По возрастанию」与日语「昇順」），固定像素宽度必然顾此失彼；
// 这里以 9pt 界面字体实测各控件文字宽度，左侧成簇顺序排、右侧语言/更新
// 按钮靠右对齐。窗口宽 880 px 为宽文字语言预留余量（v0.4 由 780 加宽）。
void PlaceToolbar(MainWindow& w, const LayoutMetrics& m) {
    HDC dc = GetDC(w.hwnd);
    HGDIOBJ oldFont = dc != nullptr ? SelectObject(dc, w.uiFont) : nullptr;

    const int top = m.btnAsc.top;
    const int height = m.btnAsc.bottom - m.btnAsc.top;
    const int gap = 6;
    const int innerGap = 2;
    const int clusterGap = 12;

    // widthOf：实测文字宽 + 内边距，夹在 [min, max]
    struct {
        HDC dc;
        int operator()(HWND control, int extra, int minWidth, int maxWidth) {
            int width = MeasureTextWidth(dc, control) + extra;
            if (width < minWidth) width = minWidth;
            if (width > maxWidth) width = maxWidth;
            return width;
        }
    } widthOf = {dc};

    RECT client = {};
    GetClientRect(w.hwnd, &client);

    // ---- 右簇：检查更新 / 语言，靠右对齐 ----
    int right = client.right - kMargin;
    int width = widthOf(w.toolbar.btnUpdate, 24, 84, 220);
    Place(w.toolbar.btnUpdate, MakeRect(right - width, top, width, height));
    right -= width + gap;
    width = widthOf(w.toolbar.btnLang, 24, 84, 220);
    Place(w.toolbar.btnLang, MakeRect(right - width, top, width, height));

    // ---- 左簇：排序 → 段位 → 两个复选 ----
    // 右簇起点即左簇的边界；宽文字语言（俄语等）即便文案精简后也可能逼近
    // 边界，这里兜底：任一控件不得越过右簇左缘（超出时收窄，按钮文字以
    // 省略号收尾，见 ToolbarDrawSortButton 的 DT_END_ELLIPSIS）
    const int leftLimit = right - gap;
    int x = kMargin;
    width = widthOf(w.toolbar.btnAsc, 24, 68, 160);
    Place(w.toolbar.btnAsc, MakeRect(x, top, width, height));
    x += width + gap;
    width = widthOf(w.toolbar.btnDesc, 24, 68, 160);
    Place(w.toolbar.btnDesc, MakeRect(x, top, width, height));
    x += width + clusterGap;

    width = widthOf(w.toolbar.segLabel, 8, 40, 160);
    Place(w.toolbar.segLabel, MakeRect(x, top + 4, width, height - 8));
    x += width + innerGap;
    for (int i = 0; i < 4; ++i) {
        const int radioWidth = m.radio[i].right - m.radio[i].left;
        Place(w.toolbar.radio[i], MakeRect(x, top, radioWidth, height));
        x += radioWidth + innerGap;
    }
    x += clusterGap - innerGap;

    width = widthOf(w.toolbar.chkDedupe, 26, 86, 220);
    if (x + width > leftLimit) width = leftLimit - x;
    if (width >= 40) {
        Place(w.toolbar.chkDedupe, MakeRect(x, top, width, height));
        x += width + gap;
    }
    width = widthOf(w.toolbar.chkFill, 26, 86, 220);
    if (x + width > leftLimit) width = leftLimit - x;
    if (width >= 40) Place(w.toolbar.chkFill, MakeRect(x, top, width, height));

    if (dc != nullptr) {
        SelectObject(dc, oldFont);
        ReleaseDC(w.hwnd, dc);
    }
}

// FR-12：面板标题行同样按实测文字宽度布局。固定像素宽度在窄语言（中文）
// 下浪费、在宽语言（法语 Résultat、俄语 Копировать результат）下截断，
// 与工具栏同一策略：标题 = 文字宽 + 4，按钮 = 文字宽 + 24。
void PlacePanelHeads(MainWindow& w, const LayoutMetrics& m) {
    HDC dc = GetDC(w.hwnd);
    HGDIOBJ oldFont = dc != nullptr ? SelectObject(dc, w.uiFont) : nullptr;

    struct {
        HDC dc;
        int operator()(HWND control, int extra, int minWidth) {
            wchar_t text[160] = {0};
            GetWindowTextW(control, text, 160);
            SIZE size = {0, 0};
            if (!GetTextExtentPoint32W(dc, text, lstrlenW(text), &size)) return minWidth;
            const int width = size.cx + extra;
            return width < minWidth ? minWidth : width;
        }
    } widthOf = {dc};

    // ---- 输入面板标题行：标题 / 清空（统计区右对齐，位置沿用布局计算） ----
    // 按钮跟在标题实测宽度之后，避免宽文字标题（俄语 Rезультат 等）
    // 越过按钮的固定 x 位置而被盖掉词尾
    RECT rect = m.inputTitle;
    rect.right = rect.left + widthOf(w.input.title, 4, 40);
    Place(w.input.title, rect);
    RECT btn = m.btnClear;
    btn.left = rect.right + 4;
    btn.right = btn.left + widthOf(w.input.btnClear, 24, 56);
    Place(w.input.btnClear, btn);
    Place(w.input.statIn, m.statIn);

    // ---- 结果面板标题行：标题 / 复制按钮 / 已复制提示 ----
    rect = m.resultTitle;
    rect.right = rect.left + widthOf(w.result.title, 4, 40);
    Place(w.result.title, rect);
    btn = m.btnCopy;
    btn.left = rect.right + 4;
    btn.right = btn.left + widthOf(w.result.btnCopy, 24, 84);
    Place(w.result.btnCopy, btn);
    RECT copyMsg = m.copyMsg;
    copyMsg.left = btn.right + 8;
    copyMsg.right = copyMsg.left + widthOf(w.result.copyMsg, 8, 60);
    Place(w.result.copyMsg, copyMsg);
    Place(w.result.statOut, m.statOut);

    if (dc != nullptr) {
        SelectObject(dc, oldFont);
        ReleaseDC(w.hwnd, dc);
    }
}

std::wstring DirLabel(const UiState& state) {
    return DirWord(state.mode);
}

std::wstring SegLabel(const UiState& state) {
    if (state.keySeg == core::kNoSegment) return std::wstring();
    return Tf(StrId::BySegFmt, core::kSegmentNames[state.keySeg]);
}

std::wstring SegmentText(const UiState& state) {
    if (state.valid.empty()) return T(StrId::SegDash);
    if (state.diffSeg == core::kNoSegment) return T(StrId::SegNone);
    return Tf(StrId::SegFmt, core::kSegmentNames[state.diffSeg]);
}

// FR-3.4 判定说明文案（整句组句在 i18n.cpp，各语言句式不同）
std::wstring KeyHintText(const UiState& state) {
    if (state.diffSeg == core::kNoSegment) return T(StrId::HintNoDiff);
    const core::DiffPreview preview = core::DiffValues(state.valid, state.diffSeg);
    std::wstring text = preview.head;
    if (preview.truncated) text += L" " + Tf(StrId::DiffMore, preview.total);
    return KeyHintSentence(state.diffSeg, text);
}

void UpdateStatusBar(MainWindow& w, const wchar_t* leftOverride = nullptr) {
    const UiState& state = State();
    std::wstring left;
    std::wstring right;

    if (!state.hasView || state.valid.empty()) {
        left = state.valid.empty() ? T(StrId::StReady) : T(StrId::StReadyPending);
        right = L"—";
    } else if (state.view.useFill) {
        left = T(StrId::StFilledPre) + DirLabel(state) + T(StrId::StClose);
        right = Tf(StrId::FmtSlots, static_cast<int>(state.view.fill.total),
                   static_cast<int>(state.view.fill.gaps));
        if (!SegLabel(state).empty()) right += L" · " + SegLabel(state);
    } else {
        left = T(StrId::StSortedPre) + DirLabel(state) + T(StrId::StClose);
        right = Tf(StrId::FmtTotal, static_cast<int>(state.view.sorted.size()));
        if (!SegLabel(state).empty()) right += L" · " + SegLabel(state);
    }
    if (leftOverride != nullptr) left = leftOverride;

    StatusBarSetText(w.status, left, SegmentText(state), right);
}

// 结果面板标题行右侧统计（FR-4.8）
void UpdatePanelStat(MainWindow& w) {
    const UiState& state = State();
    std::wstring text = L"—";

    if (state.hasView && !state.valid.empty()) {
        if (state.view.useFill) {
            text = Tf(StrId::FmtPanelFill, static_cast<int>(state.view.fill.total),
                      static_cast<int>(state.view.fill.gaps), DirLabel(state).c_str());
        } else {
            text = Tf(StrId::FmtPanelPlain, static_cast<int>(state.view.sorted.size()),
                      DirLabel(state).c_str());
        }
        if (!SegLabel(state).empty()) text += L" · " + SegLabel(state);
    }
    ResultPanelSetStat(w.result, text);
}

void LayoutAndPlace(MainWindow& w) {
    RECT client;
    GetClientRect(w.hwnd, &client);

    const UiState& state = State();
    const bool hintVisible = !state.valid.empty();
    const int panelWidth = client.right - 2 * kMargin;
    const int faultsWidth = panelWidth - 24;
    const int faultsHeight = state.invalid.empty()
                                ? 0
                                : FaultsMeasure(state.invalid, faultsWidth, w.monoFont);

    w.layout = ComputeLayout(client.right, client.bottom, faultsHeight, hintVisible);
    const LayoutMetrics& m = w.layout;

    PlaceToolbar(w, m);      // FR-12：工具栏按当前语言实测文字宽度布局
    PlacePanelHeads(w, m);  // FR-12：面板标题行同样实测宽度（宽语言不截断）

    Place(w.input.statIn, m.statIn);
    Place(w.input.edit, m.inputEdit);
    Place(w.input.hint, m.inputHint);

    Place(w.keyHint, m.keyHint);

    Place(w.result.statOut, m.statOut);
    Place(w.result.list, m.resultList);

    Place(w.status.bar, m.statusBar);
    StatusBarResize(w.status, client.right);
}

// FR-1.6：输入 → 分析 → 刷新界面
void ApplyInput(MainWindow& w) {
    UiState& state = State();
    state.dedupeChecked = SendMessageW(w.toolbar.chkDedupe, BM_GETCHECK, 0, 0) == BST_CHECKED;
    state.fillChecked = SendMessageW(w.toolbar.chkFill, BM_GETCHECK, 0, 0) == BST_CHECKED;

    const core::Analysis analysis = core::Analyze(InputPanelGetText(w.input), state.dedupeChecked);
    state.valid = analysis.valid;
    state.invalid = analysis.invalid;
    state.diffSeg = analysis.diffSeg;
    state.keySeg = analysis.diffSeg;  // FR-3.3：排序段位即差异段

    state.hasView = false;
    if (state.mode != core::SortDir::None && !state.valid.empty()) {
        state.view = core::BuildView(analysis, state.mode, state.fillChecked);
        state.hasView = true;
    }

    InputPanelUpdateStat(w.input, state.valid.size(), state.invalid.size(), state.dedupeChecked);
    ToolbarSyncSegments(w.toolbar, !state.valid.empty(), state.diffSeg);
    ToolbarSyncSortButtons(w.toolbar);
    ToolbarEnableSortButtons(w.toolbar, !state.valid.empty());
    ResultPanelSetCopyEnabled(w.result, state.hasView);
    MenuBarEnableItems(w.hwnd, !state.valid.empty(), state.hasView, state.fillChecked);

    // FR-3.4 判定说明：有效地址 ≥ 1 条时显示
    const std::wstring hint = state.valid.empty() ? std::wstring() : KeyHintText(state);
    SetWindowTextW(w.keyHint, hint.c_str());
    ShowWindow(w.keyHint, state.valid.empty() ? SW_HIDE : SW_SHOW);

    LayoutAndPlace(w);
    ResultRenderRebuild(w.result, state);
    UpdatePanelStat(w);
    UpdateStatusBar(w);
    InvalidateRect(w.hwnd, nullptr, TRUE);
}

void DoSort(MainWindow& w, core::SortDir dir) {
    State().mode = dir;
    ApplyInput(w);
}

void DoClear(MainWindow& w) {
    w.suppressChange = true;
    InputPanelSetText(w.input, std::wstring());
    w.suppressChange = false;

    State().mode = core::SortDir::None;
    State().copyMsgVisible = false;
    ResultPanelShowCopyMsg(w.result, false);
    KillTimer(w.hwnd, kTimerCopyMsg);

    ApplyInput(w);
    UpdateStatusBar(w, T(StrId::MsgCleared));  // FR-7
    SetFocus(w.input.edit);
}

void DoCopyResult(MainWindow& w) {
    const std::wstring text = ResultRenderCopyText(State());
    if (text.empty()) return;
    if (!util::CopyTextToClipboard(w.hwnd, text)) return;

    ResultPanelShowCopyMsg(w.result, true);
    State().copyMsgVisible = true;
    SetTimer(w.hwnd, kTimerCopyMsg, kCopyMsgMilliseconds, nullptr);
    UpdateStatusBar(w, T(StrId::MsgCopied));
}

void DoCopyGaps(MainWindow& w) {
    UiState& state = State();
    if (!state.fillChecked) {
        UpdateStatusBar(w, T(StrId::MsgNeedFill));  // FR-9
        return;
    }

    size_t count = 0;
    const std::wstring text = ResultRenderGapText(state, &count);
    if (text.empty()) {
        UpdateStatusBar(w, T(StrId::MsgNoGaps));
        return;
    }
    if (!util::CopyTextToClipboard(w.hwnd, text)) return;

    const std::wstring message = Tf(StrId::MsgCopiedGaps, static_cast<int>(count));
    UpdateStatusBar(w, message.c_str());
}

void DoPasteHint(MainWindow& w) {
    SetFocus(w.input.edit);
    UpdateStatusBar(w, T(StrId::MsgPasteFocus));
}

// FR-12：语言切换后刷新所有界面文字（字体 → 菜单重建 → 各控件 → 重算统计与列表）
void ApplyLanguage(MainWindow& w) {
    // 字体可能随语言变化（中/日文字形），先重建并下发到全部界面控件
    const HFONT newFont = CreateUiFont();
    if (newFont != nullptr) {
        ApplyUiFont(w, newFont);
        DeleteObject(w.uiFont);
        w.uiFont = newFont;
    }
    SetWindowTextW(w.hwnd, T(StrId::WindowTitle));
    MenuBarRebuild(w.hwnd, w.instance);
    ToolbarApplyLanguage(w.toolbar);
    InputPanelApplyLanguage(w.input);
    ResultPanelApplyLanguage(w.result);
    ApplyInput(w);  // 重算统计、判定说明、状态栏、表头与列表文字，并施加菜单启用状态
    InvalidateRect(w.hwnd, nullptr, TRUE);
}

// 语言下拉（FR-12）：模态弹出，选中项经 WM_COMMAND 回投
void ShowLangMenu(MainWindow& w) {
    HMENU menu = MenuBarBuildLangMenu(CurrentLangIndex());
    if (menu == nullptr) return;
    RECT rect;
    GetWindowRect(w.toolbar.btnLang, &rect);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, rect.left, rect.bottom, 0,
                   w.hwnd, nullptr);
    MenuBarFreeLangMenu(menu);
}

void DrawPanel(HDC dc, const RECT& panel, int headHeight, MainWindow& w) {
    const ThemeColors& colors = Colors();
    FillRect(dc, &panel, w.panelBrush);

    RECT head = panel;
    head.bottom = head.top + headHeight;
    FillRect(dc, &head, w.headBrush);

    // 标题行下边线
    RECT line = {head.left + 1, head.bottom - 1, head.right - 1, head.bottom};
    FillRect(dc, &line, w.borderBrush);

    // 面板外框
    HPEN pen = CreatePen(PS_SOLID, 1, colors.panelBorder);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, panel.left, panel.top, panel.right, panel.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_CREATE: {
            const CREATESTRUCTW* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
            MainWindow* w = new MainWindow();
            w->instance = create->hInstance;
            w->hwnd = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(w));
            g_window = w;

            w->uiFont = CreateUiFont();
            w->monoFont = CreateMonoFont();
            w->appBrush = CreateSolidBrush(Colors().appBack);
            w->panelBrush = CreateSolidBrush(Colors().panelBack);
            w->headBrush = CreateSolidBrush(Colors().panelHeadBack);
            w->infoBrush = CreateSolidBrush(Colors().infoBack);
            w->borderBrush = CreateSolidBrush(Colors().panelBorder);

            MenuBarAttach(hwnd, w->instance);
            ToolbarCreate(w->toolbar, hwnd, w->instance, w->uiFont);
            InputPanelCreate(w->input, hwnd, w->instance, w->uiFont, w->monoFont);
            w->keyHint = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hwnd,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdKeyHint)),
                                         w->instance, nullptr);
            SendMessageW(w->keyHint, WM_SETFONT, reinterpret_cast<WPARAM>(w->uiFont), TRUE);
            ResultPanelCreate(w->result, hwnd, w->instance, w->uiFont, w->monoFont);
            StatusBarCreate(w->status, hwnd, w->instance, w->uiFont);

            ApplyInput(*w);
            SetFocus(w->input.edit);
            return 0;
        }

        case WM_SIZE:
            if (wparam != SIZE_MINIMIZED) {
                MainWindow* w = FromWindow(hwnd);
                if (w != nullptr) LayoutAndPlace(*w);
            }
            return 0;

        case WM_GETMINMAXINFO: {
            MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lparam);
            info->ptMinTrackSize.x = kWindowWidth;
            info->ptMaxTrackSize.x = kWindowWidth;
            info->ptMinTrackSize.y = kMinWindowHeight;
            return 0;
        }

        case WM_SIZING: {
            // 只允许垂直方向改变尺寸（§5.1）
            RECT* rect = reinterpret_cast<RECT*>(lparam);
            if (wparam == WMSZ_LEFT || wparam == WMSZ_TOPLEFT || wparam == WMSZ_BOTTOMLEFT) {
                rect->left = rect->right - kWindowWidth;
            } else {
                rect->right = rect->left + kWindowWidth;
            }
            if (rect->bottom - rect->top < kMinWindowHeight) {
                rect->bottom = rect->top + kMinWindowHeight;
            }
            return TRUE;
        }

        case WM_NCHITTEST: {
            const LRESULT hit = DefWindowProcW(hwnd, message, wparam, lparam);
            switch (hit) {
                case HTLEFT:
                case HTRIGHT:
                    return HTBORDER;  // 左右边框不可调整，也不显示水平调整光标
                case HTTOPLEFT:
                case HTTOPRIGHT:
                    return HTTOP;
                case HTBOTTOMLEFT:
                case HTBOTTOMRIGHT:
                    return HTBOTTOM;
                default:
                    return hit;
            }
        }

        case WM_PRINTCLIENT: {
            // 带视觉样式的单选 / 复选通过 DrawThemeParentBackground 向父窗口索取背景，
            // 该请求走 WM_PRINTCLIENT；不处理时系统默认填白，工具栏上会出现白色底块（§5.5）。
            MainWindow* w = FromWindow(hwnd);
            if (w == nullptr) break;
            RECT client;
            GetClientRect(hwnd, &client);
            FillRect(reinterpret_cast<HDC>(wparam), &client, w->appBrush);
            return 0;
        }

        case WM_ERASEBKGND: {
            MainWindow* w = FromWindow(hwnd);
            if (w != nullptr) {
                RECT client;
                GetClientRect(hwnd, &client);
                FillRect(reinterpret_cast<HDC>(wparam), &client, w->appBrush);
                return 1;
            }
            break;
        }

        case WM_PAINT: {
            MainWindow* w = FromWindow(hwnd);
            if (w == nullptr) break;

            PAINTSTRUCT paint;
            HDC dc = BeginPaint(hwnd, &paint);
            const ThemeColors& colors = Colors();

            RECT client;
            GetClientRect(hwnd, &client);
            FillRect(dc, &client, w->appBrush);

            const int headHeight = w->layout.controlHeight + kHeadExtra;
            DrawPanel(dc, w->layout.inputPanel, headHeight, *w);
            DrawPanel(dc, w->layout.resultPanel, headHeight, *w);

            // FR-3.4 判定说明条：系统信息栏配色
            if (!State().valid.empty()) {
                RECT hint = w->layout.keyHint;
                FillRect(dc, &hint, w->infoBrush);
                HBRUSH border = CreateSolidBrush(colors.panelBorder);
                FrameRect(dc, &hint, border);
                DeleteObject(border);
            }

            // FR-1.5 无效项标签（自绘三处之一）
            if (!State().invalid.empty()) {
                FaultsPaint(dc, w->layout.faults, State().invalid, w->monoFont);
            }

            EndPaint(hwnd, &paint);
            return 0;
        }

        case WM_CTLCOLORBTN: {
            // 单选 / 复选与工具栏底色一致：不出现系统默认的白色底（§5.5）
            MainWindow* w = FromWindow(hwnd);
            if (w == nullptr) break;
            SetBkColor(reinterpret_cast<HDC>(wparam), Colors().appBack);
            return reinterpret_cast<LRESULT>(w->appBrush);
        }

        case WM_CTLCOLORSTATIC: {
            MainWindow* w = FromWindow(hwnd);
            if (w == nullptr) break;
            const int id = GetDlgCtrlID(reinterpret_cast<HWND>(lparam));
            HDC dc = reinterpret_cast<HDC>(wparam);

            if (id == kIdKeyHint) {
                SetTextColor(dc, Colors().infoText);
                SetBkColor(dc, Colors().infoBack);
                return reinterpret_cast<LRESULT>(w->infoBrush);
            }
            if (id == kIdInputHint) {
                // FR-1.7 粘贴提示：弱化颜色、与输入面板同底
                SetTextColor(dc, Colors().dimText);
                SetBkColor(dc, Colors().panelBack);
                return reinterpret_cast<LRESULT>(w->panelBrush);
            }
            if (id == kIdInputTitle || id == kIdResultTitle || id == kIdStatIn ||
                id == kIdStatOut || id == kIdCopyMsg || id == kIdSegLabel) {
                SetBkColor(dc, Colors().panelHeadBack);
                return reinterpret_cast<LRESULT>(w->headBrush);
            }
            return reinterpret_cast<LRESULT>(w->panelBrush);
        }

        case WM_CTLCOLOREDIT: {
            MainWindow* w = FromWindow(hwnd);
            if (w == nullptr) break;
            SetBkColor(reinterpret_cast<HDC>(wparam), Colors().panelBack);
            return reinterpret_cast<LRESULT>(w->panelBrush);
        }

        case WM_DRAWITEM: {
            MainWindow* w = FromWindow(hwnd);
            const DRAWITEMSTRUCT* item = reinterpret_cast<const DRAWITEMSTRUCT*>(lparam);
            if (w != nullptr && (item->CtlID == static_cast<UINT>(kIdBtnAsc) ||
                                 item->CtlID == static_cast<UINT>(kIdBtnDesc))) {
                ToolbarDrawSortButton(w->toolbar, *item);
                return TRUE;
            }
            break;
        }

        case kMsgApplyLang: {
            // 语言菜单的模态循环结束后再重建，避免在弹出期间换菜单
            MainWindow* w = FromWindow(hwnd);
            if (w != nullptr) ApplyLanguage(*w);
            return 0;
        }

        case WM_NOTIFY: {
            const NMHDR* header = reinterpret_cast<const NMHDR*>(lparam);
            if (header->code == NM_CUSTOMDRAW) {
                MainWindow* w = FromWindow(hwnd);
                return static_cast<LRESULT>(
                    ResultRenderCustomDraw(lparam, w != nullptr ? w->uiFont : nullptr));
            }
            break;
        }

        case WM_TIMER:
            if (wparam == kTimerCopyMsg) {
                KillTimer(hwnd, kTimerCopyMsg);
                MainWindow* w = FromWindow(hwnd);
                if (w != nullptr) ResultPanelShowCopyMsg(w->result, false);
                State().copyMsgVisible = false;
                return 0;
            }
            break;

        case WM_COMMAND: {
            MainWindow* w = FromWindow(hwnd);
            if (w == nullptr) break;

            const int id = LOWORD(wparam);
            const int notify = HIWORD(wparam);
            switch (id) {
                case kIdInputEdit:
                    if (notify == EN_CHANGE && !w->suppressChange) ApplyInput(*w);
                    return 0;
                case kIdChkDedupe:
                case kIdChkFill:
                    if (notify == BN_CLICKED) ApplyInput(*w);
                    return 0;
                case kIdBtnAsc:
                case kCmdAsc:
                    DoSort(*w, core::SortDir::Asc);
                    return 0;
                case kIdBtnDesc:
                case kCmdDesc:
                    DoSort(*w, core::SortDir::Desc);
                    return 0;
                case kIdBtnClear:
                case kCmdClear:
                    DoClear(*w);
                    return 0;
                case kIdBtnCopy:
                case kCmdCopyResult:
                    if (State().hasView) DoCopyResult(*w);
                    return 0;
                case kCmdCopyGaps:
                    DoCopyGaps(*w);
                    return 0;
                case kIdBtnUpdate:
                    util::OpenUrl(APP_RELEASE_URL);
                    return 0;
                case kIdBtnLang:
                    if (notify == BN_CLICKED) ShowLangMenu(*w);
                    return 0;
                default:
                    break;
            }
            // 语言菜单项（FR-12）：命令 ID 连续编号，与 Languages() 下标对应
            if (id >= kCmdLangFirst &&
                id < kCmdLangFirst + static_cast<int>(Languages().size())) {
                if (id - kCmdLangFirst != CurrentLangIndex()) {
                    SetLangByIndex(id - kCmdLangFirst);
                    PostMessageW(hwnd, kMsgApplyLang, 0, 0);
                }
                return 0;
            }
            switch (id) {
                case kCmdAbout:
                    ShowAboutDialog(hwnd, w->instance);
                    return 0;
                case kCmdPasteHint:
                    DoPasteHint(*w);
                    return 0;
                case kCmdExit:
                    SendMessageW(hwnd, WM_CLOSE, 0, 0);
                    return 0;
                default:
                    break;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY: {
            MainWindow* w = FromWindow(hwnd);
            if (w != nullptr) {
                DeleteObject(w->uiFont);
                DeleteObject(w->monoFont);
                DeleteObject(w->appBrush);
                DeleteObject(w->panelBrush);
                DeleteObject(w->headBrush);
                DeleteObject(w->infoBrush);
                DeleteObject(w->borderBrush);
            }
            PostQuitMessage(0);
            return 0;
        }

        case WM_NCDESTROY: {
            MainWindow* w = FromWindow(hwnd);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            g_window = nullptr;
            delete w;
            return 0;
        }

        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

}  // namespace

const wchar_t* MainWindowClassName() {
    return APP_WINDOW_CLASS;
}

bool CreateMainWindow(HINSTANCE instance, int showCommand) {
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
    // 类背景画刷：带视觉样式的单选 / 复选会通过 WM_PRINTCLIENT 向父窗口索取背景，
    // 没有类画刷时默认填白，会在工具栏上留下白色底块（§5.5）。
    // 实际绘制仍由 WM_ERASEBKGND / WM_PAINT 负责，这里只是给系统一个正确颜色的画刷。
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_BTNFACE + 1));
    windowClass.lpszClassName = MainWindowClassName();
    if (RegisterClassExW(&windowClass) == 0) return false;

    // §5.1：窗口宽固定 780 px（含边框）；不提供最大化
    const DWORD style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX;
    const DWORD exStyle = 0;

    RECT probe = {0, 0, kWindowWidth, DefaultClientHeight()};
    AdjustWindowRectEx(&probe, style, TRUE, exStyle);
    const int windowWidth = kWindowWidth;
    const int windowHeight = (probe.bottom - probe.top);
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    const int x = (screenWidth - windowWidth) / 2;
    const int y = (screenHeight - windowHeight) / 2;

    HWND hwnd = CreateWindowExW(exStyle, windowClass.lpszClassName, T(StrId::WindowTitle),
                                style | WS_CLIPCHILDREN, x, y, windowWidth, windowHeight, nullptr,
                                nullptr, instance, nullptr);
    if (hwnd == nullptr) return false;

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);
    return true;
}

int RunMessageLoop() {
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (g_window != nullptr) {
            // 输入框获得焦点时不翻译加速键，保证 Ctrl+C / Ctrl+V 为编辑框原生行为
            const bool editFocused = GetFocus() == g_window->input.edit;
            if (!editFocused && MenuBarTranslate(g_window->hwnd, &message)) continue;
            // 标准控件之间的 Tab 遍历
            if (IsDialogMessageW(g_window->hwnd, &message)) continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

}  // namespace ui
