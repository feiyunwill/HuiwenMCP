/* 通用文件卡片：完整显示格式图标与可换行文件名，不负责文件读写。 */
#include "FileCard.h"
#include <QPainter>
#include <QTextLayout>
#include <QHash>
#include <algorithm>

namespace
{
// 按任意字符换行，避免没有空格的长英文文件名超出卡片。
qreal layoutName(QTextLayout &layout, int width)
{
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    option.setAlignment(Qt::AlignHCenter);
    layout.setTextOption(option);
    layout.beginLayout();
    qreal height = 0;
    while (true)
    {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(std::max(1, width));
        line.setPosition(QPointF(0, height));
        height += line.height();
    }
    layout.endLayout();
    return height;
}
}

FileCard::FileCard(QWidget *parent) : QPushButton(parent)
{
    setMinimumSize(160, 240);
    setMaximumWidth(400);
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    setCursor(Qt::PointingHandCursor);
}

void FileCard::setFile(const QString &kind, const QString &name)
{
    kind_ = kind.toUpper();
    name_ = name;
    static const QHash<QString, QString> aliases = {
        {"word", "docx"}, {"doc", "docx"}, {"docm", "docx"},
        {"markdown", "md"}, {"xls", "xlsx"}, {"excel", "xlsx"},
        {"ppt", "pptx"}, {"powerpoint", "pptx"}, {"text", "txt"},
        {"png", "photo"}, {"jpg", "photo"}, {"jpeg", "photo"}};
    const QString key = kind.toLower();
    icon_.load(QStringLiteral(":/file-icons/") + aliases.value(key, key) + QStringLiteral(".png"));
    if (icon_.isNull())
        icon_.load(QStringLiteral(":/file-icons/genericfile.png"));
    prompt_.clear();
    setAccessibleName(name);
    setToolTip(name);
    setMinimumHeight(heightForWidth(width()));
    updateGeometry();
    update();
}

void FileCard::setPrompt(const QString &prompt, bool selectable)
{
    kind_.clear();
    name_.clear();
    prompt_ = prompt;
    icon_ = QPixmap();
    setEnabled(selectable);
    setAccessibleName(prompt);
    setToolTip(prompt);
    setMinimumHeight(240);
    updateGeometry();
    update();
}

QSize FileCard::sizeHint() const { return QSize(300, heightForWidth(300)); }

void FileCard::resizeEvent(QResizeEvent *event)
{
    QPushButton::resizeEvent(event);
    setMinimumHeight(heightForWidth(width()));
}

int FileCard::heightForWidth(int width) const
{
    if (kind_.isEmpty())
        return 240;
    QTextLayout text(name_, QFont(QStringLiteral("sans-serif"), 12));
    return std::max(240, 176 + static_cast<int>(layoutName(text, width - 24)) + 24);
}

void FileCard::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor("#667B98"));
    if (kind_.isEmpty())
    {
        if (isEnabled())
        {
            painter.setPen(QPen(QColor("#A7B7CB"), 2, Qt::DashLine));
            painter.drawRoundedRect(rect().adjusted(8, 8, -8, -8), 12, 12);
            painter.setFont(QFont(QStringLiteral("sans-serif"), 36));
            painter.drawText(rect().adjusted(0, 0, 0, -35), Qt::AlignCenter, "+");
        }
        painter.setFont(QFont(QStringLiteral("sans-serif"), 13));
        painter.drawText(rect().adjusted(12, isEnabled() ? 90 : 0, -12, 0), Qt::AlignCenter, prompt_);
        return;
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!icon_.isNull())
    {
        const QPixmap image = icon_.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap((width() - image.width()) / 2, 24 + (128 - image.height()) / 2, image);
    }
    painter.setPen(QColor("#183153"));
    QTextLayout text(name_, QFont(QStringLiteral("sans-serif"), 12));
    layoutName(text, width() - 24);
    text.draw(&painter, QPointF(12, 176));
}
