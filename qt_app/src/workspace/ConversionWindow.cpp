/* 转换工作区：左右独立区域、可拖动分隔线，以及跟随分隔线的悬浮转化按钮。 */
#include "ConversionWindow.h"
#include "components/ConvertButton.h"
#include "resultView/MarkdownResultView.h"
#include "sandbox/SandboxService.h"
#include "services/ConversionService.h"
#include "sourceView/SourceDocumentView.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QSplitter>
#include <QSplitterHandle>
#include <QTimer>
#include <QVBoxLayout>

ConversionWindow::ConversionWindow(QWidget *parent)
    : QWidget(parent), splitter_(new QSplitter(Qt::Horizontal, this)), source_(new SourceDocumentView),
      result_(new MarkdownResultView), button_(new ConvertButton(this)), buttonBackdrop_(new QWidget(this)),
      conversion_(new ConversionService(this)), sandbox_(new SandboxService(this)), status_(new QLabel(this)),
      scrollSync_(new QCheckBox(QStringLiteral("同步滚动"), this))
{
    setWindowTitle(QStringLiteral("汇文MCP · 转换工作区"));
    setMinimumSize(640, 480);
    setAutoFillBackground(true);
    QPalette colors = palette();
    colors.setColor(QPalette::Window, QColor("#F5F7FB"));
    // 滚动区域默认使用 Base，和窗口及按钮遮罩统一为同一种背景色。
    colors.setColor(QPalette::Base, colors.color(QPalette::Window));
    setPalette(colors);
    setObjectName(QStringLiteral("conversionWindow"));
    setAttribute(Qt::WA_StyledBackground);
    buttonBackdrop_->setObjectName(QStringLiteral("conversionButtonBackdrop"));
    buttonBackdrop_->setAttribute(Qt::WA_StyledBackground);
    // 显式指定所有可见背景，避免平台样式为滚动内容另行填充默认颜色。
    setStyleSheet(QStringLiteral(
        "QWidget#conversionWindow, QWidget#conversionButtonBackdrop, "
        "QWidget#sourceDocumentBackground, QWidget#resultDocumentBackground, "
        "QWidget#sourceDocumentViewport, QWidget#resultDocumentViewport "
        "{ background-color: %1; }").arg(colors.color(QPalette::Window).name()));
    // 遮住按钮圆角及上下留白处的分割线，按钮始终位于遮挡层上方。
    buttonBackdrop_->setAutoFillBackground(true);
    buttonBackdrop_->setPalette(colors);
    buttonBackdrop_->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto *options = new QHBoxLayout;
    options->setContentsMargins(24, 8, 24, 0);
    options->addStretch();
    options->addWidget(scrollSync_);
    layout->addLayout(options);
    scrollSync_->setChecked(true);
    scrollSync_->setEnabled(false);
    scrollSync_->setToolTip(QStringLiteral("两侧预览就绪后，按原 PDF 页码同步；无页码时按总进度同步"));
    splitter_->addWidget(source_);
    splitter_->addWidget(result_);
    splitter_->setChildrenCollapsible(false);
    splitter_->setHandleWidth(7);
    splitter_->setSizes({600, 600});
    splitter_->handle(1)->setCursor(Qt::SplitHCursor);
    splitter_->setStyleSheet("QSplitter::handle:horizontal { background: #D4DEEC; margin: 0 3px; }");
    splitter_->installEventFilter(this);
    // 剩余高度全部分配给文档区域，避免空状态栏占据半个窗口。
    layout->addWidget(splitter_, 1);
    status_->setContentsMargins(16, 4, 16, 4);
    status_->setMinimumHeight(28);
    status_->setWordWrap(true);
    status_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(status_, 0);
    connect(splitter_, &QSplitter::splitterMoved, this, [this] { placeConvertButton(); });
    connect(source_, &SourceDocumentView::previewReadyChanged, this, [this] { refreshScrollSync(); });
    connect(result_, &MarkdownResultView::previewReadyChanged, this, [this] { refreshScrollSync(); });
    connect(scrollSync_, &QCheckBox::toggled, this, [this] { refreshScrollSync(); });
    connect(source_, &SourceDocumentView::scrollPositionChanged, this,
            [this](int page, double fraction, double progress)
            {
                if (scrollSync_->isEnabled() && scrollSync_->isChecked())
                    result_->setScrollPosition(page, fraction, progress);
            });
    connect(result_, &MarkdownResultView::scrollPositionChanged, this,
            [this](int page, double fraction, double progress)
            {
                if (scrollSync_->isEnabled() && scrollSync_->isChecked())
                    source_->setScrollPosition(page, fraction, progress);
            });
    connect(source_, &SourceDocumentView::selectionRequested, this,
            [this]
            {
                picking_ = true;
                refreshBusyState();
                sandbox_->selectPdf();
            });
    connect(sandbox_, &SandboxService::documentSelected, this,
            [this](const DocumentItem &document)
            {
                document_ = document;
                retry_ = false;
                status_->clear();
                source_->setDocuments({document});
                result_->setResultFile({});
            });
    connect(sandbox_, &SandboxService::selectionFinished, this,
            [this]
            {
                picking_ = false;
                refreshBusyState();
            });
    connect(sandbox_, &SandboxService::failed, status_, &QLabel::setText);
    connect(source_, &SourceDocumentView::documentOpened, this,
            [this]
            {
                sandbox_->markUsed(document_);
            });
    connect(result_, &MarkdownResultView::locationRequested, sandbox_, &SandboxService::openResultLocation);
    // 按钮独立接入转换服务，区域之间不互相调用。
    connect(button_, &QPushButton::clicked, this,
            [this]
            {
                converting_ = true;
                refreshBusyState();
                result_->setResultFile({});
                status_->setText(QStringLiteral("正在转化 PDF…"));
                sandbox_->markUsed(document_);
                conversion_->convertPdfToMd(document_.inputPath, document_.outputPath);
            });
    connect(conversion_, &ConversionService::succeeded, this,
            [this](const QString &path, const QString &message)
            {
                converting_ = false;
                retry_ = false;
                sandbox_->markUsed(document_);
                result_->setResultFile(path);
                status_->setText(QStringLiteral("[code=0] %1 路径：%2").arg(message, path));
                refreshBusyState();
            });
    connect(conversion_, &ConversionService::failed, this,
            [this](int code, const QString &message)
            {
                converting_ = false;
                retry_ = true;
                sandbox_->markUsed(document_);
                status_->setText(QStringLiteral("[code=%1] %2").arg(code).arg(message));
                refreshBusyState();
            });
}

void ConversionWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeConvertButton();
}

bool ConversionWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == splitter_ && event->type() == QEvent::Resize)
        QTimer::singleShot(0, this, [this] { placeConvertButton(); });
    return QWidget::eventFilter(watched, event);
}

void ConversionWindow::placeConvertButton()
{
    const QPoint center = splitter_->handle(1)->mapTo(this, splitter_->handle(1)->rect().center());
    button_->move(center.x() - button_->width() / 2,
                  splitter_->mapTo(this, QPoint(0, splitter_->height())).y() - button_->height() - 72);
    buttonBackdrop_->setGeometry(button_->geometry().adjusted(-2, -32, 2, 32));
    // CEF 是原生子节点，预览区域避开悬浮按钮，避免盖住 Qt 绘制的按钮和遮罩。
    const int resultBottom = result_->mapTo(this, QPoint(0, result_->height())).y();
    result_->setPreviewBottomInset(qMax(0, resultBottom - buttonBackdrop_->geometry().top()));
    buttonBackdrop_->raise();
    button_->raise();
}

void ConversionWindow::refreshBusyState()
{
    source_->setBusy(picking_ || converting_);
    button_->setState(!document_.inputPath.isEmpty(), picking_ || converting_, retry_);
    placeConvertButton();
}

void ConversionWindow::refreshScrollSync()
{
    const bool ready = source_->isPreviewReady() && result_->isPreviewReady();
    scrollSync_->setEnabled(ready);
    if (ready && scrollSync_->isChecked())
        source_->publishScrollPosition();
}
