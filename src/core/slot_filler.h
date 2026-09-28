#pragma once

#include <vector>

#include "core/types.h"

namespace core {

// FR-4.2 ~ FR-4.7 / §7.4 槽位生成：
//   按前 k 段（k = 段位）分组，第 k 段段值序列 1…255（存在段值 0 的地址时最前补 0），
//   低位段一律归 0，命中集合为空即空缺。
//   预估行数超过 kFillLimit 时返回 tooLarge（不铺槽位，由界面回退单列）。
FillResult BuildFilled(const std::vector<Ipv4Entry>& sorted, SortDir dir, int keySeg);

}  // namespace core
