#include "core/sorter.h"

#include <algorithm>

#include "core/comparator.h"

namespace core {

std::vector<Ipv4Entry> SortAddresses(const std::vector<Ipv4Entry>& list, SortDir dir, int keySeg) {
    std::vector<Ipv4Entry> sorted = list;
    const int sign = (dir == SortDir::Desc) ? -1 : 1;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [sign, keySeg](const Ipv4Entry& a, const Ipv4Entry& b) {
                         return sign * CompareAddress(a, b, keySeg) < 0;
                     });
    return sorted;
}

}  // namespace core
