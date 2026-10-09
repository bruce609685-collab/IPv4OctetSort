#pragma once

#include <string>

namespace util {

// NFR-4 日志：
//   位置 = 主程序 exe 同级目录的 IPv4OctetSort.log（UTF-8）；
//   内容仅记录启动、退出与错误信息，不含用户粘贴的地址；
//   超过 1 MB 时轮换为 IPv4OctetSort.log.old；
//   任何写入失败都不得影响程序运行（全部静默容错）。
void InitLogger(const std::wstring& executablePath);
void LogLine(const wchar_t* level, const std::wstring& message);
void LogLine(const wchar_t* level, const char* message);
void ShutdownLogger();

}  // namespace util
