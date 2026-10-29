/* Markdown 输出区域：由外部切换卡片或只读正文，不持有预览及目录操作按钮。 */
#pragma once
#include <QWidget>
class FileCard;
class MarkdownPreviewWidget;
class QStackedWidget;

class MarkdownResultView : public QWidget
{
    Q_OBJECT
  public:
    explicit MarkdownResultView(QWidget *parent = nullptr);
    void setResultFile(const QString &path);
    // 结果是否已设置，与 CEF 是否完成加载无关。
    bool hasResultFile() const
    {
        return !path_.isEmpty();
    }
    void setPreviewMode(bool preview);
    bool isPreviewReady() const;
    void setScrollPosition(int page, double fraction, double progress);
  signals:
    void locationRequested(const QString &path);
    void previewReadyChanged(bool ready);
    void scrollPositionChanged(int page, double fraction, double progress);

  private:
    FileCard *card_;
    QStackedWidget *stack_;
    MarkdownPreviewWidget *preview_;
    QString path_;
    bool previewMode_ = false;
};
