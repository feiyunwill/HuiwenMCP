/* 输出区域由文件菜单控制展示方式；未转换保持提示，生成结果后按当前查看方式展示。 */
#include "MarkdownResultView.h"
#include "MarkdownPreviewWidget.h"
#include "components/FileCard.h"
#include <QFileInfo>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QScrollArea>

MarkdownResultView::MarkdownResultView(QWidget *parent)
    : QWidget(parent), card_(new FileCard(this)), stack_(new QStackedWidget(this)),
      preview_(new MarkdownPreviewWidget(stack_))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setBackgroundRole(QPalette::Window);
    scroll->viewport()->setBackgroundRole(QPalette::Window);
    scroll->viewport()->setObjectName(QStringLiteral("resultDocumentViewport"));
    auto *content = new QWidget;
    content->setObjectName(QStringLiteral("resultDocumentBackground"));
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
    setResultFile({});
    connect(card_, &QPushButton::clicked, this, [this] { emit locationRequested(path_); });
    connect(preview_, &MarkdownPreviewWidget::previewReadyChanged, this,
            &MarkdownResultView::previewReadyChanged);
    connect(preview_, &MarkdownPreviewWidget::scrollPositionChanged, this,
            &MarkdownResultView::scrollPositionChanged);
}

void MarkdownResultView::setResultFile(const QString &path)
{
    path_ = path;
    stack_->setCurrentIndex(0);
    preview_->setDocument({});
    if (path.isEmpty())
        card_->setPrompt(QStringLiteral("待转化 PDF"), false);
    else
    {
        card_->setFile(QStringLiteral("MD"), QFileInfo(path).fileName());
        card_->setEnabled(true);
    }
    setPreviewMode(previewMode_);
}

void MarkdownResultView::setPreviewMode(bool preview)
{
    previewMode_ = preview;
    if (!preview || path_.isEmpty())
        stack_->setCurrentIndex(0);
    else if (stack_->currentWidget() != preview_)
    {
        preview_->setDocument(path_);
        stack_->setCurrentWidget(preview_);
    }
}

bool MarkdownResultView::isPreviewReady() const { return preview_->isPreviewReady(); }

void MarkdownResultView::setScrollPosition(int page, double fraction, double progress)
{
    preview_->setScrollPosition(page, fraction, progress);
}
