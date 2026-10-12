/* 中部分割线上的悬浮转化按钮；宽度随状态文字调整，不占用整行。 */
#include "ConvertButton.h"
#include <QFontMetrics>
#include <algorithm>

ConvertButton::ConvertButton(QWidget *parent) : QPushButton(QStringLiteral("转化"), parent)
{
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("QPushButton { background: #2968D9; color: white; border: "
                  "none; border-radius: 8px; padding: 2px 12px; }"
                  "QPushButton:disabled { background: #B2BDD0; }");
    setState(false, false);
}

void ConvertButton::setState(bool hasDocument, bool busy)
{
    setEnabled(hasDocument && !busy);
    setText(busy ? QStringLiteral("转化中…") : QStringLiteral("转化"));
    const QFontMetrics metrics = fontMetrics();
    setFixedSize(std::max(busy ? 128 : 96, metrics.horizontalAdvance(text()) + 24),
                 std::max(40, metrics.height() + 4));
}
