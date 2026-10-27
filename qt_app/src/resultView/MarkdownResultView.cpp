/* 输出区域实现；未转换显示提示，成功后显示 MD 占位图和文件名。 */
#include "MarkdownResultView.h"
#include "MarkdownPreviewWidget.h"
#include "components/FileCard.h"
#include <QFileInfo>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QScrollArea>

MarkdownResultView::MarkdownResultView(QWidget *parent)
    : QWidget(parent), card_(new FileCard(this)), toolbar_(new QWidget(this)),
      previewButton_(new QPushButton(QStringLiteral("预览"), toolbar_)), stack_(new QStackedWidget(this)),
      preview_(new MarkdownPreviewWidget(stack_))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *tools = new QHBoxLayout(toolbar_);
    tools->setContentsMargins(24, 16, 24, 0);
    tools->addWidget(previewButton_);
    auto *openButton = new QPushButton(QStringLiteral("打开所在目录"), toolbar_);
    tools->addWidget(openButton);
    tools->addStretch();
    layout->addWidget(toolbar_);
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
    connect(openButton, &QPushButton::clicked, this, [this] { emit locationRequested(path_); });
    connect(previewButton_, &QPushButton::clicked, this,
            [this]
            {
                if (stack_->currentWidget() == preview_)
                {
                    stack_->setCurrentIndex(0);
                    previewButton_->setText(QStringLiteral("预览"));
                }
                else if (!path_.isEmpty())
                {
                    preview_->setDocument(path_);
                    stack_->setCurrentWidget(preview_);
                    previewButton_->setText(QStringLiteral("返回文件"));
                }
            });
}

void MarkdownResultView::setResultFile(const QString &path)
{
    path_ = path;
    stack_->setCurrentIndex(0);
    preview_->setDocument({});
    toolbar_->setVisible(!path.isEmpty());
    previewButton_->setText(QStringLiteral("预览"));
    if (path.isEmpty())
        card_->setPrompt(QStringLiteral("待转化 PDF"), false);
    else
    {
        card_->setFile(QStringLiteral("MD"), QFileInfo(path).fileName());
        card_->setEnabled(true);
    }
}

void MarkdownResultView::setPreviewBottomInset(int inset) { preview_->setBottomInset(inset); }

bool MarkdownResultView::isPreviewReady() const { return preview_->isPreviewReady(); }

void MarkdownResultView::setScrollPosition(int page, double fraction, double progress)
{
    preview_->setScrollPosition(page, fraction, progress);
}
