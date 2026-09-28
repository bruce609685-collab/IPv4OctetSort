#pragma once

#include <string>

namespace util {

// UTF-8 ↔ UTF-16 转换（文档 §8.4.3）
std::wstring WideFromUtf8(const std::string& text);
std::string Utf8FromWide(const std::wstring& text);

}  // namespace util
