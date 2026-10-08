#pragma once

#include <string>

#include "core/types.h"

// 界面多语言（FR-11）：全部界面文字集中在 UI_TEXT_LIST 这一份清单里，
// X 宏同时生成枚举与中、英两张表——条目数与顺序天生一致，增删文字只改一处。
// 语言选择写入 HKCU 注册表，下次启动沿用；无记录时跟随系统界面语言。
namespace ui {

enum class Lang { ZhCn, En };

// X(枚举名, 中文, English)。带 %d / %s 的条目是格式串，用 Tf() 系列函数填充。
#define UI_TEXT_LIST(X)                                                            \
    /* 主窗口 */                                                                    \
    X(WindowTitle, L"IPv4OctetSort — IPv4网络地址排序器",                            \
      L"IPv4OctetSort — IPv4 Address Sorter")                                       \
    X(StartupError, L"程序启动失败：主窗口创建失败。",                                \
      L"Startup failed: could not create the main window.")                         \
    /* 菜单栏 */                                                                    \
    X(MenuFile, L"文件(&F)", L"&File")                                              \
    X(MenuClear, L"清空输入(&C)\tCtrl+D", L"&Clear Input\tCtrl+D")                  \
    X(MenuPaste, L"粘贴到输入(&P)\tCtrl+V", L"&Paste to Input\tCtrl+V")            \
    X(MenuExit, L"退出(&X)\tAlt+F4", L"E&xit\tAlt+F4")                              \
    X(MenuEdit, L"编辑(&E)", L"&Edit")                                              \
    X(MenuCopy, L"复制结果(&C)\tCtrl+C", L"&Copy Result\tCtrl+C")                   \
    X(MenuCopyGaps, L"仅复制空缺地址(&G)", L"Copy &Gaps Only")                      \
    X(MenuView, L"视图(&V)", L"&View")                                              \
    X(MenuAsc, L"升序排列(&A)", L"Sort &Ascending")                                  \
    X(MenuDesc, L"降序排列(&D)", L"Sort &Descending")                                \
    X(MenuAbout, L"关于(&A)", L"&About")                                            \
    /* 工具栏 */                                                                    \
    X(BtnAsc, L"升序排列", L"Sort Asc")                                             \
    X(BtnDesc, L"降序排列", L"Sort Desc")                                           \
    X(SegLabel, L"排序段位", L"Sort by")                                            \
    X(ChkDedupe, L"合并重复", L"Merge dupes")                                       \
    X(ChkFill, L"缺位填充", L"Fill gaps")                                           \
    X(BtnUpdate, L"检查更新", L"Check updates")                                     \
    /* 语言名称按其自身语言显示，不随界面切换；▼ 为下拉指示 */                        \
    X(LangNameZh, L"简体中文", L"简体中文")                                          \
    X(LangNameEn, L"English", L"English")                                           \
    X(LangArrow, L" ▼", L" ▼")                                                     \
    /* 输入面板 */                                                                  \
    X(InputTitle, L"输入", L"Input")                                                \
    X(BtnClear, L"清空", L"Clear")                                                  \
    X(StatEntries, L"%d 条", L"%d entries")                                         \
    X(StatUnrecognized, L" · %d 条未识别", L" · %d unrecognized")                   \
    X(StatMerged, L" · 已合并重复", L" · merged duplicates")                        \
    X(InputHint, L"粘贴地址，一行一个，逗号或空格分隔也可以",                          \
      L"Paste addresses, one per line; commas or spaces also work")                 \
    X(FaultFmt, L"第 %d 行 · %s — %s", L"Line %d · %s — %s")                        \
    X(FaultMore, L"另有 %d 条未显示", L"%d more not shown")                         \
    /* 无效项原因（分类见 core::ReasonKind；%d 为段序号） */                          \
    X(ReasonFormat, L"格式不是 IPv4 点分十进制", L"not a dotted-decimal IPv4")       \
    X(ReasonOctetFmt, L"第 %d 段超出 0-255", L"octet %d out of range 0-255")         \
    X(ReasonPrefix, L"掩码长度超出 0-32", L"prefix length out of range 0-32")        \
    /* 段位判定说明（FR-3.4，句式两种语言不同，组句见 KeyHintSentence） */            \
    X(HintNoDiff, L"四段取值一致，没有差异段；缺位填充按 D 段铺槽位",                  \
      L"All four octets are identical - no differing octet; gaps fill along octet D") \
    X(HintIdentCn, L" 段取值一致，", L"")                                           \
    X(HintIdentEn, L"", L" identical; ")                                            \
    X(HintMultiCn1, L" 段有多个取值（", L"")                                        \
    X(HintMultiCn2, L"），仅可按 ", L"")                                            \
    X(HintMultiCn3, L" 段排序", L"")                                                \
    X(HintMultiEn1, L"", L" has multiple values (")                                 \
    X(HintMultiEn2, L"", L"), sort only by octet ")                                 \
    X(HintOctet1, L"", L"Octet ")                                                   \
    X(HintOctetN, L"", L"Octets ")                                                  \
    X(DiffMore, L" 等 %d 个值", L" of %d values")                                   \
    /* 状态栏与操作提示 */                                                          \
    X(StReady, L"就绪", L"Ready")                                                  \
    X(StReadyPending, L"就绪 · 待排序", L"Ready · awaiting sort")                   \
    X(StFilledPre, L"已补位（", L"Filled (")                                       \
    X(StSortedPre, L"已排序（", L"Sorted (")                                       \
    X(StClose, L"）", L")")                                                         \
    X(FmtSlots, L"槽位 %d · 空缺 %d", L"%d slots · %d gaps")                        \
    X(FmtTotal, L"共 %d 条", L"%d total")                                          \
    X(BySegFmt, L"按 %s 段", L"by octet %s")                                        \
    X(SegDash, L"段位：—", L"Octet: -")                                             \
    X(SegNone, L"段位：无差异", L"Octet: none")                                     \
    X(SegFmt, L"段位：%s 段", L"Octet: %s")                                         \
    X(MsgCleared, L"已清空输入", L"Input cleared")                                  \
    X(MsgCopied, L"结果已复制到剪贴板", L"Result copied to clipboard")               \
    X(MsgNeedFill, L"请先勾选「缺位填充」再复制空缺地址",                              \
      L"Tick \"Fill gaps\" before copying gap addresses")                           \
    X(MsgNoGaps, L"没有空缺地址", L"No gap addresses")                              \
    X(MsgCopiedGaps, L"已复制 %d 个空缺地址", L"Copied %d gap addresses")            \
    X(MsgPasteFocus, L"已聚焦输入框，按 Ctrl+V 粘贴",                                \
      L"Input focused - press Ctrl+V to paste")                                     \
    X(DirAsc, L"升序", L"ascending")                                                \
    X(DirDesc, L"降序", L"descending")                                              \
    /* 结果面板 */                                                                  \
    X(ResultTitle, L"结果", L"Result")                                              \
    X(BtnCopy, L"复制结果", L"Copy Result")                                         \
    X(CopyMsg, L"已复制", L"Copied")                                                \
    X(ColAddress, L"地址", L"Address")                                              \
    X(ColSequence, L"完整序列", L"Full sequence")                                   \
    X(ColYourList, L"你的清单", L"Your list")                                       \
    X(FmtPanelFill, L"%d 行 · 空缺 %d 个 · %s", L"%d rows · %d gaps · %s")          \
    X(FmtPanelPlain, L"%d 条 · %s", L"%d entries · %s")                             \
    X(EmptyHintA, L"点「", L"Click \"")                                             \
    X(EmptyHintB, L"」或「", L"\" or \"")                                           \
    X(EmptyHintC, L"」开始。", L"\" to start.")                                     \
    X(GapText, L"IP地址空缺", L"IP address gap")                                    \
    X(NoticeFmt, L"按当前段位要铺 %d 个槽位，超过上限 8,192 个，本次未补位，已按普通列表显示。把输入收窄到更少的网段后即可补位。", \
      L"Filling needs %d slots, above the 8,192 limit - not filled; showing a plain list. Narrow the input to fewer subnets to enable filling.") \
    /* 关于对话框 */                                                                \
    X(AboutCaption, L"关于 IPv4OctetSort", L"About IPv4OctetSort")                  \
    X(AboutNameCn, L"程序名称", L"Chinese name")                                    \
    X(AboutNameEn, L"英文名称", L"English name")                                    \
    X(AboutVersion, L"版本", L"Version")                                            \
    X(AboutDate, L"构建日期", L"Build date")                                        \
    X(AboutOk, L"确定", L"OK")

enum class StrId {
#define X(id, zh, en) id,
    UI_TEXT_LIST(X)
#undef X
    Count
};

// 当前语言（InitLang 之前为简体中文）
Lang CurrentLang();
void SetLang(Lang lang);

// 启动时读取注册表；无记录时取系统界面语言（英文系统 → English）
void InitLang();

// 取当前语言的字符串；格式串请用 Tf() 系列填充
const wchar_t* T(StrId id);

// Tf：格式化 %d / %s 条目（%s 对应宽字符串）
std::wstring Tf(StrId id, int a);
std::wstring Tf(StrId id, int a, int b);
std::wstring Tf(StrId id, int a, const wchar_t* s);
std::wstring Tf(StrId id, int a, int b, const wchar_t* s);
std::wstring Tf(StrId id, int a, const wchar_t* s1, const wchar_t* s2);
std::wstring Tf(StrId id, const wchar_t* s);

// 语言显示名与按钮文字（当前语言名 + ▼）
const wchar_t* LangName(Lang lang);
std::wstring LangButtonText();

// 排序方向词（升序/降序 → ascending/descending，用于状态与统计）
std::wstring DirWord(core::SortDir dir);

// FR-3.4 判定说明整句（两种语言句式不同，单独组句）
std::wstring KeyHintSentence(int diffSeg, const std::wstring& preview);

// 无效项原因（按当前语言渲染；kind/arg 来自 core 的解析结果）
std::wstring ReasonText(core::ReasonKind kind, int arg);

}  // namespace ui
