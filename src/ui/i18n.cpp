#include "ui/i18n.h"

#include <windows.h>

#include <cwchar>
#include <string>

// 语言状态、字符串表与组句函数。表由 i18n.h 的 UI_TEXT_LIST 经 X 宏生成，
// 两张表条目数由 static_assert 保证与枚举一致。
namespace ui {
namespace {

Lang g_lang = Lang::ZhCn;

const wchar_t* const kZh[] = {
#define X(id, zh, en) zh,
    UI_TEXT_LIST(X)
#undef X
};

const wchar_t* const kEn[] = {
#define X(id, zh, en) en,
    UI_TEXT_LIST(X)
#undef X
};

static_assert(sizeof(kZh) / sizeof(kZh[0]) == static_cast<size_t>(StrId::Count),
              "中文表条目数与 StrId::Count 不一致");
static_assert(sizeof(kEn) / sizeof(kEn[0]) == static_cast<size_t>(StrId::Count),
              "英文表条目数与 StrId::Count 不一致");

constexpr const wchar_t* kRegKey = L"Software\\IPv4OctetSort";
constexpr const wchar_t* kRegValue = L"Language";

}  // namespace

Lang CurrentLang() {
    return g_lang;
}

void InitLang() {
    wchar_t value[16] = {0};
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegKey, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
        DWORD bytes = sizeof(value) - sizeof(wchar_t);
        DWORD type = 0;
        const LONG result = RegQueryValueExW(key, kRegValue, nullptr, &type,
                                             reinterpret_cast<LPBYTE>(value), &bytes);
        RegCloseKey(key);
        if (result == ERROR_SUCCESS && type == REG_SZ) {
            if (wcscmp(value, L"en-US") == 0) {
                g_lang = Lang::En;
                return;
            }
            if (wcscmp(value, L"zh-CN") == 0) {
                g_lang = Lang::ZhCn;
                return;
            }
        }
    }
    // 无记录：跟随系统界面语言（英文族 → English，其余 → 简体中文）
    g_lang = (PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_ENGLISH) ? Lang::En : Lang::ZhCn;
}

void SetLang(Lang lang) {
    g_lang = lang;
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key,
                        nullptr) == ERROR_SUCCESS) {
        const wchar_t* value = lang == Lang::En ? L"en-US" : L"zh-CN";
        RegSetValueExW(key, kRegValue, 0, REG_SZ, reinterpret_cast<const BYTE*>(value),
                       static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
    }
}

const wchar_t* T(StrId id) {
    const size_t index = static_cast<size_t>(id);
    const bool english = (g_lang == Lang::En);
    if (index >= static_cast<size_t>(StrId::Count)) return L"";
    return english ? kEn[index] : kZh[index];
}

// Tf 展开：不走 printf 族——MinGW 下 swprintf 对 %s 的宽/窄解释因运行时而异，
// 会把宽参数的 UTF-16 字节当窄字符串解码（中文参数直接损坏）。格式串只用到
// %d / %s / %%，这里按顺序取参手工替换，行为跨平台确定。
namespace {

struct Arg {
    bool isInt = false;
    int num = 0;
    const wchar_t* str = nullptr;
};

std::wstring Expand(StrId id, const Arg* args, size_t count) {
    const wchar_t* fmt = T(id);
    std::wstring out;
    out.reserve(wcslen(fmt) + 32);
    size_t next = 0;
    for (const wchar_t* p = fmt; *p != L'\0'; ++p) {
        if (p[0] == L'%' && p[1] == L'%') {
            out += L'%';
            ++p;
            continue;
        }
        if (p[0] == L'%' && (p[1] == L'd' || p[1] == L's') && next < count) {
            if (p[1] == L'd') {
                if (args[next].isInt) out += std::to_wstring(args[next].num);
            } else {
                if (!args[next].isInt && args[next].str != nullptr) out += args[next].str;
            }
            ++next;
            ++p;
            continue;
        }
        out += *p;
    }
    return out;
}

}  // namespace

std::wstring Tf(StrId id, int a) {
    const Arg args[] = {{true, a, nullptr}};
    return Expand(id, args, 1);
}

std::wstring Tf(StrId id, int a, int b) {
    const Arg args[] = {{true, a, nullptr}, {true, b, nullptr}};
    return Expand(id, args, 2);
}

std::wstring Tf(StrId id, int a, const wchar_t* s) {
    const Arg args[] = {{true, a, nullptr}, {false, 0, s}};
    return Expand(id, args, 2);
}

std::wstring Tf(StrId id, int a, int b, const wchar_t* s) {
    const Arg args[] = {{true, a, nullptr}, {true, b, nullptr}, {false, 0, s}};
    return Expand(id, args, 3);
}

std::wstring Tf(StrId id, int a, const wchar_t* s1, const wchar_t* s2) {
    const Arg args[] = {{true, a, nullptr}, {false, 0, s1}, {false, 0, s2}};
    return Expand(id, args, 3);
}

std::wstring Tf(StrId id, const wchar_t* s) {
    const Arg args[] = {{false, 0, s}};
    return Expand(id, args, 1);
}

const wchar_t* LangName(Lang lang) {
    return lang == Lang::En ? T(StrId::LangNameEn) : T(StrId::LangNameZh);
}

std::wstring LangButtonText() {
    return std::wstring(LangName(g_lang)) + T(StrId::LangArrow);
}

std::wstring DirWord(core::SortDir dir) {
    if (dir == core::SortDir::None) return std::wstring();
    return dir == core::SortDir::Desc ? T(StrId::DirDesc) : T(StrId::DirAsc);
}

std::wstring ReasonText(core::ReasonKind kind, int arg) {
    switch (kind) {
        case core::ReasonKind::OctetRange:
            return Tf(StrId::ReasonOctetFmt, arg);
        case core::ReasonKind::PrefixRange:
            return T(StrId::ReasonPrefix);
        case core::ReasonKind::Format:
        default:
            return T(StrId::ReasonFormat);
    }
}

std::wstring KeyHintSentence(int diffSeg, const std::wstring& preview) {
    const bool english = (g_lang == Lang::En);
    if (diffSeg == core::kNoSegment) return T(StrId::HintNoDiff);

    // 差异段之前的段取值一致：中文「A、B 段取值一致，」；英文「Octets A, B identical; 」
    std::wstring head;
    for (int i = 0; i < diffSeg; ++i) {
        if (i) head += english ? L", " : L"、";
        head += core::kSegmentNames[i];
    }
    if (!head.empty()) {
        if (english) {
            head = (diffSeg > 1 ? T(StrId::HintOctetN) : T(StrId::HintOctet1)) + head +
                   T(StrId::HintIdentEn);
        } else {
            head += T(StrId::HintIdentCn);
        }
    }

    const std::wstring seg = core::kSegmentNames[diffSeg];
    if (english) {
        return head + seg + T(StrId::HintMultiEn1) + preview + T(StrId::HintMultiEn2) + seg;
    }
    return head + seg + T(StrId::HintMultiCn1) + preview + T(StrId::HintMultiCn2) + seg +
           T(StrId::HintMultiCn3);
}

}  // namespace ui
