#pragma once

#include <string>

namespace util {

// FR-8：调用系统默认浏览器打开链接（程序自身不联网）
bool OpenUrl(const std::wstring& url);

}  // namespace util
