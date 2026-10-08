#pragma once

#include <string>
#include <vector>

#include "core/types.h"

namespace core {

// FR-1.1 分词：按行拆分 → 剥离 # 与 // 注释 → 按分隔符集拆词 → 丢弃空词元。
// 词元保留 1 起始的行号，供 FR-1.5 的无效项提示使用。
std::vector<Token> Tokenize(const std::wstring& text);

}  // namespace core
