/* 沙箱文件模型：关联同次任务的输入输出、缓存打开路径和最近使用时间。 */
#pragma once
#include <QString>

struct DocumentItem
{
    QString kind;
    QString name;
    QString inputPath;
    QString outputPath;
    QString taskPath;
    QString mode;
    qint64 lastUsedAt = 0;
    QString openPath;
};
