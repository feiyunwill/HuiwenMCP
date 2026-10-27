/* 输入文档区域的独立入口：显示选取状态，通过信号请求系统选择器。 */
#pragma once
#include "sandbox/DocumentItem.h"
#include <QVector>
#include <QWidget>
class FileCard;
class PdfPreviewWidget;
class QPushButton;
class QStackedWidget;

class SourceDocumentView : public QWidget
{
    Q_OBJECT
  public:
    explicit SourceDocumentView(QWidget *parent = nullptr);
    void setDocuments(const QVector<DocumentItem> &documents);
    void setBusy(bool busy);
    bool isPreviewReady() const;
    void setScrollPosition(int page, double fraction, double progress);
    void publishScrollPosition();
  signals:
    void selectionRequested();
    void documentOpened();
    void previewReadyChanged(bool ready);
    void scrollPositionChanged(int page, double fraction, double progress);

  private:
    FileCard *card_;
    QWidget *toolbar_;
    QPushButton *previewButton_;
    QPushButton *replaceButton_;
    QStackedWidget *stack_;
    PdfPreviewWidget *preview_;
    QString inputPath_;
};
