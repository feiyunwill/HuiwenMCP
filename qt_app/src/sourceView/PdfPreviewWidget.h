/* PDF 正文预览：按区域宽度排版，后台渲染可见页。 */
#pragma once
#include <QAbstractScrollArea>
#include <QCache>
#include <QFutureWatcher>
#include <QImage>
#include <QRectF>
#include <QTimer>
#include <QVector>
#include <memory>

struct PdfPreviewDocument;
struct PdfPreviewResult
{
    quint64 generation = 0;
    int page = -1;
    QVector<QSizeF> pageSizes;
    QImage image;
    QString error;
};

class PdfPreviewWidget : public QAbstractScrollArea
{
    Q_OBJECT
  public:
    explicit PdfPreviewWidget(QWidget *parent = nullptr);
    ~PdfPreviewWidget() override;
    void setDocument(const QString &path);

  protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;

  private:
    void layoutPages();
    void requestRender();
    int firstVisiblePage() const;
    QSize renderSize(int page) const;
    QString path_;
    QString message_;
    QString layoutError_;
    quint64 generation_ = 0;
    bool loadPending_ = false;
    bool taskPending_ = false;
    std::shared_ptr<PdfPreviewDocument> document_;
    QVector<QSizeF> pageSizes_;
    QVector<QRectF> pageRects_;
    QCache<int, QImage> images_{64 * 1024 * 1024};
    QFutureWatcher<PdfPreviewResult> watcher_;
    QTimer resizeTimer_;
};
