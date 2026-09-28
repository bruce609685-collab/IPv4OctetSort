#include "core/slot_filler.h"

#include <algorithm>
#include <map>

namespace core {
namespace {

// 一个高位段分组：high 为前 k 段取值，items 为 sorted 中的下标
struct Group {
    std::vector<int> high;
    std::vector<int> items;
};

std::wstring HighKey(const int* octets, int count) {
    std::wstring key;
    for (int i = 0; i < count; ++i) {
        if (i) key += L'.';
        key += std::to_wstring(octets[i]);
    }
    return key;
}

bool LessGroup(const Group& a, const Group& b) {
    const size_t n = a.high.size() < b.high.size() ? a.high.size() : b.high.size();
    for (size_t i = 0; i < n; ++i) {
        if (a.high[i] != b.high[i]) return a.high[i] < b.high[i];
    }
    return false;
}

}  // namespace

FillResult BuildFilled(const std::vector<Ipv4Entry>& sorted, SortDir dir, int keySeg) {
    FillResult result;
    result.tooLarge = false;
    result.estimate = 0;
    result.total = 0;
    result.gaps = 0;

    const int k = (keySeg == kNoSegment) ? kSegmentCount - 1 : keySeg;
    result.keySeg = k;

    // 1. 按前 k 段取值分组（分组内保持 sorted 的先后）
    std::vector<Group> groups;
    std::map<std::wstring, size_t> indexOf;
    for (size_t i = 0; i < sorted.size(); ++i) {
        const std::wstring key = HighKey(sorted[i].octets, k);
        const std::map<std::wstring, size_t>::iterator it = indexOf.find(key);
        size_t groupIndex;
        if (it == indexOf.end()) {
            Group group;
            for (int s = 0; s < k; ++s) group.high.push_back(sorted[i].octets[s]);
            groups.push_back(group);
            groupIndex = groups.size() - 1;
            indexOf[key] = groupIndex;
        } else {
            groupIndex = it->second;
        }
        groups[groupIndex].items.push_back(static_cast<int>(i));
    }

    // 分组排序：高位段升序；降序时整体反转（FR-4.6）
    std::stable_sort(groups.begin(), groups.end(), LessGroup);
    if (dir == SortDir::Desc) std::reverse(groups.begin(), groups.end());

    // 2. FR-4.7 数量上限保护（分组数 × 255）
    result.estimate = static_cast<long long>(groups.size()) * 255;
    if (result.estimate > kFillLimit) {
        result.tooLarge = true;
        return result;
    }

    // 3. 段值序列 1…255（0 由各分组按需补入，见下）
    std::vector<int> segValues;
    segValues.reserve(255);
    for (int v = 1; v <= 255; ++v) segValues.push_back(v);

    // 4. 铺槽位并匹配命中（FR-4.4 / FR-4.5）
    for (size_t g = 0; g < groups.size(); ++g) {
        const Group& group = groups[g];

        std::vector<int> items = group.items;
        if (dir == SortDir::Desc) std::reverse(items.begin(), items.end());

        // 按第 k 段取值分桶
        std::map<int, std::vector<int> > buckets;
        for (size_t i = 0; i < items.size(); ++i) {
            buckets[sorted[items[i]].octets[k]].push_back(items[i]);
        }

        // FR-4.3 段值为 0：0 是最小值，升序排在 1 之前，降序随整条序列反转到末尾
        std::vector<int> values = segValues;
        if (!buckets.empty() && buckets.begin()->first == 0) values.insert(values.begin(), 0);
        if (dir == SortDir::Desc) std::reverse(values.begin(), values.end());

        for (size_t s = 0; s < values.size(); ++s) {
            const int seg = values[s];

            SlotRow row;
            for (int i = 0; i < kSegmentCount; ++i) row.slot[i] = 0;
            for (int i = 0; i < k; ++i) row.slot[i] = group.high[i];
            row.slot[k] = seg;

            const std::map<int, std::vector<int> >::const_iterator hit = buckets.find(seg);
            if (hit == buckets.end() || hit->second.empty()) {
                row.entryIndex = -1;
                row.showSlot = true;
                ++result.gaps;
                result.rows.push_back(row);
            } else {
                const std::vector<int>& hits = hit->second;
                for (size_t h = 0; h < hits.size(); ++h) {
                    row.entryIndex = hits[h];
                    row.showSlot = (h == 0);  // 槽位地址仅首行显示
                    result.rows.push_back(row);
                }
            }
        }
    }

    result.total = static_cast<int>(result.rows.size());
    return result;
}

}  // namespace core
