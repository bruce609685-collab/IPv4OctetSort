#pragma once

#include <string>
#include <vector>

// 外置语言文件（*.lng）解析器（FR-12）。纯 C++ 标准库实现，
// 本层禁止包含任何 Windows 头文件（文档 §8.3 硬性约束 1），
// 因此语言包的正确性可以放进 tests/test_core.cpp 直接验证。
//
// 文件格式（UTF-8，有无 BOM 均可）：
//   · # 或 ; 开头为注释；空行忽略
//   · 每行一条  键 = 值 ；键为字母/数字/下划线（须字母开头），值在首个 = 之后
//   · 值首尾空白会被去除（分隔符写在代码里，见 ui/i18n 的组句函数）
//   · 值内转义：\t \n \r \\ \xNN（两位十六进制，如 \x20 表示空格）；
//     其余反斜杠序列原样保留两个字符
//   · locale 与 name 是两条普通键，由调用方单独取出（自描述元数据）
namespace core {

struct LngEntry {
    std::wstring key;
    std::wstring value;
};

struct LngFile {
    bool ok = false;          // 解析成功（无致命错误）
    std::wstring error;       // 第一条错误的说明；ok 时为空
    int errorLine = 0;        // 出错行号（1 起）；ok 时为 0
    std::vector<LngEntry> entries;  // 按出现顺序；重名键以第一条为准
};

// 解析一段 UTF-8 文本（整个 .lng 文件内容）。
LngFile ParseLng(const std::string& utf8Text);

// 读取本地文件并解析；读取失败返回 ok=false 且 error 说明原因。
LngFile LoadLngFile(const std::wstring& path);

}  // namespace core
