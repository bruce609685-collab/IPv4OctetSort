#include "core/tokenizer.h"

namespace core {
namespace {

// 分隔符集：空白、逗号、分号、顿号、中文逗号、制表符（FR-1.1）
bool IsSeparator(wchar_t c) {
    if (c == L',' || c == L';' || c == L' ') return true;
    if (c == 0x3001 || c == 0xFF0C) return true;  // 、 ，
    if (c == 0x3000) return true;                 // 全角空格
    return c < 0x20;                              // 制表符与其余控制字符
}

// 剥离注释：先截断第一个 #，再截断第一个 //（FR-1.1 第 2 步）
std::wstring StripComment(const std::wstring& line) {
    std::wstring out = line;
    size_t hash = out.find(L'#');
    if (hash != std::wstring::npos) out.erase(hash);
    size_t slash = out.find(L"//");
    if (slash != std::wstring::npos) out.erase(slash);
    return out;
}

}  // namespace

std::vector<Token> Tokenize(const std::wstring& text) {
    std::vector<Token> tokens;
    size_t pos = 0;
    int lineNo = 1;

    while (pos <= text.size()) {
        size_t end = text.find(L'\n', pos);
        std::wstring line = (end == std::wstring::npos) ? text.substr(pos)
                                                        : text.substr(pos, end - pos);
        if (!line.empty() && line.back() == L'\r') line.pop_back();

        const std::wstring clean = StripComment(line);

        size_t i = 0;
        while (i < clean.size()) {
            while (i < clean.size() && IsSeparator(clean[i])) ++i;
            const size_t start = i;
            while (i < clean.size() && !IsSeparator(clean[i])) ++i;
            if (i > start) {
                Token token;
                token.raw = clean.substr(start, i - start);
                token.line = lineNo;
                tokens.push_back(token);
            }
        }

        if (end == std::wstring::npos) break;
        pos = end + 1;
        ++lineNo;
    }

    return tokens;
}

}  // namespace core
