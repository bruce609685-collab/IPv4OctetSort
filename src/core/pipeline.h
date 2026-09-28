#pragma once

#include <string>

#include "core/types.h"

namespace core {

// FR-1.6 / §8.4.1：核心处理流水线（对外唯一入口）。
// 分词 → 解析 →（可选）合并重复 → 差异段判定。
Analysis Analyze(const std::wstring& text, bool dedupe);

// 排序 +（可选）补位，得到一次渲染所需的视图。
// useFill 为 true 但预估超限时，返回的 view.useFill 为 false（回退单列，FR-4.7），
// 同时 view.fill.tooLarge 为 true 且 view.fill.estimate 保存预估行数。
ViewResult BuildView(const Analysis& analysis, SortDir dir, bool useFill);

}  // namespace core
