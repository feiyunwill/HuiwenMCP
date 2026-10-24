/* 鸿蒙授权文件适配：将系统选择器的 fd 复制到通用沙箱规则生成的路径。 */
#pragma once
#include "sandbox/DocumentItem.h"

#ifdef Q_OS_OPENHARMONY
using HarmonySandboxDocument = DocumentItem;

class HarmonySandbox
{
  public:
    static HarmonySandboxDocument importDocument(const QString &filesDir, const QString &name, int sourceFd,
                                                 const QString &kind, const QString &mode);
    static QString validateOutput(const QString &filesDir, const QString &path);
};
#endif
