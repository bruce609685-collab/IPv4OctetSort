#include "ui/i18n.h"

#include <windows.h>

#include <cwchar>

#include "core/lng_file.h"
#include "ui/embedded_languages.h"

// 语言状态与外置语言包加载（FR-12）。包数据由构建期生成（languages.cpp），
// 这里只做解析、回退查找与注册表记忆。
namespace ui {
namespace {

constexpr const wchar_t* kRegKey = L"Software\\IPv4OctetSort";
constexpr const wchar_t* kRegValue = L"Language";

std::vector<LangDef> g_languages;
int g_current = 0;  // 下标 0 固定为 en-US（默认与回退）

const std::wstring* FindValue(const LangDef& lang, const wchar_t* key) {
    for (size_t i = 0; i < lang.values.size(); ++i) {
        if (lang.values[i].first == key) return &lang.values[i].second;
    }
    return nullptr;
}

std::wstring Lower(std::wstring text) {
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] >= L'A' && text[i] <= L'Z') text[i] = text[i] - L'A' + L'a';
    }
    return text;
}

// BCP-47 主标签（- 之前部分），小写
std::wstring PrimarySubtag(const std::wstring& locale) {
    const size_t dash = locale.find(L'-');
    return Lower(dash == std::wstring::npos ? locale : locale.substr(0, dash));
}

int FindByLocale(const std::wstring& locale) {
    if (locale.empty()) return -1;
    const std::wstring want = Lower(locale);
    for (size_t i = 0; i < g_languages.size(); ++i) {
        if (Lower(g_languages[i].locale) == want) return static_cast<int>(i);
    }
    const std::wstring wantPrimary = PrimarySubtag(locale);
    for (size_t i = 0; i < g_languages.size(); ++i) {
        if (PrimarySubtag(g_languages[i].locale) == wantPrimary) return static_cast<int>(i);
    }
    return -1;
}

// 解析全部内嵌语言包；坏文件保留占位（loaded=false），键回退英语
void LoadLanguages() {
    if (!g_languages.empty()) return;

    for (unsigned int n = 0; n < kEmbeddedLanguageCount; ++n) {
        const EmbeddedLanguage& blob = kEmbeddedLanguages[n];
        const std::string bytes(reinterpret_cast<const char*>(blob.data), blob.size);
        const core::LngFile parsed = core::ParseLng(bytes);

        LangDef lang;
        lang.loaded = parsed.ok;
        if (parsed.ok) {
            for (size_t i = 0; i < parsed.entries.size(); ++i) {
                lang.values.push_back(std::make_pair(parsed.entries[i].key, parsed.entries[i].value));
            }
            const std::wstring* locale = FindValue(lang, L"locale");
            const std::wstring* name = FindValue(lang, L"name");
            lang.locale = locale != nullptr ? *locale : std::wstring();
            lang.nativeName = name != nullptr ? *name : lang.locale;
        } else {
            // 解析失败：以文件名作为 locale，名称照显，键全部回退英语
            std::wstring stem;
            for (const char* p = blob.fileStem; *p != '\0'; ++p) {
                stem += static_cast<wchar_t>(static_cast<unsigned char>(*p));
            }
            lang.locale = stem;
            lang.nativeName = stem;
        }
        g_languages.push_back(lang);
    }

    // en-US 必须是下标 0：找不到英语包时把回退目标指向第一个可用包
    if (FindByLocale(L"en-US") != 0 && !g_languages.empty()) {
        for (size_t i = 0; i < g_languages.size(); ++i) {
            if (PrimarySubtag(g_languages[i].locale) == L"en") {
                std::swap(g_languages[0], g_languages[i]);
                break;
            }
        }
    }
}

std::wstring RegistryLanguage() {
    wchar_t value[32] = {0};
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return std::wstring();
    }
    DWORD bytes = sizeof(value) - sizeof(wchar_t);
    DWORD type = 0;
    const LONG result =
        RegQueryValueExW(key, kRegValue, nullptr, &type, reinterpret_cast<LPBYTE>(value), &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_SZ) return std::wstring();
    return std::wstring(value);
}

void WriteRegistryLanguage(const std::wstring& locale) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS) {
        return;
    }
    RegSetValueExW(key, kRegValue, 0, REG_SZ, reinterpret_cast<const BYTE*>(locale.c_str()),
                   static_cast<DWORD>((locale.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
}

// 系统界面语言 → BCP-47 标签（Win7 有 LCIDToLocaleName）
std::wstring SystemUiLanguage() {
    wchar_t name[LOCALE_NAME_MAX_LENGTH] = {0};
    if (LCIDToLocaleName(GetUserDefaultUILanguage(), name, LOCALE_NAME_MAX_LENGTH, 0) == 0) {
        return std::wstring();
    }
    return std::wstring(name);
}

}  // namespace

void InitLang() {
    LoadLanguages();
    if (g_languages.empty()) return;

    int index = FindByLocale(RegistryLanguage());
    if (index < 0) index = FindByLocale(SystemUiLanguage());
    if (index < 0) index = 0;  // 默认英语
    g_current = index;
}

const std::vector<LangDef>& Languages() {
    return g_languages;
}

int CurrentLangIndex() {
    return g_current;
}

bool SetLangByIndex(int index) {
    if (index < 0 || index >= static_cast<int>(g_languages.size())) return false;
    g_current = index;
    WriteRegistryLanguage(g_languages[g_current].locale);
    return true;
}

bool SetLangByLocale(const std::wstring& locale) {
    const int index = FindByLocale(locale);
    if (index < 0) return false;
    g_current = index;
    return true;
}

const wchar_t* T(StrId id) {
    const size_t index = static_cast<size_t>(id);
    if (index >= static_cast<size_t>(StrId::Count)) return L"";
    const wchar_t* key = kStrKeys[index];

    const std::wstring* value = FindValue(g_languages[g_current], key);
    if (value != nullptr && !value->empty()) return value->c_str();
    if (g_current != 0) {
        value = FindValue(g_languages[0], key);
        if (value != nullptr && !value->empty()) return value->c_str();
    }
    return key;  // 英语也缺：回显键名，便于语言包作者发现漏译
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

std::wstring Tf(StrId id, const wchar_t* s1, const wchar_t* s2, const wchar_t* s3,
                const wchar_t* s4) {
    const Arg args[] = {{false, 0, s1}, {false, 0, s2}, {false, 0, s3}, {false, 0, s4}};
    return Expand(id, args, 4);
}

const wchar_t* LangName(int index) {
    if (index < 0 || index >= static_cast<int>(g_languages.size())) return L"";
    return g_languages[index].nativeName.c_str();
}

std::wstring LangButtonText() {
    return std::wstring(LangName(g_current)) + L" " + T(StrId::LangArrow);
}

std::wstring DirWord(core::SortDir dir) {
    if (dir == core::SortDir::None) return std::wstring();
    return dir == core::SortDir::Desc ? T(StrId::DirDesc) : T(StrId::DirAsc);
}

std::wstring KeyHintSentence(int diffSeg, const std::wstring& preview) {
    if (diffSeg == core::kNoSegment) return T(StrId::HintNoDiff);

    // 差异段之前的段：英文 "Octets A, B" / 中文 "A、B"
    std::wstring joined;
    for (int i = 0; i < diffSeg; ++i) {
        if (i) joined += T(StrId::HintJoinSep);
        joined += core::kSegmentNames[i];
    }
    const std::wstring head =
        Tf(diffSeg > 1 ? StrId::HintIdentMany : StrId::HintIdentOne, joined.c_str());
    const std::wstring seg = core::kSegmentNames[diffSeg];
    return Tf(StrId::HintMulti, head.c_str(), seg.c_str(), preview.c_str(), seg.c_str());
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

const wchar_t* UiFontFace() {
    const std::wstring primary = PrimarySubtag(g_languages.empty() ? L"en" : g_languages[g_current].locale);
    if (primary == L"zh") return L"Microsoft YaHei";
    if (primary == L"ja") return L"Meiryo";
    return L"Segoe UI";  // en/fr/it/ru 及一切其它：Win7 起自带
}

}  // namespace ui
