// 核心逻辑单元测试（文档 §8.2 tests/，命令行运行，不依赖界面）。
// 覆盖 AC-01 ~ AC-19 的算法部分。
#include <cstdio>
#include <string>
#include <vector>

#include "core/comparator.h"
#include "core/dedupe.h"
#include "core/parser.h"
#include "core/pipeline.h"
#include "core/segment.h"
#include "core/slot_filler.h"
#include "core/sorter.h"
#include "core/tokenizer.h"

namespace {

int gPass = 0;
int gFail = 0;

// 输出保持纯 ASCII，避免控制台代码页干扰（地址与数字均为 ASCII）
std::string Narrow(const std::wstring& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        out += (text[i] < 128) ? static_cast<char>(text[i]) : '?';
    }
    return out;
}

void Check(const char* label, bool ok, const std::string& detail = std::string()) {
    if (ok) {
        ++gPass;
        std::printf("PASS  %s\n", label);
    } else {
        ++gFail;
        std::printf("FAIL  %s   [%s]\n", label, detail.c_str());
    }
}

std::wstring JoinDisplays(const std::vector<core::Ipv4Entry>& list) {
    std::wstring out;
    for (size_t i = 0; i < list.size(); ++i) {
        if (i) out += L",";
        out += list[i].display;
    }
    return out;
}

const wchar_t* SegName(int seg) {
    return seg == core::kNoSegment ? L"none" : core::kSegmentNames[seg];
}

// 段位提示文案（FR-3.4，与界面拼接规则一致）
std::wstring HintText(const core::Analysis& analysis) {
    if (analysis.valid.empty()) return L"";
    if (analysis.diffSeg == core::kNoSegment) {
        return L"四段取值一致，没有差异段；缺位填充按 D 段铺槽位";
    }
    std::wstring head;
    for (int i = 0; i < analysis.diffSeg; ++i) {
        if (i) head += L"、";
        head += core::kSegmentNames[i];
    }
    if (!head.empty()) head += L" 段取值一致，";
    head += core::kSegmentNames[analysis.diffSeg];
    head += L" 段有多个取值（";
    head += core::DiffValuePreview(analysis.valid, analysis.diffSeg);
    head += L"），仅可按 ";
    head += core::kSegmentNames[analysis.diffSeg];
    head += L" 段排序";
    return head;
}

void TestTokenizer() {
    const std::vector<core::Token> tokens =
        core::Tokenize(L"192.168.1.1 192.168.1.2,192.168.1.3\r\n"
                       L"192.168.1.4、192.168.1.5 ; # 注释 192.168.1.99\n"
                       L"192.168.1.6 // 行内注释 192.168.1.98\n"
                       L"\n"
                       L"10.0.0.1/24\t10.0.0.2");
    Check("FR-1.1 token count", tokens.size() == 8,
          "count=" + std::to_string(tokens.size()));
    Check("FR-1.1 line numbers", tokens.size() == 8 && tokens[0].line == 1 &&
                                     tokens[2].line == 1 && tokens[3].line == 2 &&
                                     tokens[4].line == 2 && tokens[5].line == 3 &&
                                     tokens.back().line == 5,
          tokens.size() == 8 ? "last line=" + std::to_string(tokens.back().line) : "-");
    Check("FR-1.1 comment stripped", tokens.size() == 8 && tokens[3].raw == L"192.168.1.4" &&
                                         tokens[5].raw == L"192.168.1.6",
          tokens.size() == 8 ? Narrow(tokens[5].raw) : "-");
    // 全角分号不在分隔符集内（与预览版 FR-1.1 的分隔符集合一致），会作为无效词元列出
    Check("FR-1.1 full width semicolon is not a separator",
          core::Tokenize(L"10.0.0.1；10.0.0.2").size() == 1,
          "count=" + std::to_string(core::Tokenize(L"10.0.0.1；10.0.0.2").size()));
}

void TestParser() {
    const core::ParseResult good = core::ParseAddress(L"192.168.1.1");
    Check("FR-1.2 dotted quad", good.ok && good.entry.display == L"192.168.1.1" &&
                                    good.entry.prefix == core::kNoPrefix,
          Narrow(good.entry.display));

    const core::ParseResult masked = core::ParseAddress(L"10.0.0.0/24");
    Check("FR-1.2 with prefix", masked.ok && masked.entry.prefix == 24 &&
                                    masked.entry.display == L"10.0.0.0/24",
          Narrow(masked.entry.display));

    const core::ParseResult leading = core::ParseAddress(L"192.168.001.008");
    Check("FR-1.4 normalize leading zeros", leading.ok && leading.entry.display == L"192.168.1.8",
          Narrow(leading.entry.display));

    const core::ParseResult outOfRange = core::ParseAddress(L"192.168.1.300");
    Check("FR-1.3 octet range", !outOfRange.ok &&
                                    outOfRange.reason == L"第 4 段超出 0-255",
          Narrow(outOfRange.reason));

    const core::ParseResult firstOctet = core::ParseAddress(L"300.1.1.1");
    Check("FR-1.3 first octet range", !firstOctet.ok &&
                                          firstOctet.reason == L"第 1 段超出 0-255",
          Narrow(firstOctet.reason));

    const core::ParseResult badPrefix = core::ParseAddress(L"10.0.0.0/33");
    Check("FR-1.3 prefix range", !badPrefix.ok &&
                                     badPrefix.reason == L"掩码长度超出 0-32",
          Narrow(badPrefix.reason));

    Check("FR-1.3 three segments rejected", !core::ParseAddress(L"192.168.1").ok);
    Check("FR-1.3 five segments rejected", !core::ParseAddress(L"192.168.1.1.1").ok);
    Check("FR-1.3 non digit rejected", !core::ParseAddress(L"192.168.1.a").ok);
    Check("FR-1.3 empty segment rejected", !core::ParseAddress(L"192..1.1").ok);
    Check("FR-1.3 long prefix rejected as format",
          !core::ParseAddress(L"10.0.0.0/123").ok &&
              core::ParseAddress(L"10.0.0.0/123").reason == core::kFormatReason);
    Check("FR-1.5 reason text", core::ParseAddress(L"abc").reason == L"格式不是 IPv4 点分十进制");
}

void TestSortAndSegment() {
    const core::Analysis a1 = core::Analyze(L"192.168.1.153\n192.168.1.8\n192.168.1.55\n192.168.1.17", false);
    const std::vector<core::Ipv4Entry> asc1 = core::SortAddresses(a1.valid, core::SortDir::Asc, a1.diffSeg);
    Check("AC-01 ascending by octet value",
          JoinDisplays(asc1) == L"192.168.1.8,192.168.1.17,192.168.1.55,192.168.1.153",
          Narrow(JoinDisplays(asc1)));
    Check("AC-04 diff segment = D", SegName(a1.diffSeg) == std::wstring(L"D"), Narrow(SegName(a1.diffSeg)));

    const core::Analysis a2 = core::Analyze(L"10.176.86.1\n10.176.127.1\n10.176.13.1\n10.176.223.1", false);
    const std::vector<core::Ipv4Entry> asc2 = core::SortAddresses(a2.valid, core::SortDir::Asc, a2.diffSeg);
    Check("AC-02 ascending C segment",
          JoinDisplays(asc2) == L"10.176.13.1,10.176.86.1,10.176.127.1,10.176.223.1",
          Narrow(JoinDisplays(asc2)));
    Check("AC-05 diff segment = C", SegName(a2.diffSeg) == std::wstring(L"C"), Narrow(SegName(a2.diffSeg)));

    const core::Analysis a3 = core::Analyze(
        L"192.168.1.153\n192.168.1.17\n10.176.86.1\n10.179.173.44", false);
    Check("AC-03 diff segment = A", SegName(a3.diffSeg) == std::wstring(L"A"), Narrow(SegName(a3.diffSeg)));

    const core::Analysis a4 = core::Analyze(
        L"10.1.180.3\n10.188.52.88\n10.179.173.44\n10.176.211.15", false);
    Check("AC-06 diff segment = B", SegName(a4.diffSeg) == std::wstring(L"B"), Narrow(SegName(a4.diffSeg)));

    // FR-2.3 同四段不同掩码：掩码小者在前（无掩码视为 /32，与前一条并列时保持输入先后）
    const core::Analysis a5 = core::Analyze(L"10.0.0.1\n10.0.0.1/24\n10.0.0.1/32", false);
    const std::vector<core::Ipv4Entry> asc5 = core::SortAddresses(a5.valid, core::SortDir::Asc, a5.diffSeg);
    Check("FR-2.3 smaller prefix first",
          JoinDisplays(asc5) == L"10.0.0.1/24,10.0.0.1,10.0.0.1/32",
          Narrow(JoinDisplays(asc5)));

    const core::Analysis a6 = core::Analyze(L"0.0.0.0\n255.255.255.255\n1.1.1.1", false);
    const std::vector<core::Ipv4Entry> asc6 = core::SortAddresses(a6.valid, core::SortDir::Asc, a6.diffSeg);
    Check("FR-2.5 boundary octets",
          JoinDisplays(asc6) == L"0.0.0.0,1.1.1.1,255.255.255.255", Narrow(JoinDisplays(asc6)));

    const core::Analysis a7 = core::Analyze(L"10.0.0.1/24\n10.0.0.1\n10.0.0.2", false);
    const std::vector<core::Ipv4Entry> desc7 = core::SortAddresses(a7.valid, core::SortDir::Desc, a7.diffSeg);
    Check("FR-2.2 descending", JoinDisplays(desc7) == L"10.0.0.2,10.0.0.1,10.0.0.1/24",
          Narrow(JoinDisplays(desc7)));

    const core::Analysis a8 = core::Analyze(L"10.0.0.1/24\n10.0.0.1", false);
    Check("FR-2.4 stable order kept",
          JoinDisplays(core::SortAddresses(a8.valid, core::SortDir::Asc, a8.diffSeg)) ==
              L"10.0.0.1/24,10.0.0.1");

    Check("FR-3.2 single address has no diff segment",
          core::Analyze(L"10.0.0.1", false).diffSeg == core::kNoSegment);
    Check("FR-3.2 identical addresses have no diff segment",
          core::Analyze(L"10.0.0.1\n10.0.0.1", false).diffSeg == core::kNoSegment);
    Check("FR-1.5 invalid items excluded from sorting",
          core::Analyze(L"10.0.0.1\n192.168.1.300", false).valid.size() == 1);

    const core::Analysis a9 = core::Analyze(L"10.176.86.1\n10.176.127.1\n10.176.13.1", false);
    Check("FR-3.4 value preview",
          core::DiffValuePreview(a9.valid, a9.diffSeg) == L"86 / 127 / 13",
          Narrow(core::DiffValuePreview(a9.valid, a9.diffSeg)));
    const core::Analysis a10 = core::Analyze(
        L"10.176.1.1\n10.176.2.1\n10.176.3.1\n10.176.4.1\n10.176.5.1", false);
    Check("FR-3.4 preview truncates after three",
          core::DiffValuePreview(a10.valid, a10.diffSeg) == L"1 / 2 / 3 等 5 个值",
          Narrow(core::DiffValuePreview(a10.valid, a10.diffSeg)));
    Check("FR-3.4 hint text / FR-4.3 hint", HintText(a9) == L"A、B 段取值一致，C 段有多个取值（86 / 127 / 13），仅可按 C 段排序",
          Narrow(HintText(a9)));
}

void TestFill() {
    const std::wstring bSeg = L"10.1.180.3\n10.188.52.88\n10.179.173.44\n10.176.211.15";
    const core::Analysis a1 = core::Analyze(bSeg, false);
    const core::ViewResult v1 = core::BuildView(a1, core::SortDir::Asc, true);
    Check("AC-07 fill enabled", v1.useFill && v1.fill.total == 255 && v1.fill.gaps == 251,
          "rows=" + std::to_string(v1.fill.total) + " gaps=" + std::to_string(v1.fill.gaps));
    Check("AC-07 first / last slot",
          v1.useFill && v1.fill.rows.front().slot[1] == 1 && v1.fill.rows.back().slot[1] == 255);
    Check("AC-07 no 0 slot without 0-value address",
          v1.useFill && v1.fill.rows.front().slot[1] != 0);

    const core::ViewResult v2 = core::BuildView(a1, core::SortDir::Desc, true);
    Check("AC-07 descending slot order",
          v2.useFill && v2.fill.rows.front().slot[1] == 255 && v2.fill.rows.back().slot[1] == 1);

    std::wstring many;
    for (int i = 1; i <= 255; ++i) {
        if (i == 7 || i == 201) continue;
        if (!many.empty()) many += L"\n";
        many += L"192.168.1." + std::to_wstring(i);
    }
    const core::ViewResult v3 = core::BuildView(core::Analyze(many, false), core::SortDir::Asc, true);
    Check("AC-08 255 rows / 2 gaps", v3.useFill && v3.fill.total == 255 && v3.fill.gaps == 2,
          "rows=" + std::to_string(v3.fill.total) + " gaps=" + std::to_string(v3.fill.gaps));
    if (v3.useFill) {
        std::wstring gaps;
        for (size_t i = 0; i < v3.fill.rows.size(); ++i) {
            if (v3.fill.rows[i].entryIndex < 0) {
                if (!gaps.empty()) gaps += L",";
                gaps += std::to_wstring(v3.fill.rows[i].slot[3]);
            }
        }
        Check("AC-08 gap positions", gaps == L"7,201", Narrow(gaps));
    }

    // AC-09 同一槽位命中两条（差异段 C：10.176.209.1 与 10.176.209.1/24 落在同一槽位）
    const core::Analysis a4 = core::Analyze(L"10.176.82.1\n10.176.209.1\n10.176.209.1/24", false);
    const core::ViewResult v4 = core::BuildView(a4, core::SortDir::Asc, true);
    int sameSlotRows = 0;
    bool firstShowsSlot = false;
    bool secondHidesSlot = false;
    bool bothHit = true;
    if (v4.useFill) {
        for (size_t i = 0; i < v4.fill.rows.size(); ++i) {
            const core::SlotRow& row = v4.fill.rows[i];
            if (row.slot[2] != 209) continue;
            ++sameSlotRows;
            if (row.entryIndex < 0) bothHit = false;
            if (sameSlotRows == 1) firstShowsSlot = row.showSlot;
            if (sameSlotRows == 2) secondHidesSlot = !row.showSlot;
        }
    }
    Check("AC-09 same slot expands to two rows",
          v4.useFill && sameSlotRows == 2 && firstShowsSlot && secondHidesSlot && bothHit &&
              v4.fill.gaps == 253 && v4.fill.total == 256,
          "rows=" + std::to_string(sameSlotRows) + " gaps=" + std::to_string(v4.fill.gaps));

    // AC-10 / AC-19 段值 0
    const std::wstring withZero = bSeg + L"\n10.0.5.5";
    const core::ViewResult v5 = core::BuildView(core::Analyze(withZero, false), core::SortDir::Asc, true);
    Check("AC-19 ascending 0 before 1",
          v5.useFill && v5.fill.rows.size() == 256 &&
              v5.fill.rows[0].slot[1] == 0 && v5.fill.rows[1].slot[1] == 1 && v5.fill.gaps == 251,
          "rows=" + std::to_string(v5.fill.rows.size()) + " first=" + std::to_string(v5.fill.rows[0].slot[1]));
    const core::ViewResult v6 = core::BuildView(core::Analyze(withZero, false), core::SortDir::Desc, true);
    Check("AC-19 descending 0 at end",
          v6.useFill && v6.fill.rows.size() == 256 &&
              v6.fill.rows[0].slot[1] == 255 && v6.fill.rows[255].slot[1] == 0,
          "rows=" + std::to_string(v6.fill.rows.size()) + " last=" + std::to_string(v6.fill.rows[255].slot[1]));

    // 有效地址仅 1 条：无差异段，按 D 段铺 255 槽位（§7.6）
    const core::ViewResult v7 = core::BuildView(core::Analyze(L"10.1.2.3", false), core::SortDir::Asc, true);
    Check("FR-7.6 single address fills by D segment",
          v7.useFill && v7.fill.total == 255 && v7.fill.rows.front().slot[3] == 1 &&
              v7.fill.rows.back().slot[3] == 255,
          "rows=" + std::to_string(v7.fill.total));

    // FR-4.7 上限保护：直接指定段位构造多分组场景
    // （界面只允许按差异段补位，差异段之前各段取值必然唯一，故分组数恒为 1，
    //   此用例用于验证超限分支本身；UI 路径下不可达，见 AC-12 备注）
    std::vector<core::Ipv4Entry> synthetic;
    for (int i = 1; i <= 33; ++i) {
        const core::ParseResult one = core::ParseAddress(std::to_wstring(i) + L".1.1.1");
        synthetic.push_back(one.entry);
    }
    const core::FillResult limited = core::BuildFilled(synthetic, core::SortDir::Asc, 1);
    Check("FR-4.7 limit guard (direct segment)", limited.tooLarge && limited.estimate == 33 * 255,
          "estimate=" + std::to_string(limited.estimate));
}

void TestDedupe() {
    const core::Analysis a1 = core::Analyze(L"10.0.0.1\n10.0.0.1\n10.0.0.1/24", true);
    Check("AC-13 duplicate addresses merged", a1.valid.size() == 1 &&
                                                  a1.valid[0].display == L"10.0.0.1/24",
          "count=" + std::to_string(a1.valid.size()));

    const core::Analysis a2 = core::Analyze(
        L"10.176.82.1\n10.176.55.1\n10.176.209.1\n10.176.209.1/24\n10.176.209.5/24", true);
    Check("AC-16 slot identity merge", a2.valid.size() == 3 && SegName(a2.diffSeg) == std::wstring(L"C"),
          "count=" + std::to_string(a2.valid.size()));
    Check("AC-16 keeps smallest prefix",
          JoinDisplays(a2.valid).find(L"10.176.209.1/24") != std::wstring::npos,
          Narrow(JoinDisplays(a2.valid)));

    const core::Analysis a3 = core::Analyze(L"192.168.1.8\n192.168.1.9\n192.168.1.8/24", true);
    Check("AC-17 D segment merge", a3.valid.size() == 2 && SegName(a3.diffSeg) == std::wstring(L"D"),
          Narrow(JoinDisplays(a3.valid)));

    const core::Analysis a4 = core::Analyze(L"10.0.0.1/24\n10.0.0.1", true);
    Check("FR-5 keep first on equal prefix",
          a4.valid.size() == 1 && a4.valid[0].display == L"10.0.0.1/24", Narrow(JoinDisplays(a4.valid)));
}

void TestCopyText() {
    // FR-6.1 复制内容格式：补位模式两列制表符分隔
    const core::ViewResult view = core::BuildView(
        core::Analyze(L"10.176.86.1\n10.176.127.1", false), core::SortDir::Asc, true);
    std::wstring text;
    if (view.useFill) {
        for (size_t i = 0; i < view.fill.rows.size(); ++i) {
            const core::SlotRow& row = view.fill.rows[i];
            if (!text.empty()) text += L"\n";
            for (int s = 0; s < core::kSegmentCount; ++s) {
                if (s) text += L".";
                text += std::to_wstring(row.slot[s]);
            }
            text += L"\t";
            text += (row.entryIndex >= 0) ? view.sorted[row.entryIndex].display : L"IP地址空缺";
        }
    }
    const size_t firstTab = text.find(L'\t');
    const size_t firstBreak = text.find(L'\n');
    Check("FR-6.1 two column tab separated",
          view.useFill && view.fill.total == 255 && firstTab != std::wstring::npos &&
              firstBreak != std::wstring::npos && firstTab < firstBreak &&
              text.compare(0, 10, L"10.176.1.0") == 0,
          "rows=" + std::to_string(view.fill.total) + " head=" + Narrow(text.substr(0, 12)));
}

}  // namespace

int main() {
    TestTokenizer();
    TestParser();
    TestSortAndSegment();
    TestFill();
    TestDedupe();
    TestCopyText();

    std::printf("\nTOTAL: PASS %d / FAIL %d\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
