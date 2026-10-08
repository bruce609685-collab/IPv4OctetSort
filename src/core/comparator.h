#pragma once

#include "core/types.h"

namespace core {

// 无掩码按 /32 处理（FR-2.3）
int PrefixOf(const Ipv4Entry& entry);

// §7.2 比较函数：指定段位优先，其余段按 A→B→C→D 兜底；
// 四段全等时掩码小者（网段更宽）在前。
int CompareAddress(const Ipv4Entry& a, const Ipv4Entry& b, int keySeg);

}  // namespace core
