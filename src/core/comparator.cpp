#include "core/comparator.h"

namespace core {

int PrefixOf(const Ipv4Entry& entry) {
    return entry.prefix == kNoPrefix ? kFullPrefix : entry.prefix;
}

int CompareAddress(const Ipv4Entry& a, const Ipv4Entry& b, int keySeg) {
    int order[kSegmentCount];
    int count = 0;
    if (keySeg != kNoSegment) order[count++] = keySeg;
    for (int i = 0; i < kSegmentCount; ++i) {
        if (i != keySeg) order[count++] = i;
    }

    for (int n = 0; n < count; ++n) {
        const int i = order[n];
        if (a.octets[i] != b.octets[i]) return a.octets[i] - b.octets[i];
    }
    return PrefixOf(a) - PrefixOf(b);
}

}  // namespace core
