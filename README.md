# 汇文 MCP

鸿蒙 PC 上的 PDF 与 Markdown 阅读、转换应用。本阶段先把文字型 PDF 转成沙箱内的 MD 文件。

## 许可证

本项目自有代码采用 [PolyForm Noncommercial License 1.0.0](LICENSE)，官方原文见 [PolyForm 网站](https://polyformproject.org/licenses/noncommercial/1.0.0)。这是源码公开的非商业使用许可。

允许在许可规定的用途范围内使用、学习、研究、修改和分发。商业产品开发、销售、商业服务等超出许可范围的用途，需要另行取得相关权利人的授权。许可对教育机构等组织另有允许用途规定，以 `LICENSE` 原文为准。

第三方源码、依赖和资源仍遵循各自的许可证及版权声明，不受本项目许可证替代。贡献代码前，请确认你有权将该贡献按本项目许可证授权。

## 当前状态

- 首页保持 ArkTS；PDF/MD 预览按钮仍只导入文件，PDF 转 MD 按钮改为启动独立 Qt 工作区。
- Qt 开发依赖保存在 `third_party/qt/`，运行库从 `entry/libs/arm64-v8a/` 链接并随 HAP 分发；PDFium fork 保存在 `third_party/pdfium/`，Qt 左侧输入区域已接入其公开 C API 实现正文预览；新增功能尚未编译、打包或真机验收。
- DevEco Run 构建前执行 `PrepareNativeDependencies`，按当前 PDFium 源码增量生成 `libpdfium.so`；Qt/NAPI 的 C++ 编译后同步新库到 `libs`。没有保存源码自动构建、应用内动态替换库或保持进程状态的热重载。
- Qt 工作区默认最大化并保留系统标题栏和窗口边框，输入、输出区域解耦；中部分割线可拖动，悬浮“转化”按钮随分隔线移动。
- 输入文件卡片调用现有 DocumentStore 选择 PDF、复制沙箱并安排输出路径。导入成功后上方显示“预览”和“更换文件”，点击“预览”才切换为多页正文；支持滚动，随窗口/分栏宽度等比排版并重新渲染，可返回文件卡片。PDFium 后台串行渲染可见页，连续尺寸变化合并请求；位图缓存上限为 64 MiB，超长页面降低采样分辨率。Qt ConversionService 仍复用 PythonRunner 转换，右侧成功后显示 MD 文件卡片。
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
│   │   │   ├── pages/HomePage.ets      [已有] ArkTS 首页和功能入口
│   │   │   ├── pages/Index.ets         [已有] QPA 固定加载的 Qt XComponent 承载页
│   │   │   ├── services/ConversionService.ets [已有] 页面调用原生转换的统一入口
│   │   │   └── utils/DocumentStore.ets [已有] 文件选择、沙箱副本与输出路径
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
│       └── platform/harmony/qt_bridge.h/.cpp   Qt ↔ ArkTS 线程桥
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

当前 Qt 转换链为 `ConvertButton → Qt ConversionService.convertPdfToMd() → PythonRunner → convert_pdf.py → huiwen_converter.pdf_to_md() → MarkItDown → outputs/md/*.md`。Python API 与第三方库适配保持原有边界。原 ArkTS 转换接口仍保留，但首页按钮原来的即时转换调用已注释。

鸿蒙跳转由 `HomePage.ets → startAbility(QtWorkspaceAbility) → QPA.startQtApplication() → libhuiwen_qt.so/main()` 完成。Qt 窗口实际显示后通过 NAPI 线程安全回调通知新 Ability；新 Ability 通过应用 EventHub 通知首页，只有发起跳转的首页消费一次通知并调用 `terminateSelf()`。这只结束首页 Ability，不结束应用进程。

当前 QPA 固定加载 `pages/Index`，不能让该路由同时作为 ArkTS 首页。`EntryAbility` 加载独立的 `pages/HomePage`；`Index.ets` 仅创建 Qt 的 NODE 类型 XComponent。`QtWorkspaceAbility.newLocalStorage()` 提供存储，QPA 写入 `idName` 并传给承载页。若在承载页放首页事件监听，会在 Qt 就绪时把 Qt Ability 也关闭；缺少存储方法则会产生 `newLocalStorage not found`、`Need object` 等日志。

文件选择链为 `SourceDocumentView → SandboxService → QtBridge → QtWorkspaceAbility → DocumentStore`。Qt 请求进入 ArkTS 线程处理；结果经 NAPI 投递到 Qt 线程，不跨线程直接操作界面。新 Ability 使用自己的 Context，首页关闭后文件选择仍可继续。

仓库中的 `third_party/markitdown` 是固定版本的上游源码，**不会把整个 Git 仓库装进应用**。`pack_python_hnp.sh` 仅把运行所需的 `markitdown/`、`huiwen_converter/`、Python 依赖及其原生 `.so` 放入 HNP 的 `lib/python3.14/site-packages/`，固定命令脚本放在 `share/huiwen/`；HNP 同时包含 Python 解释器。构建 HAP 时，`libhuiwen_native.so`、`libhuiwen_qt.so` 和最小 Qt 运行库进入 HAP，生成的 `huiwen_python.hnp` 也随 HAP 分发。ArkTS 不能直接导入 HNP 内的 Python 函数，必须经应用自己的 NAPI 桥接调用。

### 接口边界

| 位置 | 项目接口 | 调用者与职责 |
| --- | --- | --- |
| `converter/huiwen_converter/__init__.py` | `pdf_to_md(input_path, output_path) -> Path` | Python 公开业务接口；生成非空 MD 后返回路径，失败抛出异常。后续格式实现并验证后，才新增对应的显式接口。 |
| `converter/huiwen_converter/_markitdown_adapter.py` | 内部 `pdf_to_markdown()` | 唯一直接导入 MarkItDown 的文件；不作为业务 API。 |
| `entry/src/main/ets/services/ConversionService.ets` | `pdfToMd(inputPath, outputPath): Promise<string>` | ArkTS 页面调用；只转发到 NAPI，成功返回 MD 路径。 |
| `entry/src/main/cpp/napi_bridge.cpp` | `convertPdf(inputPath, outputPath)` | 原生边界；异步启动 HNP 中的固定命令，不接受任意 Shell 命令。 |

## 沙箱文件路径

当前设备用户号为 `100` 时，应用私有沙箱的预期物理路径如下。程序通过 `getApplicationContext().filesDir` 获取运行时目录，不写死用户号和包路径；设备上的实际映射需在验收时核对。

```text
/data/app/el2/100/base/cn.com.HuiWenMCP/
└── files/
    ├── inputs/
    │   ├── pdf/                 PDF 预览、PDF 转 MD 选入的副本
    │   └── md/                  MD 预览选入的副本
    └── outputs/
        └── md/                  PDF 转换生成的 Markdown
```

| 按钮 | 选取后记录的路径 | 本阶段行为 |
| --- | --- | --- |
| PDF 预览 | `files/inputs/pdf/<唯一编号>.pdf` | 打开系统选择器、复制文件、留在首页 |
| MD 预览 | `files/inputs/md/<唯一编号>.md`（选择 `.markdown` 时保留该后缀） | 打开系统选择器、复制文件、留在首页 |
| PDF 转 MD | 输入：`files/inputs/pdf/<唯一编号>.pdf`；输出：`files/outputs/md/<原文件名>-<唯一编号>.md` | 首页打开 Qt；Qt 输入区域选取文件，点击转化后显示 MD 卡片 |

系统选择器返回的 URI 只用于读取选中文件；应用内部后续使用沙箱副本路径。转换成功后才生成非空 `.md` 文件并显示路径；文字无法提取的扫描件会提示需要 OCR，不生成空结果。ArkTS 首页仍只导入文件，正文预览由 Qt 工作区左侧的“预览”按钮进入。MD 卡片通过 `openLink('filemanager://openDirectory')` 和 `parameters.fileUri` 传入生成文件的 URI，请求系统文件管理器定位其所在的 `outputs/md/`；失败直接显示错误，不弹出保存选择器或导出副本。内部路径继续由 `Context.filesDir` 获取，鸿蒙 PC 文件管理器中的对应展示位置为 `/storage/Users/currentUser/appdata/el2/base/cn.com.HuiWenMCP/files/outputs/md`，实际定位能力需真机验证。

## 本地构建

工程当前配置为 HarmonyOS PC、`arm64-v8a`，目标 SDK 为 `6.1.1(24)`。首次克隆后，将 `build-profile.example.json5` 复制为本机的 `build-profile.json5`，需要安装到设备时再在 DevEco Studio 配置签名。

Qt 运行库已经使用 `.so`。首次准备开发依赖时，从同一套鸿蒙 Qt 5.15.12 SDK 导入所需头文件、官方 CMake 配置、mkspecs、宿主工具和五个运行库：

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

DevEco Run 的顺序为 `PreBuild → PrepareNativeDependencies → ConfigureCmake / CompileResource → BuildNativeWithNinja → ProcessLibs → PackageHap → RepackHapWithHnp → SignHap`。PDFium 编译和 Qt/NAPI 库同步在收集原生库之前完成；未改动的源码复用产物。这里只实现运行构建时更新，没有文件监听器，也没有在运行进程内卸载和替换 C++ 库。

Markdown 另由 `PrepareMarkdownResources` 在 `PreBuild` 后、`CompileResource` 和 `PackageHap` 前准备。
它复用 Hvigor 的 Node/npm，首次 Run 联网按锁文件自动安装依赖，之后每次重新生成
`entry/src/main/resources/rawfile/markdown/` 下的 JS/CSS、许可证和校验清单，随 HAP 收集；
安装或构建失败会停止打包。无需额外安装前端工具，不改动已有原生库构建逻辑，
也尚未接入编辑／预览页面。详见 [Markdown 构建说明](third_party/markdown/README.md)。

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

## Qt 工作区验收

本阶段页面入口为 `qt_app/src/workspace/ConversionWindow.cpp`。未创建后续侧栏目录；新增侧栏时使用 `sidebars/`，避免与输入/输出区域混淆。

1. 点击 ArkTS 首页“PDF 转 MD”，Qt 默认最大化并保留系统标题栏、窗口边框及窗口控制按钮；查看 `QtWorkspace` 标签下的 `HUIWEN_QT_START`、`HUIWEN_QT_READY`，确认只有 `EntryAbility` 随后结束，`QtWorkspaceAbility` 保持运行；不应再出现 `newLocalStorage not found`、`Need object`。
2. 左右文档区域占满状态栏上方的可用高度，底部状态栏保持紧凑；还原窗口并调整大小时布局应同步变化。悬停中间细线出现水平分割光标；拖动改变两侧宽度，底部小型“转化”按钮保持居中在线上。
3. 点击左侧虚线加号选择 PDF，取消应保留原状态；选取后显示 PDF 占位图和原文件名，上方出现“预览”和“更换文件”。点击“预览”应显示正文，滚动可查看后续页；拖动分隔线或缩放窗口后，页面应按当前可用宽度等比重排并重新渲染。“返回文件”回到卡片；更换文件后回到卡片，从新文件第一页开始预览。损坏、加密文件应提示原因。
4. 点击“转化”，操作期间不能重复选择或提交。成功后右侧显示 MD 占位图和输出文件名；失败在底部显示错误。再选一个 PDF 检查重复导入。
5. 点击 MD 卡片，应打开系统文件管理器并定位 `files/outputs/md`，不出现保存弹窗；原 MD 文件位置不变。查看 `QtWorkspace` 标签下的 `HUIWEN_QT_OPEN_REQUEST`（目录与文件 URI）、`HUIWEN_QT_OPEN_REQUEST_ACCEPTED`（仅表示拉起请求成功）或 `HUIWEN_QT_OPEN_FAILED`（错误码与消息）。请求成功但定位到错误目录时，也需记录实际打开位置，不能仅凭成功日志判定验收通过。

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

该检查生成两页彩色 PDF，验证导入后按钮、真实页面像素、多页滚动、调整宽度、更换文档与忙碌状态。本次仅做静态/格式检查，未执行此桌面回归、C++ 编译、HAP 打包和设备预览。
