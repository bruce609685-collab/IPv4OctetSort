# IPv4 网络地址排序器（IPv4OctetSort）

一个 Windows 小工具：给一批 IPv4 地址正确排序，并把网段里**没被占用的地址**标出来。

## 它解决什么问题

用 Excel 或记事本排序时，`192.168.1.153` 会排在 `192.168.1.8` 前面（逐字符比较时 `1` < `8`），
和真实地址顺序对不上。本工具按四段数值比较，顺序符合直觉。

## 功能

- **按数值排序**：升序 / 降序，四段逐段比较，正确排出 `8 → 17 → 55 → 153`
- **自动识别排序段位**：A / B / C / D 哪一段不同就按哪一段排，不用手工选
- **缺位填充**：把完整地址序列铺开（1–255），空缺位置显示「IP地址空缺」，一眼看出网段里哪些地址没被占用
- **合并重复**：同一网段的重复地址只保留一条
- **一键复制**：复制结果可直接粘进 Excel；补位模式下是「完整序列 + 你的清单」两列对照
- **无效输入提示**：格式错误、段值超范围、掩码超范围分别给出原因，不影响其它地址的处理

## 使用

1. 下载 `IPv4OctetSort.exe`
2. 双击运行（免安装）
3. 把地址粘进输入框（一行一个，逗号或空格分隔也可以），点「升序排列」或「降序排列」

## 系统要求

| 项目 | 说明 |
|---|---|
| 系统 | Windows 7 SP1 及以上（含 Windows 10 / 11），32 位与 64 位均可 |
| 安装 | 免安装，单个 exe，双击即用 |
| 依赖 | 无。不需要 .NET、VC++ 运行库等任何第三方组件 |
| 网络 | 程序不联网（「检查更新」只是用系统浏览器打开本仓库页面） |
| 日志 | 会在 exe 同级目录写 `IPv4OctetSort.log`，只记录启动、退出与错误，不记录你粘贴的地址 |

## 从源码编译

需要 MSYS2 的 MinGW-w64（GCC，支持 C++17）与 windres。在项目根目录执行：

```
build\build.bat
```

产物输出到 `dist\IPv4OctetSort.exe`。跑核心逻辑单元测试（命令行，不依赖界面）：

```
build\build_tests.bat
```

习惯 CMake 的话见 `build\CMakeLists.txt` 头部的说明；工程目录含中文时请用 `build\build_cmake.bat`。

## 项目结构

```
IPv4OctetSort/
├── README.md                      软件说明（本文件）
├── LICENSE                        GPL-3.0 许可证全文
├── .gitignore                     编译产物与日志的忽略规则
│
├── src/
│   ├── main.cpp                   程序入口：DPI 声明 → 单实例检测 → 创建主窗口 → 消息循环 → 退出清理
│   │
│   ├── app/                       【启动层】程序级初始化
│   │   ├── app_info.h             产品信息常量（名称 / 版本 / 发布页 / 构建日期宏）
│   │   ├── single_instance.h      单实例检测接口
│   │   ├── single_instance.cpp    内核互斥体检测；已存在时激活并前置已有窗口
│   │   ├── dpi_aware.h            DPI 感知声明接口
│   │   └── dpi_aware.cpp          调用 SetProcessDPIAware（Vista 起可用，不使用 Win8+ DPI API）
│   │
│   ├── core/                      【核心逻辑层】纯 C++，不含任何 Windows 头文件，可独立编译测试
│   │   ├── types.h                数据结构：地址条目、无效项、词元、分析结果、槽位行、补位结果
│   │   ├── tokenizer.h            分词接口
│   │   ├── tokenizer.cpp          按行拆分、剥离 # 与 // 注释、按分隔符集拆词、记录行号
│   │   ├── parser.h               单个地址解析接口
│   │   ├── parser.cpp             合法性校验、失败原因文案、前导零归一化、显示串生成
│   │   ├── comparator.h           地址比较接口
│   │   ├── comparator.cpp         逐段比较；指定段位优先；四段相同则掩码小者（网段更宽）在前
│   │   ├── sorter.h               排序接口
│   │   ├── sorter.cpp             升序 / 降序，稳定排序（std::stable_sort）
│   │   ├── segment.h              差异段相关接口
│   │   ├── segment.cpp            差异段判定、某段不同取值的预览字符串
│   │   ├── dedupe.h               合并重复接口
│   │   ├── dedupe.cpp             槽位身份分组、掩码最小择优、按各组首次出现保序
│   │   ├── slot_filler.h          缺位填充接口
│   │   ├── slot_filler.cpp        分组、铺槽位、命中匹配、同槽位多行展开、段值 0、超限判定
│   │   ├── pipeline.h             核心流水线接口
│   │   └── pipeline.cpp           对外唯一入口：分词 → 解析 →（去重）→ 判段；排序 +（补位）视图
│   │
│   ├── ui/                        【界面层】Win32 控件
│   │   ├── control_ids.h          控件 ID、菜单命令 ID、对话框控件 ID、定时器 ID 集中定义
│   │   ├── theme_colors.h         最小自绘所需的系统配色（高亮 / 信息栏 / 斑马纹 / 无效项）
│   │   ├── ui_state.h             界面共享状态与访问器
│   │   ├── layout.h               布局与字体接口
│   │   ├── layout.cpp             固定 780 px 布局计算、界面字体与等宽字体创建
│   │   ├── main_window.h          主窗口接口
│   │   ├── main_window.cpp        窗口类注册、消息分发、控件定位、命令处理、自绘与定时器
│   │   ├── toolbar.h              工具栏接口
│   │   ├── toolbar.cpp            工具栏控件创建；排序按钮高亮、段位单选可用性同步
│   │   ├── input_panel.h          输入面板接口
│   │   ├── input_panel.cpp        输入框、清空按钮、统计、粘贴提示；无效项标签测量与绘制
│   │   ├── result_panel.h         结果面板接口
│   │   ├── result_panel.cpp       复制按钮、已复制提示、统计文字、列表列设置
│   │   ├── result_render.h        结果渲染接口
│   │   ├── result_render.cpp      列表重建（单列 / 双列）、斑马纹与空缺着色、复制文本生成
│   │   ├── status_bar.h           状态栏接口
│   │   ├── status_bar.cpp         状态栏三分区文本更新
│   │   ├── menu_bar.h             菜单接口
│   │   ├── menu_bar.cpp           菜单挂载、加速键翻译、菜单项启用状态
│   │   ├── about_dialog.h         关于对话框接口
│   │   └── about_dialog.cpp       模态关于对话框（程序名称 / 英文名称 / 版本 / 构建日期）
│   │
│   ├── util/                      【工具层】通用能力
│   │   ├── text_convert.h         UTF-8 ↔ UTF-16 转换接口
│   │   ├── text_convert.cpp       转换实现
│   │   ├── clipboard.h            剪贴板接口
│   │   ├── clipboard.cpp          以 Unicode 文本写入剪贴板
│   │   ├── logger.h               日志接口
│   │   ├── logger.cpp             日志写入、1 MB 轮换、写入失败静默容错
│   │   ├── shell_open.h           打开链接接口
│   │   └── shell_open.cpp         调用系统默认浏览器打开 URL
│   │
│   └── resource/                  【资源】
│       ├── app.rc                 资源脚本：清单引用、菜单、关于对话框、版本信息
│       └── app.manifest           comctl32 v6 依赖、DPI 感知、支持的系统声明
│
├── tests/
│   └── test_core.cpp              核心逻辑单元测试（49 项断言，命令行运行、不依赖界面）
│
└── build/
    ├── build.bat                  一键编译：MinGW-w64，产出 32 位主产物 + 64 位副产物
    ├── build_tests.bat            编译并运行核心逻辑单元测试
    ├── build_cmake.bat            CMake / Ninja 一键编译（中文路径下经英文 junction 编译）
    └── CMakeLists.txt             CMake 配置：源文件清单、编译与链接选项、测试目标
```

**分层约定**

- `core` 只依赖 C++ 标准库，**不包含任何 Windows 头文件**，因此可以脱离界面单独编译与测试（`tests/test_core.cpp` 就是这么跑的）
- `ui` 内部通过 `ui_state.h` 共享状态；除 `main_window` 作为协调者外，各面板模块之间不直接互相调用
- `util` 可调用 Windows API，但不依赖 `ui` 与 `core`
- 单个源文件建议控制在 250 行以内，文件即职责边界

## 下载

请到 [Releases](https://github.com/bruce609685-collab/IPv4OctetSort/releases) 页面下载最新版本。

当前版本：**v0.2**

## 许可证

本项目采用 **GPL-3.0** 许可证，详见 [LICENSE](LICENSE)。

Copyright (C) 2026 bruce609685-collab
