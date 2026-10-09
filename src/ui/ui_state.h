#pragma once

#include <vector>

#include "core/types.h"

// 界面共享状态（§8.3 第 4 条：ui 层内部通过本文件共享状态）。
namespace ui {

struct UiState {
    core::SortDir mode = core::SortDir::None;  // 当前排序方向
    int keySeg = core::kNoSegment;             // 当前排序段位（= 差异段）
    int diffSeg = core::kNoSegment;            // 差异段判定结果
    std::vector<core::Ipv4Entry> valid;        // 有效地址
    std::vector<core::InvalidItem> invalid;    // 无效项
    core::ViewResult view;                     // 最近一次渲染使用的视图
    bool hasView = false;                      // view 是否可用于渲染（已排序）
    bool dedupeChecked = false;
    bool fillChecked = false;
    bool copyMsgVisible = false;
};

inline UiState& State() {
    static UiState state;
    return state;
}

}  // namespace ui
