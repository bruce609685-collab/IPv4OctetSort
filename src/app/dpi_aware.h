#pragma once

namespace app {

// §3.3 第 5 条：DPI 声明只允许使用 Vista 起可用的 SetProcessDPIAware()，
// 禁止使用 Windows 8 及以上才提供的 DPI API。
void EnableDpiAwareness();

}  // namespace app
