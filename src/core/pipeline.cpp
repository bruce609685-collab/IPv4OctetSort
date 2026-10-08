#include "core/pipeline.h"

#include "core/dedupe.h"
#include "core/parser.h"
#include "core/segment.h"
#include "core/slot_filler.h"
#include "core/sorter.h"
#include "core/tokenizer.h"

namespace core {

Analysis Analyze(const std::wstring& text, bool dedupe) {
    Analysis analysis;
    analysis.diffSeg = kNoSegment;
    analysis.deduped = false;

    const std::vector<Token> tokens = Tokenize(text);
    std::vector<Ipv4Entry> parsed;
    parsed.reserve(tokens.size());

    for (size_t i = 0; i < tokens.size(); ++i) {
        const ParseResult one = ParseAddress(tokens[i].raw);
        if (one.ok) {
            parsed.push_back(one.entry);
        } else {
            InvalidItem item;
            item.raw = tokens[i].raw;
            item.line = tokens[i].line;
            item.reason = one.reason;
            item.reasonKind = one.reasonKind;
            item.reasonArg = one.reasonArg;
            analysis.invalid.push_back(item);
        }
    }

    analysis.valid = parsed;
    if (dedupe && !parsed.empty()) {
        analysis.valid = Dedupe(parsed);
        analysis.deduped = true;
    }

    // 以去重后的清单重新判定差异段（FR-5 处理顺序的最后一步）
    analysis.diffSeg = DetectDiffSeg(analysis.valid);
    return analysis;
}

ViewResult BuildView(const Analysis& analysis, SortDir dir, bool useFill) {
    ViewResult view;
    view.useFill = false;
    view.fill.tooLarge = false;
    view.fill.estimate = 0;
    view.fill.total = 0;
    view.fill.gaps = 0;
    view.fill.keySeg = kNoSegment;

    if (dir == SortDir::None || analysis.valid.empty()) return view;

    view.sorted = SortAddresses(analysis.valid, dir, analysis.diffSeg);
    if (!useFill) return view;

    view.fill = BuildFilled(view.sorted, dir, analysis.diffSeg);
    view.useFill = !view.fill.tooLarge;  // 超限时界面回退单列（FR-4.7）
    return view;
}

}  // namespace core
