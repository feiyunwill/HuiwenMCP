/* 输出区域实现；未转换显示提示，成功后显示 MD 占位图和文件名。 */
#include "MarkdownResultView.h"
#include "components/FileCard.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QScrollArea>

MarkdownResultView::MarkdownResultView(QWidget *parent) : QWidget(parent), card_(new FileCard(this))
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
    layout->addWidget(scroll);
    setMinimumWidth(260);
    setResultFile({});
    connect(card_, &QPushButton::clicked, this, [this] { emit locationRequested(path_); });
}

void MarkdownResultView::setResultFile(const QString &path)
{
    path_ = path;
    if (path.isEmpty())
        card_->setPrompt(QStringLiteral("待转化 PDF"), false);
    else
    {
        card_->setFile(QStringLiteral("MD"), QFileInfo(path).fileName());
        card_->setEnabled(true);
    }
}
