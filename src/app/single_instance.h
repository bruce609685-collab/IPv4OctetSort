#pragma once

namespace app {

// NFR-3 单实例运行：
//   首次启动返回 true；若已有实例在运行，则把已有窗口带到前台并聚焦，返回 false，
//   调用方应直接退出。基于内核互斥体，进程崩溃后不会残留死锁。
bool ClaimSingleInstance(const wchar_t* windowClassName);

}  // namespace app
