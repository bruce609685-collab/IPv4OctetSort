#include "app/dpi_aware.h"

#include <windows.h>

namespace app {

void EnableDpiAwareness() {
    // SetProcessDPIAware 自 Vista 起可用（Win7 上必然存在），
    // 直接调用即可满足 §3.3 第 5 条：不使用 Windows 8 及以上的 DPI API。
    SetProcessDPIAware();
}

}  // namespace app
