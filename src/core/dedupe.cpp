#include "core/dedupe.h"

#include <map>

#include "core/comparator.h"
#include "core/segment.h"

namespace core {
namespace {

// 去重键：槽位身份 octets[0..lastSeg]（§7.5 第 2 步）
std::wstring SlotKey(const Ipv4Entry& entry, int lastSeg) {
    std::wstring key;
    for (int i = 0; i <= lastSeg; ++i) {
        if (i) key += L'.';
        key += std::to_wstring(entry.octets[i]);
    }
    return key;
}

}  // namespace

std::vector<Ipv4Entry> Dedupe(const std::vector<Ipv4Entry>& list) {
    std::vector<Ipv4Entry> out;
    if (list.empty()) return out;

    const int diffSeg = DetectDiffSeg(list);
    const int lastSeg = (diffSeg == kNoSegment) ? kSegmentCount - 1 : diffSeg;

    std::map<std::wstring, size_t> indexOf;
    for (size_t i = 0; i < list.size(); ++i) {
        const Ipv4Entry& entry = list[i];
        const std::wstring key = SlotKey(entry, lastSeg);
        const std::map<std::wstring, size_t>::iterator it = indexOf.find(key);
        if (it == indexOf.end()) {
            indexOf[key] = out.size();
            out.push_back(entry);
        } else if (PrefixOf(entry) < PrefixOf(out[it->second])) {
            out[it->second] = entry;  // 掩码更小者胜出；并列时保留先出现者
        }
    }
    return out;
}

}  // namespace core
