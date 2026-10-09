#pragma once

#include <windows.h>

#include <string>

namespace util {

// FR-6.1：以 Unicode 文本写入剪贴板。失败返回 false，不抛出异常。
bool CopyTextToClipboard(HWND owner, const std::wstring& text);

}  // namespace util
