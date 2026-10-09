#include "util/clipboard.h"

#include <cstring>

namespace util {

bool CopyTextToClipboard(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) return false;
    bool ok = false;

    if (EmptyClipboard()) {
        const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL block = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (block != nullptr) {
            void* target = GlobalLock(block);
            if (target != nullptr) {
                memcpy(target, text.c_str(), bytes);
                GlobalUnlock(block);
                if (SetClipboardData(CF_UNICODETEXT, block) != nullptr) {
                    ok = true;  // 所有权移交系统，不再 GlobalFree
                } else {
                    GlobalFree(block);
                }
            } else {
                GlobalFree(block);
            }
        }
    }

    CloseClipboard();
    return ok;
}

}  // namespace util
