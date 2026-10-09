#pragma once

#include <string>
#include <vector>

#include "core/types.h"
#include "ui/text_keys.h"

// 界面多语言（FR-12）。全部文字在 languages/*.lng 外置文件中，
// 构建时由 tools/lng2cpp.cpp 打进 exe（见 ui/embedded_languages.h）；
// 新增语言只需把一个 .lng 文件放进 languages/ 重新编译。
//
// 查找顺序：当前语言 → 英语（en-US，默认兼回退）→ 键名（便于发现漏译）。
// 语言选择写入 HKCU 注册表（存 BCP-47 标签，如 zh-CN），下次启动沿用；
// 无记录时跟随系统界面语言；都不匹配则英语。
// 键清单与 StrId 见 ui/text_keys.h。
namespace ui {

// 一个已加载的语言包
struct LangDef {
    std::wstring locale;      // BCP-47 标签（en-US、zh-CN、zh-TW…）
    std::wstring nativeName;  // 该语言自身书写的名称（下拉菜单按此显示）
    bool loaded = false;      // 文件解析成功；坏文件不阻断启动，缺失键回退英语
    std::vector<std::pair<std::wstring, std::wstring> > values;  // 键值对（含 locale/name）
};

// 解析全部内嵌语言包并选定初始语言；须在创建任何窗口之前调用（main.cpp）。
void InitLang();

// 全部语言（下标 0 固定为 en-US，即默认与回退语言）
const std::vector<LangDef>& Languages();
int CurrentLangIndex();

// 切换语言并写入注册表；index 越界时不动。返回是否切换成功。
bool SetLangByIndex(int index);

// 按 BCP-47 标签查找（先全标签、后主标签，不区分大小写）；找不到返回 false。
// InitLang 用它匹配注册表记忆与系统界面语言。
bool SetLangByLocale(const std::wstring& locale);

// 取当前语言的字符串；格式串请用 Tf() 系列填充
const wchar_t* T(StrId id);

// Tf：按顺序填充 %d / %s（%s 对应宽字符串）；%% 输出一个 %
std::wstring Tf(StrId id, int a);
std::wstring Tf(StrId id, int a, int b);
std::wstring Tf(StrId id, int a, const wchar_t* s);
std::wstring Tf(StrId id, int a, int b, const wchar_t* s);
std::wstring Tf(StrId id, int a, const wchar_t* s1, const wchar_t* s2);
std::wstring Tf(StrId id, const wchar_t* s);
std::wstring Tf(StrId id, const wchar_t* s1, const wchar_t* s2, const wchar_t* s3,
                const wchar_t* s4);

// 语言显示名与按钮文字（当前语言名 + 箭头）
const wchar_t* LangName(int index);
std::wstring LangButtonText();

// 排序方向词（ascending/descending，用于状态与统计）
std::wstring DirWord(core::SortDir dir);

// FR-3.4 判定说明整句（各语言句式不同，由键 HintIdentOne/HintIdentMany/
// HintJoinSep/HintMulti 组句；preview 已含取值与 DiffMore 截断后缀）
std::wstring KeyHintSentence(int diffSeg, const std::wstring& preview);

// 无效项原因（按当前语言渲染；kind/arg 来自 core 的解析结果）
std::wstring ReasonText(core::ReasonKind kind, int arg);

// 按当前语言的界面字体字形（简体/繁体 → 微软雅黑，日语 → Meiryo，
// 其余 → Segoe UI；缺失字形由系统字体链接自动回退）
const wchar_t* UiFontFace();

}  // namespace ui
