#include "ui/menu_bar.h"

#include "ui/control_ids.h"
#include "ui/i18n.h"

// 菜单自 v0.3 起由代码构建（文字随语言整菜单重建），但一律使用系统标准绘制：
// 外观与 v0.2 的资源菜单完全一致（主题选中配色、顶层无箭头、助记符由系统匹配）。
// 因此本文件不再持有标签数据，AppendMenuW 会复制文本。
namespace ui {
namespace {

constexpr int kAccelResourceId = 102;  // IDR_ACCEL（app.rc）

HMENU Popup(HMENU parent, const wchar_t* text) {
    HMENU sub = CreatePopupMenu();
    AppendMenuW(parent, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(sub), text);
    return sub;
}

void Item(HMENU menu, UINT id, const wchar_t* text) {
    AppendMenuW(menu, MF_STRING, id, text);
}

// FR-9 菜单结构：文件 / 编辑 / 视图 + 顶层「关于」
HMENU BuildMainMenu() {
    HMENU root = CreateMenu();

    HMENU file = Popup(root, T(StrId::MenuFile));
    Item(file, kCmdClear, T(StrId::MenuClear));
    Item(file, kCmdPasteHint, T(StrId::MenuPaste));
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    Item(file, kCmdExit, T(StrId::MenuExit));

    HMENU edit = Popup(root, T(StrId::MenuEdit));
    Item(edit, kCmdCopyResult, T(StrId::MenuCopy));
    Item(edit, kCmdCopyGaps, T(StrId::MenuCopyGaps));

    HMENU view = Popup(root, T(StrId::MenuView));
    Item(view, kCmdAsc, T(StrId::MenuAsc));
    Item(view, kCmdDesc, T(StrId::MenuDesc));

    Item(root, kCmdAbout, T(StrId::MenuAbout));  // 顶层项，无下拉
    return root;
}

// 挂新菜单、销毁旧菜单
void Attach(HWND window, HMENU menu) {
    if (menu == nullptr) return;
    HMENU oldMenu = GetMenu(window);
    SetMenu(window, menu);
    if (oldMenu != nullptr) DestroyMenu(oldMenu);
}

void EnableItem(HWND window, int command, bool enabled) {
    HMENU menu = GetMenu(window);
    if (menu == nullptr) return;
    EnableMenuItem(menu, static_cast<UINT>(command), enabled ? MF_ENABLED : MF_GRAYED);
}

}  // namespace

void MenuBarAttach(HWND window, HINSTANCE instance) {
    Attach(window, BuildMainMenu());
    (void)instance;  // 加速键表在 MenuBarTranslate 中按需加载
}

void MenuBarRebuild(HWND window, HINSTANCE instance) {
    Attach(window, BuildMainMenu());
    DrawMenuBar(window);
    (void)instance;
}

bool MenuBarTranslate(HWND window, MSG* message) {
    if (message == nullptr) return false;
    static HACCEL accel =
        LoadAcceleratorsW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(kAccelResourceId));
    if (accel == nullptr) return false;
    return TranslateAcceleratorW(window, accel, message) != 0;
}

void MenuBarEnableItems(HWND window, bool hasValid, bool sorted, bool fillChecked) {
    EnableItem(window, kCmdAsc, hasValid);
    EnableItem(window, kCmdDesc, hasValid);
    EnableItem(window, kCmdCopyResult, sorted);
    // FR-9：「仅复制空缺地址」在未勾选缺位填充时仍可点击，由状态栏给出提示
    EnableItem(window, kCmdCopyGaps, sorted);
    (void)fillChecked;

    HMENU menu = GetMenu(window);
    if (menu != nullptr) DrawMenuBar(window);
}

HMENU MenuBarBuildLangMenu(Lang current) {
    HMENU menu = CreatePopupMenu();
    // 语言名称永远按其自身语言显示，不随界面语言变化
    Item(menu, kCmdLangZh, LangName(Lang::ZhCn));
    Item(menu, kCmdLangEn, LangName(Lang::En));
    CheckMenuRadioItem(menu, kCmdLangZh, kCmdLangEn,
                       current == Lang::En ? kCmdLangEn : kCmdLangZh, MF_BYCOMMAND);
    return menu;
}

void MenuBarFreeLangMenu(HMENU menu) {
    if (menu != nullptr) DestroyMenu(menu);
}

}  // namespace ui
