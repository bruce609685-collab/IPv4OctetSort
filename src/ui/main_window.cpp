#include "ui/main_window.h"

#include <commctrl.h>

#include <string>

#include "app/app_info.h"
#include "core/pipeline.h"
#include "core/segment.h"
#include "ui/about_dialog.h"
#include "ui/control_ids.h"
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

constexpr wchar_t kWindowTitle[] = APP_NAME_EN L" — " APP_NAME_CN;

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

std::wstring DirLabel(const UiState& state) {
    return state.mode == core::SortDir::Desc ? L"降序" : L"升序";
}

std::wstring SegLabel(const UiState& state) {
    if (state.keySeg == core::kNoSegment) return std::wstring();
    return L"按 " + std::wstring(core::kSegmentNames[state.keySeg]) + L" 段";
}

std::wstring SegmentText(const UiState& state) {
    if (state.valid.empty()) return L"段位：—";
    if (state.diffSeg == core::kNoSegment) return L"段位：无差异";
    return L"段位：" + std::wstring(core::kSegmentNames[state.diffSeg]) + L" 段";
}

// FR-3.4 判定说明文案
std::wstring KeyHintText(const UiState& state) {
    if (state.diffSeg == core::kNoSegment) {
        return L"四段取值一致，没有差异段；缺位填充按 D 段铺槽位";
    }
    std::wstring head;
    for (int i = 0; i < state.diffSeg; ++i) {
        if (i) head += L"、";
        head += core::kSegmentNames[i];
    }
    if (!head.empty()) head += L" 段取值一致，";

    const std::wstring seg = core::kSegmentNames[state.diffSeg];
    return head + seg + L" 段有多个取值（" + core::DiffValuePreview(state.valid, state.diffSeg) +
           L"），仅可按 " + seg + L" 段排序";
}

void UpdateStatusBar(MainWindow& w, const wchar_t* leftOverride = nullptr) {
    const UiState& state = State();
    std::wstring left;
    std::wstring right;

    if (!state.hasView || state.valid.empty()) {
        left = state.valid.empty() ? L"就绪" : L"就绪 · 待排序";
        right = L"—";
    } else if (state.view.useFill) {
        left = L"已补位（" + DirLabel(state) + L"）";
        right = L"槽位 " + std::to_wstring(state.view.fill.total) + L" · 空缺 " +
                std::to_wstring(state.view.fill.gaps);
        if (!SegLabel(state).empty()) right += L" · " + SegLabel(state);
    } else {
        left = L"已排序（" + DirLabel(state) + L"）";
        right = L"共 " + std::to_wstring(state.view.sorted.size()) + L" 条";
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
            text = std::to_wstring(state.view.fill.total) + L" 行 · 空缺 " +
                   std::to_wstring(state.view.fill.gaps) + L" 个 · " + DirLabel(state);
        } else {
            text = std::to_wstring(state.view.sorted.size()) + L" 条 · " + DirLabel(state);
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

    Place(w.toolbar.btnAsc, m.btnAsc);
    Place(w.toolbar.btnDesc, m.btnDesc);
    Place(w.toolbar.segLabel, m.segLabel);
    for (int i = 0; i < 4; ++i) Place(w.toolbar.radio[i], m.radio[i]);
    Place(w.toolbar.chkDedupe, m.chkDedupe);
    Place(w.toolbar.chkFill, m.chkFill);
    Place(w.toolbar.btnUpdate, m.btnUpdate);

    Place(w.input.title, m.inputTitle);
    Place(w.input.btnClear, m.btnClear);
    Place(w.input.statIn, m.statIn);
    Place(w.input.edit, m.inputEdit);
    Place(w.input.hint, m.inputHint);

    Place(w.keyHint, m.keyHint);

    Place(w.result.title, m.resultTitle);
    Place(w.result.btnCopy, m.btnCopy);
    Place(w.result.copyMsg, m.copyMsg);
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
    UpdateStatusBar(w, L"已清空输入");  // FR-7
    SetFocus(w.input.edit);
}

void DoCopyResult(MainWindow& w) {
    const std::wstring text = ResultRenderCopyText(State());
    if (text.empty()) return;
    if (!util::CopyTextToClipboard(w.hwnd, text)) return;

    ResultPanelShowCopyMsg(w.result, true);
    State().copyMsgVisible = true;
    SetTimer(w.hwnd, kTimerCopyMsg, kCopyMsgMilliseconds, nullptr);
    UpdateStatusBar(w, L"结果已复制到剪贴板");
}

void DoCopyGaps(MainWindow& w) {
    UiState& state = State();
    if (!state.fillChecked) {
        UpdateStatusBar(w, L"请先勾选「缺位填充」再复制空缺地址");  // FR-9
        return;
    }

    size_t count = 0;
    const std::wstring text = ResultRenderGapText(state, &count);
    if (text.empty()) {
        UpdateStatusBar(w, L"没有空缺地址");
        return;
    }
    if (!util::CopyTextToClipboard(w.hwnd, text)) return;

    const std::wstring message = L"已复制 " + std::to_wstring(count) + L" 个空缺地址";
    UpdateStatusBar(w, message.c_str());
}

void DoPasteHint(MainWindow& w) {
    SetFocus(w.input.edit);
    UpdateStatusBar(w, L"已聚焦输入框，按 Ctrl+V 粘贴");
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

    HWND hwnd = CreateWindowExW(exStyle, windowClass.lpszClassName, kWindowTitle,
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
