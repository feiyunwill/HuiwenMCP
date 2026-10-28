/* 输入区域只显示文件卡片或 PDF 正文；查看方式和更换入口由顶部文件菜单统一控制。 */
#include "SourceDocumentView.h"
#include "PdfPreviewWidget.h"
#include "components/FileCard.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QScrollArea>

SourceDocumentView::SourceDocumentView(QWidget *parent)
    : QWidget(parent), card_(new FileCard(this)), stack_(new QStackedWidget(this)),
      preview_(new PdfPreviewWidget(stack_))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setBackgroundRole(QPalette::Window);
    scroll->viewport()->setBackgroundRole(QPalette::Window);
    scroll->viewport()->setObjectName(QStringLiteral("sourceDocumentViewport"));
    auto *content = new QWidget;
    content->setObjectName(QStringLiteral("sourceDocumentBackground"));
    content->setAttribute(Qt::WA_StyledBackground);
    auto *cards = new QVBoxLayout(content);
    cards->setContentsMargins(32, 32, 32, 112);
    cards->addStretch();
    cards->addWidget(card_, 0, Qt::AlignHCenter);
    cards->addStretch();
    scroll->setWidget(content);
    stack_->addWidget(scroll);
    stack_->addWidget(preview_);
    layout->addWidget(stack_, 1);
    setMinimumWidth(260);
    setDocuments({});
    connect(card_, &QPushButton::clicked, this, &SourceDocumentView::selectionRequested);
    connect(preview_, &PdfPreviewWidget::previewReadyChanged, this,
            &SourceDocumentView::previewReadyChanged);
    connect(preview_, &PdfPreviewWidget::scrollPositionChanged, this,
            &SourceDocumentView::scrollPositionChanged);
    connect(preview_, &PdfPreviewWidget::documentOpened, this,
            [this](const QString &path)
            {
                if (path == inputPath_)
                    emit documentOpened();
            });
}

void SourceDocumentView::setDocuments(const QVector<DocumentItem> &documents)
{
    stack_->setCurrentIndex(0);
    preview_->setDocument({});
    inputPath_ = documents.isEmpty() ? QString() : documents.first().inputPath;
    if (documents.isEmpty())
        card_->setPrompt(QStringLiteral("请选择 PDF"), true);
    else
        card_->setFile(documents.first().kind, documents.first().name);
    setPreviewMode(previewMode_);
}

void SourceDocumentView::setBusy(bool busy)
{
    card_->setEnabled(!busy);
}

void SourceDocumentView::setPreviewMode(bool preview)
{
    previewMode_ = preview;
    if (!preview || inputPath_.isEmpty())
        stack_->setCurrentIndex(0);
    else if (stack_->currentWidget() != preview_)
    {
        preview_->setDocument(inputPath_);
        stack_->setCurrentWidget(preview_);
    }
}

bool SourceDocumentView::isPreviewReady() const { return preview_->isPreviewReady(); }

void SourceDocumentView::setScrollPosition(int page, double fraction, double progress)
{
    preview_->setScrollPosition(page, fraction, progress);
}

void SourceDocumentView::publishScrollPosition() { preview_->publishScrollPosition(); }
