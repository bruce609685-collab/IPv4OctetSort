#include "core/lng_file.h"

#include <fstream>
#include <sstream>

namespace core {
namespace {

bool IsKeyStart(wchar_t c) {
    return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || c == L'_';
}

bool IsKeyChar(wchar_t c) {
    return IsKeyStart(c) || (c >= L'0' && c <= L'9');
}

bool IsHexDigit(wchar_t c) {
    return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F');
}

int HexValue(wchar_t c) {
    if (c >= L'0' && c <= L'9') return c - L'0';
    if (c >= L'a' && c <= L'f') return c - L'a' + 10;
    return c - L'A' + 10;
}

std::wstring Trim(const std::wstring& text) {
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && (text[begin] == L' ' || text[begin] == L'\t' ||
                           text[begin] == L'\r' || text[begin] == L'\n')) {
        ++begin;
    }
    while (end > begin && (text[end - 1] == L' ' || text[end - 1] == L'\t' ||
                           text[end - 1] == L'\r' || text[end - 1] == L'\n')) {
        --end;
    }
    return text.substr(begin, end - begin);
}

// UTF-8 → UTF-16。严格拒绝：截断序列、overlong、代理区、超过 U+10FFFF；
// 遇到坏字节即失败（语言包必须干净，宁可拒绝不可乱显）。
bool DecodeUtf8(const std::string& in, std::wstring* out) {
    out->clear();
    out->reserve(in.size());

    size_t i = 0;
    if (in.size() >= 3 && static_cast<unsigned char>(in[0]) == 0xEF &&
        static_cast<unsigned char>(in[1]) == 0xBB &&
        static_cast<unsigned char>(in[2]) == 0xBF) {
        i = 3;  // BOM
    }

    while (i < in.size()) {
        const unsigned char c = static_cast<unsigned char>(in[i]);
        if (c < 0x80) {
            out->push_back(static_cast<wchar_t>(c));
            ++i;
            continue;
        }

        int length = 0;
        unsigned int code = 0;
        if (c >= 0xC2 && c <= 0xDF) {
            length = 2;
            code = c & 0x1F;
        } else if (c >= 0xE0 && c <= 0xEF) {
            length = 3;
            code = c & 0x0F;
        } else if (c >= 0xF0 && c <= 0xF4) {
            length = 4;
            code = c & 0x07;
        } else {
            return false;
        }
        if (i + static_cast<size_t>(length) > in.size()) return false;

        for (int k = 1; k < length; ++k) {
            const unsigned char cc = static_cast<unsigned char>(in[i + k]);
            if ((cc & 0xC0) != 0x80) return false;
            code = (code << 6) | (cc & 0x3F);
        }
        // overlong / 代理区 / 上限
        if ((length == 2 && code < 0x80) || (length == 3 && code < 0x800) ||
            (length == 4 && code < 0x10000) || (code >= 0xD800 && code <= 0xDFFF) ||
            code > 0x10FFFF) {
            return false;
        }

        if (code <= 0xFFFF) {
            out->push_back(static_cast<wchar_t>(code));
        } else {
            code -= 0x10000;
            out->push_back(static_cast<wchar_t>(0xD800 + (code >> 10)));
            out->push_back(static_cast<wchar_t>(0xDC00 + (code & 0x3FF)));
        }
        i += static_cast<size_t>(length);
    }
    return true;
}

// 展开值内的转义：\t \n \r \\ \xNN（两位十六进制）；其余反斜杠原样保留两个字符
std::wstring Unescape(const std::wstring& value) {
    std::wstring out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == L'\\' && i + 1 < value.size()) {
            const wchar_t next = value[i + 1];
            if (next == L't') {
                out.push_back(L'\t');
                ++i;
                continue;
            }
            if (next == L'n') {
                out.push_back(L'\n');
                ++i;
                continue;
            }
            if (next == L'r') {
                out.push_back(L'\r');
                ++i;
                continue;
            }
            if (next == L'\\') {
                out.push_back(L'\\');
                ++i;
                continue;
            }
            if (next == L'x' && i + 3 < value.size() &&
                IsHexDigit(value[i + 2]) && IsHexDigit(value[i + 3])) {
                const int high = HexValue(value[i + 2]);
                const int low = HexValue(value[i + 3]);
                out.push_back(static_cast<wchar_t>(high * 16 + low));
                i += 3;
                continue;
            }
        }
        out.push_back(value[i]);
    }
    return out;
}

bool HasKey(const std::vector<LngEntry>& entries, const std::wstring& key) {
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].key == key) return true;
    }
    return false;
}

}  // namespace

LngFile ParseLng(const std::string& utf8Text) {
    LngFile result;

    std::wstring text;
    if (!DecodeUtf8(utf8Text, &text)) {
        result.error = L"UTF-8 decoding failed";
        result.errorLine = 0;
        return result;
    }

    std::wistringstream lines(text);
    std::wstring raw;
    int lineNo = 0;
    while (std::getline(lines, raw)) {
        ++lineNo;
        if (!raw.empty() && raw.back() == L'\r') raw.pop_back();

        const std::wstring line = Trim(raw);
        if (line.empty()) continue;
        if (line[0] == L'#' || line[0] == L';') continue;

        const size_t equal = line.find(L'=');
        if (equal == std::wstring::npos) {
            result.error = L"missing '='";
            result.errorLine = lineNo;
            return result;
        }

        const std::wstring key = Trim(line.substr(0, equal));
        if (key.empty() || !IsKeyStart(key[0])) {
            result.error = L"invalid key";
            result.errorLine = lineNo;
            return result;
        }
        for (size_t i = 1; i < key.size(); ++i) {
            if (!IsKeyChar(key[i])) {
                result.error = L"invalid key";
                result.errorLine = lineNo;
                return result;
            }
        }
        if (key.size() > 64) {
            result.error = L"key too long";
            result.errorLine = lineNo;
            return result;
        }

        const std::wstring value = Unescape(Trim(line.substr(equal + 1)));
        if (value.size() > 1024) {
            result.error = L"value too long";
            result.errorLine = lineNo;
            return result;
        }

        if (!HasKey(result.entries, key)) {
            LngEntry entry;
            entry.key = key;
            entry.value = value;
            result.entries.push_back(entry);
        }
        // 重名键以第一条为准：允许语言包在文件尾放覆盖段，但不鼓励
    }

    result.ok = true;
    return result;
}

LngFile LoadLngFile(const std::wstring& path) {
    LngFile result;
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) {
        result.error = L"cannot open file";
        return result;
    }
    std::ostringstream content;
    content << file.rdbuf();
    return ParseLng(content.str());
}

}  // namespace core
