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

`package-lock.json` 固定 21 个运行时发布包，包含四个直接依赖及其传递依赖，
并锁定构建工具 `esbuild@0.25.12` 及各宿主平台的可选二进制包。
只核对了上述四个核心版本，不将整棵传递依赖树标为与桌面应用完全一致。
所有随包许可证已保存在 `licenses/THIRD_PARTY_LICENSES.txt`。

## 文件位置

- `node_modules/`：npm 发布包，包含分发模块及类型声明；部分包也附带源码。
- `sources/`：按 npm 发布记录中的 `gitHead` 提取的四个官方源码快照，
  保留 TypeScript 实现、测试和许可声明，不包含嵌套 Git 仓库。
- `sources.json`：官方仓库、来源提交、包完整性值和快照路径。
- `sources/SHA256SUMS`、`sources/FILES.md`：源码逐文件校验值及迁入说明。
- `library.js`、`library.css`：浏览器库入口与基础容器样式，不创建页面。
- `../../packaging/ohos/build_markdown.mjs`：自动准备依赖并构建资源。
- `../../entry/src/main/resources/rawfile/markdown/`：生成的离线资源，随 HAP 分发。

发布包和源码快照为本地依赖，不进入主项目 Git；版本锁、来源记录及许可证进入 Git。
当前没有另建远程 fork；后续修改底层库时可基于记录的提交建立独立 fork。

## Run 自动构建

DevEco Run 的 `PrepareMarkdownResources` 任务在 `PreBuild` 后执行，
`CompileResource` 和 `PackageHap` 都等待该任务成功。每次都会重新构建 JS/CSS，
不依赖是否改动了 C++，也不调用 Qt、CEF 或 PDFium 的准备脚本。

构建直接复用运行 Hvigor 的 Node 和随附 npm，无需单独安装前端环境。
首次克隆后的第一次 Run 需要联网；脚本按锁文件执行 `npm ci`，不运行安装脚本，
同时安装构建工具及匹配构建电脑的可选二进制。依赖完整且版本锁、宿主平台未变时
复用 `node_modules/`，此时可离线重新构建；依赖缺失或版本锁变化时自动重新安装。
依赖安装失败或打包失败都会让任务失败，阻止继续生成 HAP。

固定输出目录为 `entry/src/main/resources/rawfile/markdown/`，包括：

- `huiwen-markdown.js`：面向 Chromium 114 的自包含 IIFE，提供全局 `HuiwenMarkdown`。
  导出 `EditorState`、`EditorView`、`markdown`、`markdownLanguage`、`parser`、`GFM`。
- `huiwen-markdown.css`：基础容器样式；CodeMirror 自带样式仍由 `EditorView` 注入。
- `THIRD_PARTY_LICENSES.txt`：21 个运行时依赖的完整许可证。
- `build.json`：锁文件、实际输入文件及产物的 SHA-256，便于核对更新。

只会在构建和校验成功后替换输出目录。生成资源被 Git 忽略；其他用户克隆时应取得
本目录的入口、版本锁、许可证和构建脚本，由首次 Run 自动还原 MD 依赖。
这项自动准备仅覆盖 Markdown；项目已有的 Qt、CEF、PDFium 和 Python 环境要求不变。

只构建 Markdown 时，可使用 DevEco 随附 Node 运行（macOS 默认安装位置）：

```sh
/Applications/DevEco-Studio.app/Contents/tools/node/bin/node packaging/ohos/build_markdown.mjs
```

命令在项目根目录执行；已使用同一套 Node 的终端也可执行 `npm run build --prefix third_party/markdown`。
`packaging/ohos/check_markdown_build.mjs` 在临时目录验证首次还原、再次构建、源码更新及失败阻断，
不构建 HAP，不运行设备应用。源码快照仍按 `sources.json` 从官方仓库另行获取，
正常构建使用 npm 发布包，不要求克隆者下载这些参考源码快照。

## 当前边界

这些库是 JavaScript / TypeScript 组件，不编译为 `.so`。现在已提供浏览器库产物，
无需运行时从网络或 `node_modules/` 加载模块；后续页面需同时加载生成的 JS 和 CSS。
目前不创建 HTML 页面、不修改 Qt 窗口或 CEF 初始化。
排版预览、界面样式、公式、图表以及同步滚动留在后续接入阶段实现。
