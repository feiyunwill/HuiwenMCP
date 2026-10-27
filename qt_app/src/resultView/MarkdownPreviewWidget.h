/* Markdown 只读正文容器；浏览器只在预览可见时创建。 */
#pragma once
#include <QWidget>
#include <memory>

class MarkdownPreviewWidget : public QWidget
{
    Q_OBJECT
  public:
    explicit MarkdownPreviewWidget(QWidget *parent = nullptr);
    ~MarkdownPreviewWidget() override;
    void setDocument(const QString &path);
    void setBottomInset(int inset);
    bool isPreviewReady() const;
    void setScrollPosition(int page, double fraction, double progress);

  signals:
    void previewReadyChanged(bool ready);
    void scrollPositionChanged(int page, double fraction, double progress);

  protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    void openBrowser();
    void closeBrowser();
    void syncGeometry();
    QRect browserRect() const;
    class Private;
    std::unique_ptr<Private> d_;
};
