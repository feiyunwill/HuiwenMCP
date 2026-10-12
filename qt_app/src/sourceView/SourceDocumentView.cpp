/* 输入区域实现；当前只显示首个 PDF，入口保留文档集合形式。 */
#include "SourceDocumentView.h"
#include "components/FileCard.h"
#include <QVBoxLayout>
#include <QScrollArea>

SourceDocumentView::SourceDocumentView(QWidget *parent) : QWidget(parent), card_(new FileCard(this))
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
    layout->addWidget(scroll);
    setMinimumWidth(260);
    setDocuments({});
    connect(card_, &QPushButton::clicked, this, &SourceDocumentView::selectionRequested);
}

void SourceDocumentView::setDocuments(const QVector<DocumentItem> &documents)
{
    if (documents.isEmpty())
        card_->setPrompt(QStringLiteral("请选择 PDF"), true);
    else
        card_->setFile(documents.first().kind, documents.first().name);
}

void SourceDocumentView::setBusy(bool busy) { card_->setEnabled(!busy); }
