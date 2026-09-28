#pragma once

#include <string>
#include <vector>

// 核心逻辑层的数据结构定义。
// 本层禁止包含任何 Windows 头文件（文档 §8.3 硬性约束 1）。
namespace core {

constexpr int kSegmentCount = 4;      // IPv4 点分四段
constexpr int kMaxOctet = 255;        // 每段取值上限
constexpr int kNoPrefix = -1;         // 无掩码；比较时按 kFullPrefix 处理
constexpr int kFullPrefix = 32;       // FR-2.3：无掩码视为 /32
constexpr int kNoSegment = -1;        // 无差异段（FR-3.2）
constexpr int kFillLimit = 8192;      // FR-4.7 槽位行数上限
constexpr int kMaxFaultsShown = 6;    // FR-1.5 无效项最多显示条数

// 段名，用于界面提示（FR-3.4）
inline const wchar_t* const kSegmentNames[kSegmentCount] = {L"A", L"B", L"C", L"D"};

enum class SortDir { None, Asc, Desc };

// 成功解析的一条地址
struct Ipv4Entry {
    int octets[kSegmentCount];  // 四段整数
    int prefix;                 // 掩码长度；kNoPrefix 表示无掩码
    std::wstring display;       // 归一化后的显示串（含掩码）
};

// 分词产生的一个词元（带 1 起始行号）
struct Token {
    std::wstring raw;
    int line;
};

// 解析失败的一个词元
struct InvalidItem {
    std::wstring raw;
    int line;
    std::wstring reason;
};

struct ParseResult {
    bool ok;
    Ipv4Entry entry;
    std::wstring reason;
};

// 分词 → 解析 →（可选）去重 → 判段 的总结果
struct Analysis {
    std::vector<Ipv4Entry> valid;
    std::vector<InvalidItem> invalid;
    int diffSeg;       // 差异段序号；kNoSegment 表示无差异段
    bool deduped;      // 是否执行过合并重复
};

// 补位结果中的一行
struct SlotRow {
    int slot[kSegmentCount];  // 槽位地址
    int entryIndex;           // 命中项在 sorted 中的下标；-1 表示空缺
    bool showSlot;            // 同槽位多条时，槽位地址仅首行显示（FR-4.5）
};

struct FillResult {
    bool tooLarge;          // FR-4.7：超过上限，未补位
    long long estimate;     // 预估行数（分组数 × 255）
    int total;              // 实际行数
    int gaps;               // 未命中的槽位个数（FR-4.8）
    int keySeg;             // 实际使用的段位
    std::vector<SlotRow> rows;
};

// 排序 +（可选）补位后的完整视图
struct ViewResult {
    std::vector<Ipv4Entry> sorted;
    FillResult fill;
    bool useFill;  // 是否按双列补位呈现；超限时为 false（回退单列）
};

}  // namespace core
