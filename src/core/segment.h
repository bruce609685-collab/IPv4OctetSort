#pragma once

#include <string>
#include <vector>

#include "core/types.h"

namespace core {

// §7.3 差异段判定：A→B→C→D 中第一个取值不唯一的段；
// 有效地址不足 2 条或四段取值完全一致时返回 kNoSegment。
int DetectDiffSeg(const std::vector<Ipv4Entry>& list);

// FR-3.4 某段的不同取值预览，最多列 3 个，超出时以「等 N 个值」收尾。
std::wstring DiffValuePreview(const std::vector<Ipv4Entry>& list, int segIndex);

}  // namespace core
