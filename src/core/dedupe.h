#pragma once

#include <vector>

#include "core/types.h"

namespace core {

// FR-5 / §7.5 合并重复：
//   去重键 = 槽位身份 octets[0..k]（k 为差异段；无差异段时按完整四段）；
//   同组保留掩码最小（网段最宽）者，掩码并列保留首次出现者；
//   输出顺序按各组首次出现的先后。
std::vector<Ipv4Entry> Dedupe(const std::vector<Ipv4Entry>& list);

}  // namespace core
