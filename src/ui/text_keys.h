#pragma once

// 界面文案的键清单（FR-12）。本文件只定义键，不定义任何语言文字——
// 全部文字在 languages/*.lng 外置文件里，由 ui/i18n 按当前语言查表。
// X 宏同时生成 StrId 枚举与键名表，二者条目数与顺序天生一致；
// tests/test_core.cpp 直接用 kStrKeys 校验语言包键集合。
//
// 带 %d / %s 的键是格式串，用 ui/i18n 的 Tf() 系列函数按顺序填充：
// 值首尾空白会被语言文件解析器去除，段间的分隔空格/顿号等写在代码里。
namespace ui {

// X(枚举名)
#define UI_TEXT_LIST(X)                \
    /* 主窗口 */                        \
    X(WindowTitle)                     \
    X(StartupError)                    \
    /* 菜单栏 */                        \
    X(MenuFile)                        \
    X(MenuClear)                       \
    X(MenuPaste)                       \
    X(MenuExit)                        \
    X(MenuEdit)                        \
    X(MenuCopy)                        \
    X(MenuCopyGaps)                    \
    X(MenuView)                        \
    X(MenuAsc)                         \
    X(MenuDesc)                        \
    X(MenuAbout)                       \
    /* 工具栏 */                        \
    X(BtnAsc)                          \
    X(BtnDesc)                         \
    X(SegLabel)                        \
    X(ChkDedupe)                       \
    X(ChkFill)                         \
    X(BtnUpdate)                       \
    X(LangArrow)                       \
    /* 输入面板 */                      \
    X(InputTitle)                      \
    X(BtnClear)                        \
    X(StatEntries)                     \
    X(StatUnrecognized)                \
    X(StatMerged)                      \
    X(InputHint)                       \
    X(FaultFmt)                        \
    X(FaultMore)                       \
    /* 无效项原因（分类见 core::ReasonKind；%d 为段序号） */ \
    X(ReasonFormat)                    \
    X(ReasonOctetFmt)                  \
    X(ReasonPrefix)                    \
    /* 段位判定说明（FR-3.4，句式见 i18n.cpp 的 KeyHintSentence） */ \
    X(HintNoDiff)                      \
    X(HintIdentOne)                    \
    X(HintIdentMany)                   \
    X(HintJoinSep)                     \
    X(HintMulti)                       \
    X(DiffMore)                        \
    /* 状态栏与操作提示 */              \
    X(StReady)                         \
    X(StReadyPending)                  \
    X(StFilledPre)                     \
    X(StSortedPre)                     \
    X(StClose)                         \
    X(FmtSlots)                        \
    X(FmtTotal)                        \
    X(BySegFmt)                        \
    X(SegDash)                         \
    X(SegNone)                         \
    X(SegFmt)                          \
    X(MsgCleared)                      \
    X(MsgCopied)                       \
    X(MsgNeedFill)                     \
    X(MsgNoGaps)                       \
    X(MsgCopiedGaps)                   \
    X(MsgPasteFocus)                   \
    X(DirAsc)                          \
    X(DirDesc)                         \
    /* 结果面板 */                      \
    X(ResultTitle)                     \
    X(BtnCopy)                         \
    X(CopyMsg)                         \
    X(ColAddress)                      \
    X(ColSequence)                     \
    X(ColYourList)                     \
    X(FmtPanelFill)                    \
    X(FmtPanelPlain)                   \
    X(EmptyHintA)                      \
    X(EmptyHintB)                      \
    X(EmptyHintC)                      \
    X(GapText)                         \
    X(NoticeFmt)                       \
    /* 关于对话框 */                    \
    X(AboutCaption)                    \
    X(AboutNameCn)                     \
    X(AboutNameEn)                     \
    X(AboutVersion)                    \
    X(AboutDate)                       \
    X(AboutOk)

enum class StrId {
#define X(id) id,
    UI_TEXT_LIST(X)
#undef X
    Count
};

// 键名表（宽字符串，与 StrId 一一对应）；语言文件左侧的键必须与此一致。
// 宽字符串化需要二级宏：L#id 不会自动合并，先做 #id 再 L## 拼接。
#define UI_WIDE2(s) L##s
#define UI_WIDE(s) UI_WIDE2(s)
inline const wchar_t* const kStrKeys[] = {
#define X(id) UI_WIDE(#id),
    UI_TEXT_LIST(X)
#undef X
#undef UI_WIDE
#undef UI_WIDE2
};

}  // namespace ui
