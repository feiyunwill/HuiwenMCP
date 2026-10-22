/* Markdown 输出区域：先显示文件卡片，点击预览后切换正文。 */
#pragma once
#include <QWidget>
class FileCard;
class MarkdownPreviewWidget;
class QPushButton;
class QStackedWidget;

class MarkdownResultView : public QWidget
{
    Q_OBJECT
  public:
    explicit MarkdownResultView(QWidget *parent = nullptr);
    void setResultFile(const QString &path);
    void setPreviewBottomInset(int inset);
  signals:
    void locationRequested(const QString &path);

  private:
    FileCard *card_;
    QWidget *toolbar_;
    QPushButton *previewButton_;
    QStackedWidget *stack_;
    MarkdownPreviewWidget *preview_;
    QString path_;
};
