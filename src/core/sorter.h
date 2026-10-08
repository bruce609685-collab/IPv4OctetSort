#pragma once

#include <vector>

#include "core/types.h"

namespace core {

// FR-2.2 / FR-2.4：返回排序后的新列表，不修改入参；
// 使用稳定排序，排序键相同的条目保持输入先后顺序。
std::vector<Ipv4Entry> SortAddresses(const std::vector<Ipv4Entry>& list, SortDir dir, int keySeg);

}  // namespace core
