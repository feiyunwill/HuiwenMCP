# CodeMirror 6 与 Markdown 解析依赖

按 CodeMirror 6 + CEF Web 页面路线准备编辑和解析底座。
以下四个版本与 2026-10-14 检查的本机桌面应用第三方清单一致，应用版本为
`26.1007.71326`；本目录没有复制应用自身的界面、样式或私有扩展。

| 包 | 固定版本 | 功能 |
| --- | --- | --- |
| `@codemirror/view` | `6.43.6` | DOM 编辑器视图、输入和文档装饰 |
| `@codemirror/state` | `6.7.1` | 文档状态、选择、事务和扩展配置 |
| `@codemirror/lang-markdown` | `6.5.0` | Markdown 语言支持和编辑辅助 |
| `@lezer/markdown` | `1.7.2` | 增量 Markdown 解析及语法扩展 |

上述四个包以外，本项目另使用 `markdown-it@14.3.3` 将 MD 原文渲染为 HTML。
`package-lock.json` 固定 28 个运行时发布包，包含编辑、解析和正文渲染依赖，
并锁定构建工具 `esbuild@0.25.12` 及各宿主平台的可选二进制包。
只核对了上述四个核心版本，不将整棵传递依赖树标为与桌面应用完全一致。
所有随包许可证已保存在 `licenses/THIRD_PARTY_LICENSES.txt`。

## 文件位置

- `node_modules/`：npm 发布包，包含分发模块及类型声明；部分包也附带源码。
- `sources/`：按 npm 发布记录中的 `gitHead` 提取的四个官方源码快照，
  保留 TypeScript 实现、测试和许可声明，不包含嵌套 Git 仓库。
- `sources.json`：官方仓库、来源提交、包完整性值和快照路径。
- `sources/SHA256SUMS`、`sources/FILES.md`：源码逐文件校验值及迁入说明。
- `library.js`、`library.css`：浏览器库入口与正文样式。
- `preview.html`：所有文档共用的离线预览模板，不是每篇 MD 的导出文件。
- `../../packaging/ohos/build_markdown.mjs`：自动准备依赖并构建资源。
- `../../entry/src/main/resources/rawfile/markdown/`：生成的离线资源，随 HAP 分发。

发布包和源码快照为本地依赖，不进入主项目 Git；版本锁、来源记录及许可证进入 Git。
当前没有另建远程 fork；后续修改底层库时可基于记录的提交建立独立 fork。

## Run 自动构建

DevEco Run 的 `PrepareMarkdownResources` 任务在 `PreBuild` 后执行，
`ConfigureCmake`、`CompileResource` 和 `PackageHap` 都等待该任务成功。
每次重新构建 JS/CSS 和预览模板，再同步 CEF 平台模块及核心数据；不重编 CEF、Qt 本体，
PDFium 的现有准备任务保持独立。

构建直接复用运行 Hvigor 的 Node 和随附 npm，无需单独安装前端环境。
首次克隆后的第一次 Run 需要联网；脚本按锁文件执行 `npm ci`，不运行安装脚本，
同时安装构建工具及匹配构建电脑的可选二进制。依赖完整且版本锁、宿主平台未变时
复用 `node_modules/`，此时可离线重新构建；依赖缺失或版本锁变化时自动重新安装。
依赖安装失败或打包失败都会让任务失败，阻止继续生成 HAP。

固定输出目录为 `entry/src/main/resources/rawfile/markdown/`，包括：

- `huiwen-markdown.js`：面向 Chromium 114 的自包含 IIFE，提供全局 `HuiwenMarkdown`。
  保留编辑／解析接口，另导出 `renderMarkdown` 和 `showPreview`。
- `huiwen-markdown.css`：标题、段落、表格、引用、代码块及容器样式。
- `preview.html`：内联 JS/CSS 的通用模板，经 Qt qrc 编入 `libhuiwen_qt.so`，
  无需假定 HAP 中的 rawfile 能被普通文件 API 读取。
- `THIRD_PARTY_LICENSES.txt`：28 个运行时依赖的完整许可证。
- `build.json`：锁文件、实际输入文件及产物的 SHA-256，便于核对更新。

只会在构建和校验成功后替换输出目录。生成资源被 Git 忽略；其他用户克隆时应取得
本目录的入口、版本锁、许可证和构建脚本，由首次 Run 自动还原 MD 依赖。
首次使用 CEF 前仍需准备项目内 CEF SDK，并执行 `python3 packaging/ohos/prepare_cef_preview.py`，
然后由 DevEco 同步新增的本地 `web_engine` 模块依赖。`prepare_cef.py` 迁入 SDK 时会自动完成
这一步；Qt、PDFium 和 Python 的原有环境要求不变。

只构建 Markdown 时，可使用 DevEco 随附 Node 运行（macOS 默认安装位置）：

```sh
/Applications/DevEco-Studio.app/Contents/tools/node/bin/node packaging/ohos/build_markdown.mjs
```

命令在项目根目录执行；已使用同一套 Node 的终端也可执行 `npm run build --prefix third_party/markdown`。
`packaging/ohos/check_markdown_build.mjs` 在临时目录验证首次还原、再次构建、源码更新及失败阻断，
不构建 HAP，不运行设备应用。源码快照仍按 `sources.json` 从官方仓库另行获取，
正常构建使用 npm 发布包，不要求克隆者下载这些参考源码快照。

## 当前边界

转换成功后仍显示 MD 文件卡片，点击“预览”才创建 CEF 子视图并直接读取 UTF-8 原文。
“返回文件”、重新转换和重新选 PDF 都会关闭旧视图；浏览器尺寸随分栏变化，底部避开
Qt 悬浮转化按钮。首次初始化有超时提示；只读预览限制单文件为 16 MiB。

模板不执行 MD 中的原始 HTML，禁止外部页面跳转和弹窗，不从网络下载页面资源。
本阶段仅允许内嵌 data 图片，普通相对路径／网络图片尚未接入；公式、图表、编辑和
与 PDF 同步滚动也尚未接入。鸿蒙源代码链路已连接，实际 CEF 启动、显示和窗口关闭
仍需编译、打包及真机验收；桌面构建只显示未配置 CEF 的提示。
