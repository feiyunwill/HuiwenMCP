/* 输入区域：导入后保留文件卡片，由顶部按钮切换 PDF 正文预览。 */
#include "SourceDocumentView.h"
#include "PdfPreviewWidget.h"
#include "components/FileCard.h"
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QScrollArea>

SourceDocumentView::SourceDocumentView(QWidget *parent)
    : QWidget(parent), card_(new FileCard(this)), toolbar_(new QWidget(this)),
      previewButton_(new QPushButton(QStringLiteral("预览"), toolbar_)),
      replaceButton_(new QPushButton(QStringLiteral("更换文件"), toolbar_)),
      stack_(new QStackedWidget(this)), preview_(new PdfPreviewWidget(stack_))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *tools = new QHBoxLayout(toolbar_);
    tools->setContentsMargins(24, 16, 24, 0);
    tools->addWidget(previewButton_);
    tools->addWidget(replaceButton_);
    tools->addStretch();
    layout->addWidget(toolbar_);
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
    connect(replaceButton_, &QPushButton::clicked, this, &SourceDocumentView::selectionRequested);
    connect(preview_, &PdfPreviewWidget::documentOpened, this,
            [this](const QString &path)
            {
                if (path == inputPath_)
                    emit documentOpened();
            });
    connect(previewButton_, &QPushButton::clicked, this,
            [this]
            {
                if (stack_->currentWidget() == preview_)
                {
                    stack_->setCurrentIndex(0);
                    previewButton_->setText(QStringLiteral("预览"));
                }
                else if (!inputPath_.isEmpty())
                {
                    preview_->setDocument(inputPath_);
                    stack_->setCurrentWidget(preview_);
                    previewButton_->setText(QStringLiteral("返回文件"));
                }
            });
}

void SourceDocumentView::setDocuments(const QVector<DocumentItem> &documents)
{
    stack_->setCurrentIndex(0);
    preview_->setDocument({});
    inputPath_ = documents.isEmpty() ? QString() : documents.first().inputPath;
    toolbar_->setVisible(!inputPath_.isEmpty());
    previewButton_->setText(QStringLiteral("预览"));
    if (documents.isEmpty())
        card_->setPrompt(QStringLiteral("请选择 PDF"), true);
    else
        card_->setFile(documents.first().kind, documents.first().name);
}

void SourceDocumentView::setBusy(bool busy)
{
    card_->setEnabled(!busy);
    previewButton_->setEnabled(!busy && !inputPath_.isEmpty());
    replaceButton_->setEnabled(!busy);
}
