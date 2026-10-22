/* Markdown 只读正文容器；浏览器只在预览可见时创建。 */
#pragma once
#include <QWidget>
#include <memory>

class MarkdownPreviewWidget : public QWidget
{
  public:
    explicit MarkdownPreviewWidget(QWidget *parent = nullptr);
    ~MarkdownPreviewWidget() override;
    void setDocument(const QString &path);
    void setBottomInset(int inset);

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
