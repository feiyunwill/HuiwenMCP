/* 文件选择、结果位置和任务目录的统一入口；路径规则复用通用 SandboxStore。 */
#pragma once
#include "DocumentItem.h"
#include <QObject>

class SandboxService : public QObject
{
    Q_OBJECT
  public:
    explicit SandboxService(QObject *parent = nullptr);
    void selectPdf();
    void openResultLocation(const QString &path);
    void openTaskDirectory(const DocumentItem &document, bool output);
    void markUsed(const DocumentItem &document);
  signals:
    void documentSelected(const DocumentItem &document);
    void selectionFinished();
    void failed(const QString &message);
};
