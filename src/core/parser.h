#pragma once

#include <string>

#include "core/types.h"

namespace core {

// 格式不合法时的原因文案（FR-1.5）
extern const wchar_t* const kFormatReason;

// FR-1.2 ~ FR-1.4 / §7.1：单地址解析与有效性校验，成功后生成归一化显示串。
// 接受 `192.168.1.1` 与 `10.0.0.0/24` 两种写法。
ParseResult ParseAddress(const std::wstring& raw);

}  // namespace core
