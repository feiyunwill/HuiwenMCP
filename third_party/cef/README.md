# 项目内 CEF 环境

本目录迁入通达 OpenQtCef 工程当前持有的鸿蒙 ARM64 CEF 环境。
这不是 v114.14.12 发布包，也不是完整 Chromium 源码；版本以 `environment.json`
中的头文件版本、来源提交和运行库校验值为准。来源可能包含尚未提交的修改，
`sdk/files.json` 记录每个迁入文件的实际 SHA-256，提交号不能代替该清单。

## 文件位置

| 位置 | 内容 |
| --- | --- |
| `sdk/include` | CEF 公开和内部头文件，不包含通达的 Qt 副本及业务 stub |
| `sdk/cmake`、`sdk/libcef_dll` | 同套 CMake 配置和 C/C++ 包装源码 |
| `sdk/ohos`、`sdk/log` | 鸿蒙嵌入适配源码，以及该包装源码引用的日志实现 |
| `sdk/web_engine` | ArkTS 适配模块、原生类型声明和浏览器核心资源 |
| `sdk/Release` | 指向 `entry/libs/arm64-v8a` 的相对库链接 |
| `sdk/licenses` | 从同套 CEF 头文件保留的 BSD 许可声明 |
| `sdk/files.json` | 逐文件来源相对路径、大小及 SHA-256 |

运行库的唯一部署位置为 `entry/libs/arm64-v8a/`，包含 `libcef.so`、`libadapter.so`、
`libadapter_c.so`、`libc++_shared.so`。`libadapter_c.so` 来自通达的已剥离构建产物，
本次没有重新编译，因此尚未验证它与迁入源码的构建一致性。

## 重新准备

```sh
python3 packaging/ohos/prepare_cef.py --source /已有工程/OpenQtCef
```

脚本先检查必需文件、ARM64 ELF 架构和动态依赖，再复制并逐文件校验。
已有同名运行库的内容不一致时停止，不覆盖现有 Qt 或其他版本的运行库。
SDK 文件及大体积运行库遵循现有本地依赖策略，不进入 Git；准备脚本、说明和
`environment.json` 进入 Git。清单不保存本机绝对来源路径。

## 当前边界

- 尚未链接 CEF、添加 `web_engine` 模块依赖、修改 Ability 或启用 MD 窗口；
  本次也没有添加 Run 任务或执行编译、HAP 打包和设备运行。
- 未迁入通达 OA/IM、登录、数据库、托盘业务，以及 Qt 运行库、宿主工具、
  `unstripped` 调试库、构建缓存、浏览器示例网页和测试资源。
- 保留的适配源码仍有来源工程的行为：`WebQtAbility` 隐藏窗口装饰，日志实现使用
  通达沙箱日志目录，HAR 声明了通用浏览器权限。实际接入前须按汇文场景调整。
- 已验证的动态依赖存在于迁入库或本机 ARM64 SDK；这不证明设备兼容或浏览器可运行。
- 来源未提供独立的 Chromium 和厂商完整许可证材料。本次只保存能找到的声明；
  分发前仍需从对应发布包补齐，不将该环境标记为许可证材料齐全。
