# 汇文MCP

鸿蒙 PC 上的 PDF 与 Markdown 阅读、转换应用。本阶段先把文字型 PDF 转成沙箱内的 MD 文件。

## 许可证

本项目自有代码采用 [PolyForm Noncommercial License 1.0.0](LICENSE)，官方原文见 [PolyForm 网站](https://polyformproject.org/licenses/noncommercial/1.0.0)。这是源码公开的非商业使用许可。

允许在许可规定的用途范围内使用、学习、研究、修改和分发。商业产品开发、销售、商业服务等超出许可范围的用途，需要另行取得相关权利人的授权。许可对教育机构等组织另有允许用途规定，以 `LICENSE` 原文为准。

第三方源码、依赖和资源仍遵循各自的许可证及版权声明，不受本项目许可证替代。贡献代码前，请确认你有权将该贡献按本项目许可证授权。

## 当前状态

- 首页保持 ArkTS；蓝色介绍区与主区域按约 30% / 70% 分配宽度，主区域提供文档预览、文档编辑、文档转化三个小正方形入口，普通字重的名称放在方块外下方。点击入口打开独立格式选择子窗口；预览和编辑仅展示 Word、PDF、MD 格式，转化窗口中的 PDF→MD 入口启动独立 Qt 工作区。
- ArkTS 首页左侧有 60vp 半透明窄侧栏，底部保留 36vp 三横线 SVG 菜单按钮，目前尚未接入菜单内容。首页内容背景与窗口容器分别设置透明，焦点态容器透明，非焦点态使用实色，右侧主内容保持不透明。
- Qt 开发依赖保存在 `third_party/qt/`，运行库从 `entry/libs/arm64-v8a/` 链接并随 HAP 分发；PDFium fork 保存在 `third_party/pdfium/`，Qt 左侧输入区域已接入其公开 C API 实现正文预览；已完成鸿蒙编译、HAP 打包；系统目录跳转沿用原有实现。
- DevEco Run 构建前执行 `PrepareNativeDependencies`，按当前 PDFium 源码增量生成 `libpdfium.so`；Qt/NAPI 的 C++ 编译后同步新库到 `libs`。没有保存源码自动构建、应用内动态替换库或保持进程状态的热重载。
- Qt 工作区默认最大化并保留系统标题栏和窗口边框，输入、输出区域解耦；中部分割线可拖动，悬浮“转化”按钮随分隔线移动。
- 输入文件卡片通过 DocumentStore 获取系统选取授权，再经 NAPI 调用 C++ harmony_sandbox 完成沙箱复制和路径安排。导入成功后上方显示“预览”和“更换文件”，点击“预览”才切换为多页正文；支持滚动，随窗口/分栏宽度等比排版并重新渲染，可返回文件卡片。PDFium 后台串行渲染可见页，连续尺寸变化合并请求；位图缓存上限为 64 MiB，超长页面降低采样分辨率。Qt ConversionService 仍复用 PythonRunner 转换；右侧成功后先显示 MD 文件卡片，点击“预览”后由 CEF 显示正文，可返回文件或打开所在目录。MD 接入尚未编译、打包或真机验收。
- 原有 Python HNP 自检与 PDF 转换已由用户在真机验证成功。新增 Qt 窗口交接、选择与结果位置操作需要真机验收。
- 汇文 Python 包只公开 `pdf_to_md()`；后续格式在依赖和设备转换通过后再增加。

## 工程结构

以下以当前仓库根目录为基准。`已有`表示工作区中存在，`生成`表示本机打包产物，`规划`表示后续阶段的目标目录或文件；规划项尚未创建。

```text
HuiWenMCP/
├── AppScope/                          [已有] 应用名称、图标和版本
├── entry/                             [已有] 鸿蒙 HAP 模块
│   ├── src/main/
│   │   ├── module.json5               [已有] 设备类型、Ability 与私有 HNP 声明
│   │   ├── ets/
│   │   │   ├── abilitystage/QtAbilityStage.ets [已有] Qt 平台初始化
│   │   │   ├── entryability/EntryAbility.ets  [已有] ArkTS 启动入口
│   │   │   ├── entryability/QtWorkspaceAbility.ets [已有] Qt 窗口及系统文件操作
│   │   │   ├── components/FunctionSidebar.ets [已有] 首页半透明窄栏与底部菜单按钮
│   │   │   ├── components/HomeFeatureCard.ets [已有] 正方形功能卡片与 SVG 图标
│   │   │   ├── components/HomeFormatDialog.ets [已有] 独立格式选择子窗口与 PDF→MD 跳转事件
│   │   │   ├── pages/HomePage.ets      [已有] 首页功能切换与 Qt 窗口交接
│   │   │   ├── pages/Index.ets         [已有] QPA 固定加载的 Qt XComponent 承载页
│   │   │   ├── services/ConversionService.ets [已有] 页面调用原生转换的统一入口
│   │   │   └── utils/DocumentStore.ets [已有] 系统选取授权与 NAPI 适配
│   │   └── cpp/
│   │       ├── CMakeLists.txt          [已有] 原生库构建配置
│   │       ├── napi_bridge.cpp        [已有] ArkTS → C++ 的 NAPI 接口
│   │       ├── python_runner.cpp/.h   [已有] 定位 HNP、启动固定自检或转换脚本
│   │       └── types/                 [已有] 原生模块的 ArkTS 类型声明
│   ├── libs/arm64-v8a/                [生成] Qt、PDFium 与本项目动态库，不进入 Git
│   │   └── libQt5{Core,Gui,Widgets,Concurrent}.so、QPA、libpdfium.so、libhuiwen_{native,qt}.so
│   ├── build-profile.json5            [已有] HAP 构建与 ABI 配置
│   ├── hvigorfile.ts                   [已有] 每次运行构建生成 Python HNP，并在 HAP 签名前注入
│   └── oh-package.json5               [已有] 模块依赖
├── converter/                         [已有] 可供桌面端复用的 Python 转换逻辑
│   ├── huiwen_converter/              [已有] 汇文公开 Python API
│   │   ├── __init__.py                 [已有] 仅导出已实现的 pdf_to_md()
│   │   ├── api.py                      [已有] 路径校验、写入 MD、返回输出路径
│   │   └── _markitdown_adapter.py      [已有] 唯一直接调用 MarkItDown 的适配文件
│   └── pdf2md/convert.py              [已有] HNP 固定的 PDF 命令入口
├── qt_app/                            [已有] Qt Widgets 工作区，鸿蒙共享库/桌面可执行入口
│   ├── CMakeLists.txt                 构建 huiwen_qt 并链接 PDFium；新增预览尚未编译验证
│   ├── main.cpp                       默认最大化并保留系统边框，显示成功后通知结束原首页
│   ├── tests/                         独立桌面预览回归检查，需宿主 Qt/PDFium
│   └── src/
│       ├── workspace/ConversionWindow.h/.cpp     主窗口与组件接线
│       ├── sourceView/
│       │   ├── SourceDocumentView.h/.cpp 输入卡片、预览切换与文件选择入口
│       │   └── PdfPreviewWidget.h/.cpp 后台 PDFium 渲染、分页滚动与尺寸自适应
│       ├── resultView/MarkdownResultView.h/.cpp 输出区域入口
│       ├── components/
│       │   ├── FileCard.h/.cpp         PDF/MD 共用占位卡片
│       │   └── ConvertButton.h/.cpp    独立悬浮转化按钮
│       ├── services/ConversionService.h/.cpp    异步 PDF 转 MD API
│       ├── sandbox/
│       │   ├── DocumentItem.h         文件格式、名称和输入/输出路径
│       │   └── SandboxService.h/.cpp  请求系统选择、接收沙箱路径和打开输出位置
│       └── platform/harmony/
│           ├── qt_bridge.h/.cpp              Qt ↔ ArkTS 线程桥
│           ├── harmony_sandbox.h/.cpp        鸿蒙沙箱规则与文件操作
│           └── harmony_sandbox_napi.cpp      异步导入与输出校验 NAPI
├── hnp/arm64-v8a/                     [生成] 鸿蒙 Python 运行环境，不进入 Git
│   └── huiwen_python.hnp               私有 HNP 归档，内部包含：
│       ├── bin/python3                 Python 解释器
│       ├── lib/python3.14/site-packages/
│       │   ├── huiwen_converter/       汇文对外 Python API
│       │   ├── markitdown/             第三方运行源码，不含其 Git 仓库
│       │   └── PDF 依赖及原生 .so       pdfplumber、Pillow、pdfium 等
│       └── share/huiwen/convert_pdf.py NAPI 启动的固定命令入口
├── packaging/                         [部分已有] 各平台打包脚本和配置
│   ├── ohos/                           [已有] HNP 打包脚本、依赖锁定文件及上游补丁
│   ├── windows/
│   └── macos/
├── tests/pdf_samples/                 [规划] 文字、双栏、表格等转换样本
├── third_party/
│   ├── markitdown/                   [子模块] MarkItDown fork
│   ├── pdfium/                       [子模块] PDFium fork 与鸿蒙构建补丁
│   └── qt/                           [本机依赖] Qt 头文件、CMake 配置及宿主工具
├── entry/src/main/resources/rawfile/third_party_licenses/ [生成] Qt/PDFium 许可材料，随包分发
├── CMakeLists.txt                     [规划] Qt 桌面工程入口
├── build-profile.example.json5       [已有] 不含签名信息的全局构建配置模板
├── build-profile.json5               [本机] 签名配置，已被 Git 忽略
└── README.md                          [已有] 项目状态与维护说明
```

当前 Qt 转换链为 `ConvertButton → Qt ConversionService.convertPdfToMd() → PythonRunner → convert_pdf.py → huiwen_converter.pdf_to_md() → MarkItDown → outputs/md/*.md`。Python API 与第三方库适配保持原有边界。原 ArkTS 转换接口仍保留；首页通过“文档转化 → PDF→MD”进入 Qt，不再选文件或直接执行转换。

鸿蒙跳转由 `HomePage.ets → startAbility(QtWorkspaceAbility) → QPA.startQtApplication() → libhuiwen_qt.so/main()` 完成。Qt 窗口实际显示后通过 NAPI 线程安全回调通知新 Ability；新 Ability 通过应用 EventHub 通知首页，只有发起跳转的首页消费一次通知并调用 `terminateSelf()`。这只结束首页 Ability，不结束应用进程。

当前 QPA 固定加载 `pages/Index`，不能让该路由同时作为 ArkTS 首页。`EntryAbility` 加载独立的 `pages/HomePage`；`Index.ets` 仅创建 Qt 的 NODE 类型 XComponent。`QtWorkspaceAbility.newLocalStorage()` 提供存储，QPA 写入 `idName` 并传给承载页。若在承载页放首页事件监听，会在 Qt 就绪时把 Qt Ability 也关闭；缺少存储方法则会产生 `newLocalStorage not found`、`Need object` 等日志。

文件选择链为 `SourceDocumentView → SandboxService → QtBridge → QtWorkspaceAbility → DocumentStore`。Qt 请求进入 ArkTS 线程处理；结果经 NAPI 投递到 Qt 线程，不跨线程直接操作界面。新 Ability 使用自己的 Context，首页关闭后文件选择仍可继续。DocumentStore 只保留 Picker、URI 授权打开及关闭；日期、格式校验、命名、重名编号、目录创建、复制和失败清理由 C++ `HarmonySandbox` 负责。

仓库中的 `third_party/markitdown` 是固定版本的上游源码，**不会把整个 Git 仓库装进应用**。`pack_python_hnp.sh` 仅把运行所需的 `markitdown/`、`huiwen_converter/`、Python 依赖及其原生 `.so` 放入 HNP 的 `lib/python3.14/site-packages/`，固定命令脚本放在 `share/huiwen/`；HNP 同时包含 Python 解释器。构建 HAP 时，`libhuiwen_native.so`、`libhuiwen_qt.so` 和最小 Qt 运行库进入 HAP，生成的 `huiwen_python.hnp` 也随 HAP 分发。ArkTS 不能直接导入 HNP 内的 Python 函数，必须经应用自己的 NAPI 桥接调用。

### 接口边界

| 位置 | 项目接口 | 调用者与职责 |
| --- | --- | --- |
| `converter/huiwen_converter/__init__.py` | `pdf_to_md(input_path, output_path) -> Path` | Python 公开业务接口；生成非空 MD 后返回路径，失败抛出异常。后续格式实现并验证后，才新增对应的显式接口。 |
| `converter/huiwen_converter/_markitdown_adapter.py` | 内部 `pdf_to_markdown()` | 唯一直接导入 MarkItDown 的文件；不作为业务 API。 |
| `entry/src/main/ets/services/ConversionService.ets` | `pdfToMd(inputPath, outputPath): Promise<string>` | ArkTS 页面调用；只转发到 NAPI，成功返回 MD 路径。 |
| `entry/src/main/cpp/napi_bridge.cpp` | `convertPdf(inputPath, outputPath)` | 原生边界；异步启动 HNP 中的固定命令，不接受任意 Shell 命令。 |

## 沙箱平台分层

- `SandboxService` 是 Qt 界面的统一入口；`Q_OS_OPENHARMONY` 分支仅引用鸿蒙桥接，桌面头文件及原有逻辑位于独立的 `#else` 分支。
- `platform/harmony/harmony_sandbox.h/.cpp` 和 `harmony_sandbox_napi.cpp` 以 `harmony_` 命名，只在 `CMAKE_SYSTEM_NAME=OHOS` 时加入原生库；源码由 `Q_OS_OPENHARMONY` 宏保护，不向 Windows/macOS 引入鸿蒙 SDK 类型。后续平台实现应放入各自目录，并通过平台条件选择，避免把规则回填到 ArkTS 或混入通用界面。
- `harmonyImportDocument(filesDir, name, fd, kind, forConversion)` 返回 `Promise<HarmonySandboxDocument>`；NAPI 同步复制授权 fd，并设置关闭时不继承到子进程，在工作线程使用独立偏移读取、排他创建目标文件、分块复制。调用方的 fd 生命周期及读偏移不受复制影响；成功、失败或排队失败均释放原生 fd，复制失败仅移除本次创建的输入文件。
- `harmonyValidateOutput(filesDir, path)` 同步校验输出目录归属、规范路径、符号链接实际位置及非空可读 MD。ArkTS 拿到校验后的路径，再使用既有 `fileUri` 和 `openLink` 打开系统文件管理器；旧 `outputs/md/` 与日期目录都兼容。
- 日期目录与重名分配使用同一份 C++ 规则；并发导入通过路径分配锁与排他文件创建避免覆盖，PDF/MD 导入共用同一接口。首页预览和编辑格式选项暂不发起导入；系统选取取消时不创建沙箱文件。

## 沙箱文件路径

当前设备用户号为 `100` 时，应用私有沙箱的预期物理路径如下。程序通过 `getApplicationContext().filesDir` 获取运行时目录，不写死用户号和包路径；设备上的实际映射需在验收时核对。

```text
/data/app/el2/100/base/cn.com.HuiWenMCP/
└── files/
    ├── inputs/
    │   └── YYYY-MM-DD/          当天选入的 PDF、MD 副本，保留原名
    └── outputs/
        └── YYYY-MM-DD/          当天导入的 PDF 对应的 Markdown
```

| 入口 | 沙箱路径 | 本阶段行为 |
| --- | --- | --- |
| 文档预览 | 暂不导入 | 仅展示 Word、PDF、MD 格式图标，不跳转 |
| 文档编辑 | 暂不导入 | 仅展示 Word、PDF、MD 格式图标，不跳转 |
| 文档转化 → PDF→MD | 输入：`files/inputs/YYYY-MM-DD/<原文件名>.pdf`；输出：`files/outputs/YYYY-MM-DD/<原文件名>.md` | 首页打开 Qt；Qt 输入区域选取文件，点击转化后显示 MD 卡片 |

系统选择器返回的 URI 只用于读取选中文件；应用内部后续使用沙箱副本路径。转换成功后才生成非空 `.md` 文件并显示路径；文字无法提取的扫描件会提示需要 OCR，不生成空结果。首页只进行功能与格式选择，文件导入和正文预览均由 Qt 工作区处理，PDF 正文由左侧的“预览”按钮进入。点击 MD 卡片沿用原有系统文件管理器跳转；鸿蒙通过 ArkTS 的 `openLink('filemanager://openDirectory')` 传入结果文件 URI。底部继续显示生成的完整 MD 路径，不增加应用内目录窗口，不导出或移动文件。内部路径由应用级 `Context.filesDir` 获取，与模块级 `haps/entry/files` 不同。转换成功前与点击结果时都会校验文件；不存在、为空或不可读取时显示错误。

## 应用图标

统一使用 `.native-tools/design/huiwen-icon-rounded-final.png` 的最终版图标，原图为 1024×1024 PNG，保留透明圆角。正式资源直接随仓库保存，构建使用以下文件：

- `AppScope/resources/base/media/app_icon.png`：应用级图标，由 `AppScope/app.json5` 引用。
- `entry/src/main/resources/base/media/app_icon.png`：ArkTS 首页与 Qt 工作区 Ability 图标，以及各自的启动页图标。
- `qt_app/resources/branding/app_icon.png`：Qt 窗口图标，通过现有 QRC 注册为 `:/branding/app_icon.png`，在 `main.cpp` 中统一设置。

三份资源均直接复制最终版 PNG；更新品牌图标时同步替换这三份文件。

## 本地构建

工程当前配置为 HarmonyOS PC、`arm64-v8a`，目标 SDK 为 `6.1.1(24)`。首次克隆后，将 `build-profile.example.json5` 复制为本机的 `build-profile.json5`，需要安装到设备时再在 DevEco Studio 配置签名。

macOS ARM64 开发机首次 Run 会按 `third_party/prebuilt/manifest.json` 下载、校验并恢复固定的
Qt/CEF `.so`、头文件、CMake 配置、宿主工具和浏览器核心资源；后续 Run 校验本地文件后复用。
下载地址对应的依赖包须先发布，离线机器可通过 `HUIWEN_PREBUILT_ARCHIVE` 指定同一压缩包。
包哈希或本地依赖不匹配会停止构建，不覆盖已修改的库。详见 [固定依赖包](third_party/prebuilt/README.md)。

维护者准备或升级 Qt SDK 时，仍可导入同一套鸿蒙 Qt 5.15.12 开发文件和五个运行库：

```sh
python3 packaging/ohos/prepare_qt.py \
  --source /绝对路径/Qt5.15.12-ohos17-arm64-v8a \
  --license-source /对应的/qtbase源码目录
```

`--source` 也可由原有 `qt.sdk.dir` 或 `QT_PREFIX` 指定，但它们只用于准备/升级依赖，正常 CMake 构建使用项目内 `third_party/qt/`。库的唯一部署位置是 `entry/libs/arm64-v8a/`，Qt 开发包内通过相对符号链接引用；宿主工具必须匹配构建电脑。修改 Qt 内核后需先编译新的 SDK，再执行准备命令同步整套依赖。

PDFium fork 为 `xristosstarnetgames439-del/pdfium` 的 `huiwen-dev` 分支，首次同步提交记录官方源码 SHA；主项目固定子模块提交。首次构建会准备 DEPS 锁定的 Git 依赖、GN 和 Clang，之后由 Ninja 增量编译当前工作树，不重置用户的源码修改：

```sh
git submodule update --init third_party/pdfium
python3 packaging/ohos/build_pdfium.py --sync
```

脚本使用鸿蒙 NDK sysroot、归档器和链接驱动，生成 `aarch64-linux-ohos` 动态库；内部 libc++ 静态链接，关闭 V8、XFA、Skia 和 PartitionAlloc，保留公开渲染、编辑及保存接口。通过 ELF 架构、必要导出和动态依赖检查后，原子更新 `entry/libs/arm64-v8a/libpdfium.so`，同时生成源码/构建参数/校验值记录及分发许可材料。构建失败不会覆盖有效旧库，但会阻止新 HAP 生成。可设置 `OHOS_NDK_HOME` 指向 `native/`。

DevEco Run 的顺序为 `PreBuild → PrepareNativeDependencies（恢复固定 Qt/CEF，再增量构建 PDFium）→ PrepareMarkdownResources → ConfigureCmake / CompileResource → BuildNativeWithNinja → ProcessLibs → PackageHap → RepackHapWithHnp → SignHap`。PDFium 编译和 Qt/NAPI 库同步在收集原生库之前完成；未改动的源码复用产物。这里只实现运行构建时更新，没有文件监听器，也没有在运行进程内卸载和替换 C++ 库。

Markdown 另由 `PrepareMarkdownResources` 在 `PreBuild` 后、`ConfigureCmake`、`CompileResource` 和 `PackageHap` 前准备。
它复用 Hvigor 的 Node/npm，首次 Run 联网按锁文件自动安装依赖，之后每次重新生成
`entry/src/main/resources/rawfile/markdown/` 下的 JS/CSS、许可证和校验清单，随 HAP 收集；
安装或构建失败会停止打包。通用预览模板还经 Qt qrc 编入工作区动态库，直接接收 MD 原文，
不为每篇 MD 另存 HTML。CEF 运行库使用项目内预编译版本，仅其包装层参与 C++ 编译；
轻量 HAR 模块已固定在 `third_party/cef/web_engine/`，OHPM 首次同步无需等候大依赖下载。
Python HNP 的运行时路径仍按下文配置，本次 Qt/CEF 自动准备不替代该配置。详见 [Markdown 构建说明](third_party/markdown/README.md)
和 [CEF 环境说明](third_party/cef/README.md)。

需要核对更新链路时，可运行 `python3 packaging/ohos/check_run_update.py`。它临时向 PDFium 加入两版构建探针，分别执行 HAP 构建并核对 `libs` 与 HAP 中的实际库内容，最后移除探针并重建最终包；不会安装或启动设备应用。该检查需要已配置可用的 HAP 签名，也会执行原有 Python HNP 打包。

首次克隆时连同固定版本的 MarkItDown 子模块一起下载：

```sh
git clone --recurse-submodules https://github.com/xristosstarnetgames439-del/HuiwenMCP.git HuiWenMCP
cd HuiWenMCP
git submodule status third_party/markitdown
```

已经克隆了主仓库的开发者，在仓库根目录补拉子模块：

```sh
git submodule update --init third_party/markitdown
git submodule status third_party/markitdown
```

再取得鸿蒙 arm64 Python 3.14 的已解压安装目录，要求包含 `bin/python3` 和 `lib/python3.14/site-packages/`。原有自检使用 [Harmonybrew 的预编译 Python 3.14.6](https://github.com/Harmonybrew/ohos-python/releases/tag/3.14.6) 通过。在被 Git 忽略的 `local.properties` 中配置本机路径：

```properties
python.runtime.dir=/绝对路径/python-3.14.6-ohos-arm64
# 可选：指定本机带 pip 的 Python，避免 DevEco 的 PATH 找不到解释器。
python.host.path=/绝对路径/宿主/python3
```

也可用 `HUIWEN_PYTHON_RUNTIME` 和 `HOST_PYTHON` 环境变量覆盖上述配置。相对配置路径按项目根目录解析。正常点击 DevEco Run 并执行 HAP 构建安装时，签名前的 `RepackHapWithHnp` 任务每次重新生成 HNP；仅热重载不保证执行此任务。仍可手动生成：

```sh
bash packaging/ohos/pack_python_hnp.sh /绝对路径/python-3.14.6-ohos-arm64
```

脚本使用本机 Python 的 pip 下载 `requirements-pdf.txt` 中锁定的鸿蒙 arm64/musl wheel。目标依赖按清单 SHA-256 缓存在被 Git 忽略的 `.hnp-cache/`，清单变化时重新安装；需要强制刷新依赖时删除该缓存。MarkItDown 子模块、汇文 Python API、自检和转换脚本每次重新复制，随后应用 `markitdown-no-magika.patch` 并用 SDK 的 `hnpcli` 生成新 HNP。只有成功生成非空归档后才替换 `hnp/arm64-v8a/huiwen_python.hnp`；任一步失败都会停止构建，不使用旧 HNP 继续安装。若 SDK 工具不在默认位置，可设置 `HNPCLI=/绝对路径/hnpcli`。HNP 的 `name/version` 固定为 `huiwen_python/1.0.0`。

MarkItDown 官方源码保持在子模块的 `v0.1.8` 提交；它采用 MIT 许可，可修改并商用，但分发时需保留许可与第三方声明。其官方包强制依赖 Magika/ONNX Runtime，而当前目标架构无法直接解析 ONNX Runtime wheel。仓库中的补丁只让 Magika 可选；本阶段输入已由选择器限定为 `.pdf`，按扩展名交给 PDF 转换器。补丁不改动子模块原始源码。pip 能解析目标架构 wheel 只说明可获取包，不代表鸿蒙设备能加载其中的原生 `.so`；需在设备上完成导入及实际 PDF 转换验收。

Hvigor 的默认 `PackageHap` 不会自动带入根目录的 HNP。`entry/hvigorfile.ts` 在 `PackageHap` 后、`SignHap` 前先生成 HNP，再调用 SDK `app_packing_tool.jar --hnp-path` 重打未签名 HAP，并检查归档中是否存在 `hnp/arm64-v8a/huiwen_python.hnp`。构建输出中的 `[HUIWEN_HNP] START`、`DEPENDENCIES install/cached`、`GENERATED`、`DONE` 可用于核对流程。SDK 不在 DevEco Studio 默认安装目录时，设置 `DEVECO_SDK_HOME` 或 `OHOS_SDK_HOME` 指向 SDK 根目录。源码修改后的实际安装更新仍需设备验证；转换诊断在 HiLog 中筛选 `HuiwenPython`，核对 `RUNTIME` 的算法标识。

```sh
cd entry && ohpm install
cd ..
hvigorw assembleHap --mode module -p product=default -p module=entry@default
```

也可以在 DevEco Studio 中使用 **Build Hap(s)/APP(s)**。修改代码后至少执行 `git diff --check`；需要验证设备行为时，再安装生成的签名 HAP 并启动 `EntryAbility`。Python 运行时自检已在真机通过；后续若更换运行时或 HNP 内容，可临时调用保留的 `checkPython()` 再验证。本机签名文件已被 Git 忽略，推送前用 `git status` 确认它没有进入提交。

## 维护约定

- 新增功能时更新“当前状态”，区分已运行的能力与尚未接入的入口。
- 调整模块、原生库或打包方式时更新“工程结构”和“本地构建”。
- 不把生成的 HAP、构建缓存或本机 SDK 路径写成跨机器通用的依赖。
- 不提交 `entry/libs/` 和 `third_party/qt/` 中的本机开发依赖；NAPI、Qt 应用与 PDFium `.so` 按源码增量生成，Qt 运行库由准备脚本导入。主仓库只固定 PDFium 子模块提交，fork 修改在其独立仓库维护。

## ArkTS 首页功能

- 窗口大小和窄侧栏保持原有设置。除侧栏、外边距及分区间距外，介绍区与主区域使用 `layoutWeight(3)` / `layoutWeight(7)` 分配宽度，主区域取消固定 360vp 宽度。
- “开始使用”下三个入口保持等宽列布局，浅灰正方形边长为所在列宽的 50%，即上一版方块边长的一半；方块内只放居中的 SVG，文档预览、文档编辑、文档转化名称以普通字重显示在方块外下方。`HomeFeatureCard` 只负责显示和点击事件，`HomePage` 保存当前功能选项。
- 点击入口后，按 `docs/自定义弹窗模版.md` 使用 `getUIContext().getPromptAction().openCustomDialog()` 打开居中的独立格式选择子窗口，采用 `showInSubWindow: true`、`isModal: false` 的组合（该接口禁止子窗口同时启用模态，也不支持 `CustomDialogController` 的 `customStyle` 参数）；首页保存返回的弹窗 ID，通过 `closeCustomDialog(id)` 统一关闭，并处理打开失败、外部取消及首页退出期间的异步清理。系统背板与内容组件 `HomeFormatDialog` 共用 `HOME_FORMAT_DIALOG_WIDTH = 360`，内外宽度统一固定为 360vp，不再使用 90% 限制；高度随内容。只由系统背板绘制浅灰背景、16vp 圆角和阴影，关闭背景模糊，内部保持透明，避免宽度不一致留下透明空白。三个弹窗共用内容布局，内容居中；左右及底部内边距为 24vp，顶部为 40vp，为右上角关闭按钮留位；格式选项自身内边距为 12vp，转换入口使用透明背景，不再额外叠加白底和边框。弹窗不在首页内展开或预留面板高度，右上角仅保留 28×28vp 关闭按钮，内含 18×18vp 的 × SVG，也可点击外部取消。预览和编辑窗口仅展示 Word、PDF、MD，暂不选择文件或跳转；转化窗口展示 PDF→MD 组合图标入口。点击该入口后由 `HomeFormatDialog.onConvertRequested` 通知首页，关闭子窗口成功后再启动现有 Qt 工作区，仍在 Qt 窗口就绪后结束首页 Ability。启动期间禁用重复操作，失败显示错误。
- `HomeFormatDialog` 通过 `title` 参数接收首页传入的标题，预览为“请选择要预览的格式”，编辑为“请选择要编辑的格式”，转化为“请选择要转化的格式”。标题以 18vp 普通字重显示在内容左上方，与格式选项间隔 16vp；保留右上角关闭按钮、360vp 内外统一宽度及原有背景设置。
- 功能 SVG 使用 Feather `book-open`、`edit-3`、`repeat`，转化方向使用 `arrow-right`，弹窗关闭使用 `x`，许可随 `rawfile/licenses/feather-icons-LICENSE.txt` 分发。格式 PNG 原样复用 Qt 已有图标，来源见 [文件格式图标说明](qt_app/resources/file-icons/README.md)。所有图标从应用资源加载，无需运行时下载。

## ArkTS 首页侧栏

- 侧栏固定在启动首页左侧，宽度 60vp；底部仅一个三横线按钮，尺寸 36×36vp，距底部 20vp。图标为 `resources/base/media/sidebar_menu.svg`，来自 [Feather Menu](https://github.com/feathericons/feather/blob/main/icons/menu.svg)，保留三横线几何形状并固定颜色，以 24×24vp 的 `Image` 显示；[上游 MIT 许可](entry/src/main/resources/rawfile/licenses/feather-icons-LICENSE.txt) 随 rawfile 分发。按钮保留零内边距，不依赖字符字体或文本省略行为。功能卡片显示在侧栏右侧的主区域。
- `EntryAbility` 在页面加载后，先设置 `setWindowBackgroundColor('#00000000')`，再独立设置 `setWindowContainerColor('#00000000', '#FFF5F7FB')`；后者需要 `ohos.permission.SET_WINDOW_TRANSPARENT`，已在模块中声明。焦点态容器透明，非焦点态使用不透明底色。两个步骤各自处理异常，设置失败时保留必要错误日志，不阻断首页使用。根布局透明，侧栏背景为 `#fae5e9ec`（约 98% 不透明度），主内容背景仍为实色；只调整背景透明度，按钮保持清晰，不额外增加模糊效果。
- 侧栏组件通过 `FunctionSidebar.onMenuRequested` 提供菜单点击入口，首页暂不绑定业务操作；Qt 转换工作区不增加侧栏。

## Qt 工作区验收

本阶段页面入口为 `qt_app/src/workspace/ConversionWindow.cpp`。未创建后续侧栏目录；新增侧栏时使用 `sidebars/`，避免与输入/输出区域混淆。

1. 点击 ArkTS 首页“文档转化”，再点击独立格式选择窗口中的“PDF → MD”，格式子窗口关闭后打开 Qt；Qt 默认最大化并保留系统标题栏、窗口边框及窗口控制按钮。查看 `QtWorkspace` 标签下的 `HUIWEN_QT_START`、`HUIWEN_QT_READY`，确认只有 `EntryAbility` 随后结束，`QtWorkspaceAbility` 保持运行；不应再出现 `newLocalStorage not found`、`Need object`。
2. 左右文档区域占满状态栏上方的可用高度，底部状态栏保持紧凑；还原窗口并调整大小时布局应同步变化。悬停中间细线出现水平分割光标；拖动改变两侧宽度，底部小型“转化”按钮保持居中在线上。
3. 点击左侧虚线加号选择 PDF，取消应保留原状态；选取后显示 PDF 占位图和原文件名，上方出现“预览”和“更换文件”。点击“预览”应显示正文，滚动可查看后续页；拖动分隔线或缩放窗口后，页面应按当前可用宽度等比重排并重新渲染。“返回文件”回到卡片；更换文件后回到卡片，从新文件第一页开始预览。损坏、加密文件应提示原因。
4. 点击“转化”，操作期间不能重复选择或提交。成功后右侧显示 MD 占位图和输出文件名，底部显示 `code=0`、成功提示和完整路径；失败显示错误码及处理建议，按钮变为“重新转化”。重试期间禁用按钮，成功后恢复“转化”；更换文件恢复初始状态，取消选择保留原状态。
5. 转换成功后底部显示完整 MD 路径；点击 MD 卡片，沿用系统文件管理器打开文件夹，文件夹内能看到真实 MD。不存在、为空或不可读取的结果应在跳转前明确报错，不能误报转换成功。
6. 日志搜索 `HUIWEN_OUTPUT`：`import_success` 为导入落盘路径，`convert_start` 为输入/输出路径，`verified` 为磁盘验证及字节数，`convert_success` 为成功通知，`locate_start` 为点击时重新检查，`locate_requested` 为鸿蒙桥接已提交。系统跳转结果继续查看原有 `HUIWEN_QT_OPEN_REQUEST`、`HUIWEN_QT_OPEN_REQUEST_ACCEPTED` 或 `HUIWEN_QT_OPEN_FAILED`。
7. 新导入按本地日期归档，例如 `inputs/2026-10-20/中文 报告.pdf`、`outputs/2026-10-20/中文 报告.md`，不再追加随机串。同一天同名输入或输出已存在时，两者统一追加 ` (2)`、` (3)`，避免覆盖；同一次导入后再次转换仍更新当前结果。日期在导入时确定；历史目录及文件保留，旧 `outputs/md/` 仍支持原有文件管理器跳转。Markdown 导入同样归入当天 `inputs/` 目录。

Python 固定入口在 stdout 最后一行返回 `HUIWEN_CONVERT_RESULT {"code":0,"message":"转化成功，Markdown 文件已生成。","outputPath":"/沙箱/outputs/日期/文件.md"}`。失败时 `code` 非零、`outputPath` 为空、进程退出码为 1；详细异常写入日志，界面展示中文原因和处理建议。C++ 独立保存完整结果行，日志尾部缓存仍限制为 4096 字节，结果不受日志截断影响。成功仍保留 `HUIWEN_CONVERT_OK` 标记，原 ArkTS 接口继续成功返回路径、失败拒绝 Promise。

错误码：`1001` 参数或格式错误，`1002` 输入不存在，`1003` 加密或禁止提取，`1004` PDF 损坏或不支持，`1005` 无可提取文字（暂不支持 OCR），`1006` 读写权限不足，`1007` 存储不足，`1008` 组件缺失，`1099` 其他转换异常；`2001` 原生进程或结果协议异常，`2002` 输出文件校验失败。C++ 同时验证字段类型、退出码、预期输出路径及真实非空文件，不能仅凭成功字段通知界面。

HAP 包结构校验：

```sh
python3 packaging/ohos/check_workspace_hap.py entry/build/default/outputs/default/entry-default-signed.hap
```

Windows/macOS 的同一 Widgets 源码提供桌面入口，尚未编译验证和制作安装包；CMake 需通过 `PDFIUM_LIBRARY` 指定与宿主架构匹配的 PDFium 动态库。桌面转换需安装同一 Python 依赖、设置 `PYTHONPATH` 为仓库的 `converter/`，`HUIWEN_CONVERT_SCRIPT` 为 `converter/pdf2md/convert.py` 的绝对路径；可用 `HUIWEN_PYTHON` 指定解释器。

PDF 预览桌面回归检查（独立工程，不加入 HAP；需要桌面 Qt 5.15 和对应宿主架构 PDFium）：

```sh
cmake -S qt_app/tests -B /tmp/huiwen-preview-check \
  -DCMAKE_PREFIX_PATH=/桌面Qt目录 -DPDFIUM_LIBRARY=/本机PDFium动态库
cmake --build /tmp/huiwen-preview-check
QT_QPA_PLATFORM=offscreen /tmp/huiwen-preview-check/pdf_preview_check
```

该检查生成两页彩色 PDF，验证导入后按钮、真实页面像素、多页滚动、调整宽度、更换文档与忙碌状态。鸿蒙 C++ 编译及 HAP 打包已通过。
