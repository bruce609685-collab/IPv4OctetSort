#pragma once

// 构建期内嵌的语言文件表（FR-12）：tools/lng2cpp.cpp 扫描 languages/*.lng
// 生成 languages.cpp 定义下列符号。语言包以外置文本维护，编译时打进 exe——
// 新增一种语言只需把一个 .lng 文件放进 languages/ 重新编译，无需改代码。
namespace ui {

struct EmbeddedLanguage {
    const char* fileStem;       // 文件名去扩展名（如 "en-US"），仅作调试线索
    const unsigned char* data;  // 文件原始字节（UTF-8 文本）
    unsigned int size;          // 字节数
};

extern const EmbeddedLanguage kEmbeddedLanguages[];
extern const unsigned int kEmbeddedLanguageCount;

}  // namespace ui
