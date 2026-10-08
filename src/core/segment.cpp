#include "core/segment.h"

namespace core {

int DetectDiffSeg(const std::vector<Ipv4Entry>& list) {
    if (list.size() < 2) return kNoSegment;

    for (int i = 0; i < kSegmentCount; ++i) {
        const int first = list[0].octets[i];
        for (size_t j = 1; j < list.size(); ++j) {
            if (list[j].octets[i] != first) return i;
        }
    }
    return kNoSegment;
}

std::wstring DiffValuePreview(const std::vector<Ipv4Entry>& list, int segIndex) {
    const DiffPreview p = DiffValues(list, segIndex);
    if (!p.truncated) return p.head;
    return p.head + L" 等 " + std::to_wstring(p.total) + L" 个值";
}

DiffPreview DiffValues(const std::vector<Ipv4Entry>& list, int segIndex) {
    std::vector<int> seen;
    for (size_t j = 0; j < list.size(); ++j) {
        const int value = list[j].octets[segIndex];
        bool found = false;
        for (size_t k = 0; k < seen.size(); ++k) {
            if (seen[k] == value) {
                found = true;
                break;
            }
        }
        if (!found) seen.push_back(value);
    }

    DiffPreview out;
    const size_t shown = seen.size() < 3 ? seen.size() : 3;
    for (size_t i = 0; i < shown; ++i) {
        if (i) out.head += L" / ";
        out.head += std::to_wstring(seen[i]);
    }
    out.truncated = seen.size() > 3;
    out.total = static_cast<int>(seen.size());
    return out;
}

}  // namespace core
