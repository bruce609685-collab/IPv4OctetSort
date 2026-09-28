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

    std::wstring head;
    const size_t shown = seen.size() < 3 ? seen.size() : 3;
    for (size_t i = 0; i < shown; ++i) {
        if (i) head += L" / ";
        head += std::to_wstring(seen[i]);
    }
    if (seen.size() > 3) {
        head += L" 等 " + std::to_wstring(seen.size()) + L" 个值";
    }
    return head;
}

}  // namespace core
