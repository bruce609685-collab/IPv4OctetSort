#include "util/logger.h"

#include <windows.h>

#include "util/text_convert.h"

namespace util {
namespace {

const unsigned long long kMaxLogBytes = 1024ull * 1024ull;  // NFR-4：1 MB
std::wstring g_logPath;
bool g_ready = false;

// 取当前本地时间的 "YYYY-MM-DD HH:MM:SS"
std::wstring NowStamp() {
    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t buffer[32] = {0};
    wsprintfW(buffer, L"%04u-%02u-%02u %02u:%02u:%02u", time.wYear, time.wMonth, time.wDay,
              time.wHour, time.wMinute, time.wSecond);
    return std::wstring(buffer);
}

// 超限时把当前日志改名为 .old（旧文件覆盖），新日志重新开始
void RotateIfTooLarge() {
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(g_logPath.c_str(), GetFileExInfoStandard, &data)) return;

    const unsigned long long size =
        (static_cast<unsigned long long>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    if (size < kMaxLogBytes) return;

    const std::wstring oldPath = g_logPath + L".old";
    MoveFileExW(g_logPath.c_str(), oldPath.c_str(), MOVEFILE_REPLACE_EXISTING);
}

void AppendLine(const std::wstring& line) {
    if (!g_ready) return;

    const std::string utf8 = Utf8FromWide(line + L"\r\n");
    if (utf8.empty()) return;

    HANDLE file = CreateFileW(g_logPath.c_str(), FILE_APPEND_DATA,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        g_ready = false;  // 写入失败则整体静默停用，不影响程序运行
        return;
    }

    DWORD written = 0;
    WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    CloseHandle(file);
}

std::wstring ExeDirectory(const std::wstring& executablePath) {
    const size_t slash = executablePath.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return std::wstring(L".");
    return executablePath.substr(0, slash);
}

}  // namespace

void InitLogger(const std::wstring& executablePath) {
    g_logPath = ExeDirectory(executablePath) + L"\\IPv4OctetSort.log";
    g_ready = true;

    RotateIfTooLarge();

    // 新文件写入 UTF-8 BOM，便于记事本正确识别编码
    const bool fresh = GetFileAttributesW(g_logPath.c_str()) == INVALID_FILE_ATTRIBUTES;
    if (fresh) {
        HANDLE file = CreateFileW(g_logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                                  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
            DWORD written = 0;
            WriteFile(file, bom, sizeof(bom), &written, nullptr);
            CloseHandle(file);
        }
    }
}

void LogLine(const wchar_t* level, const std::wstring& message) {
    if (!g_ready) return;
    RotateIfTooLarge();
    AppendLine(NowStamp() + L" [" + level + L"] " + message);
}

void LogLine(const wchar_t* level, const char* message) {
    if (!g_ready) return;
    LogLine(level, WideFromUtf8(message));
}

void ShutdownLogger() {
    g_ready = false;
    g_logPath.clear();
}

}  // namespace util
