#include "core/parser.h"

namespace core {

const wchar_t* const kFormatReason = L"格式不是 IPv4 点分十进制";

namespace {

bool IsDigit(wchar_t c) { return c >= L'0' && c <= L'9'; }

std::wstring Trim(const std::wstring& text) {
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && (text[begin] == L' ' || text[begin] == L'\t')) ++begin;
    while (end > begin && (text[end - 1] == L' ' || text[end - 1] == L'\t')) --end;
    return text.substr(begin, end - begin);
}

ParseResult Fail(const wchar_t* reason, ReasonKind kind = ReasonKind::Format, int arg = 0) {
    ParseResult result;
    result.ok = false;
    result.reason = reason;
    result.reasonKind = kind;
    result.reasonArg = arg;
    result.entry.prefix = kNoPrefix;
    for (int i = 0; i < kSegmentCount; ++i) result.entry.octets[i] = 0;
    return result;
}

// 逐位读取十进制整数；不允许非数字字符
bool ReadNumber(const std::wstring& text, size_t& pos, size_t maxDigits, int& value) {
    const size_t start = pos;
    while (pos < text.size() && IsDigit(text[pos])) ++pos;
    const size_t digits = pos - start;
    if (digits == 0 || digits > maxDigits) return false;

    int number = 0;
    for (size_t i = start; i < pos; ++i) number = number * 10 + (text[i] - L'0');
    value = number;
    return true;
}

}  // namespace

ParseResult ParseAddress(const std::wstring& raw) {
    const std::wstring text = Trim(raw);
    if (text.empty()) return Fail(kFormatReason);

    // 掩码部分：至多一个 '/'，其后 1-2 位数字（§7.1 第 1 步）
    std::wstring addressPart = text;
    std::wstring prefixPart;
    bool hasPrefix = false;
    const size_t slash = text.find(L'/');
    if (slash != std::wstring::npos) {
        if (text.find(L'/', slash + 1) != std::wstring::npos) return Fail(kFormatReason);
        addressPart = text.substr(0, slash);
        prefixPart = text.substr(slash + 1);
        hasPrefix = true;
    }

    // 四段：每段 1-3 位数字，段间以 '.' 分隔
    int octets[kSegmentCount] = {0, 0, 0, 0};
    size_t pos = 0;
    for (int i = 0; i < kSegmentCount; ++i) {
        if (!ReadNumber(addressPart, pos, 3, octets[i])) return Fail(kFormatReason);
        if (i + 1 < kSegmentCount) {
            if (pos >= addressPart.size() || addressPart[pos] != L'.') return Fail(kFormatReason);
            ++pos;
        }
    }
    if (pos != addressPart.size()) return Fail(kFormatReason);

    // 段范围（§7.1 第 2 步）
    for (int i = 0; i < kSegmentCount; ++i) {
        if (octets[i] > kMaxOctet) {
            return Fail((L"第 " + std::to_wstring(i + 1) + L" 段超出 0-255").c_str(),
                        ReasonKind::OctetRange, i + 1);
        }
    }

    // 掩码范围（§7.1 第 3 步）
    int prefix = kNoPrefix;
    if (hasPrefix) {
        size_t prefixPos = 0;
        if (!ReadNumber(prefixPart, prefixPos, 2, prefix) || prefixPos != prefixPart.size()) {
            return Fail(kFormatReason);
        }
        if (prefix > kFullPrefix) return Fail(L"掩码长度超出 0-32", ReasonKind::PrefixRange);
    }

    // 归一化显示串（FR-1.4：前导零自动去除）
    std::wstring display;
    for (int i = 0; i < kSegmentCount; ++i) {
        if (i) display += L'.';
        display += std::to_wstring(octets[i]);
    }
    if (hasPrefix) display += L"/" + std::to_wstring(prefix);

    ParseResult result;
    result.ok = true;
    for (int i = 0; i < kSegmentCount; ++i) result.entry.octets[i] = octets[i];
    result.entry.prefix = prefix;
    result.entry.display = display;
    return result;
}

}  // namespace core
