# 文件格式图标

原始 PNG 来自微软 Office 官方 CDN，供 Qt 文件卡片使用，通过 ../file-icons.qrc 打包，运行时无需网络。

- 来源：https://github.com/microsoft/fluentui/blob/master/packages/react-file-type-icons/src/initializeFileTypeIcons.tsx
- 格式映射：https://github.com/microsoft/fluentui/blob/master/packages/react-file-type-icons/src/FileTypeIconMap.ts
- 下载地址：https://res.cdn.office.net/files/fabric-cdn-prod_20260113.001/assets/item-types/96/{类型}.png
- 下载日期：2026-10-12
- 类型：pdf、md、docx、xlsx、pptx、txt、photo、genericfile。

Markdown 图标使用当前官方版本 `fabric-cdn-prod_20260917.001`（上面的旧版不包含 md.png）；其他图标使用上述 `20260113.001` 版本。

保留微软原始图片，不裁切、不重绘。图标相关权利归原权利人所有；Fluent UI 项目许可及商标说明参见 https://github.com/microsoft/fluentui/blob/master/LICENSE 。
