/* 浅灰无边框功能条；滚动选项暂时只记录菜单选中状态，不切换实际滚动策略。 */
#include "FunctionBar.h"
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QPainter>
#include <QToolButton>

namespace
{
// 保留 QMenu 的键盘和互斥选择行为，仅把选中标记绘制到菜单项右侧。
class ScrollSettingsMenu final : public QMenu
{
  public:
    using QMenu::QMenu;

  protected:
    void paintEvent(QPaintEvent *event) override
    {
        QMenu::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(QStringLiteral("#3983F7")));
        for (auto *action : actions())
        {
            if (action->isVisible() && action->isChecked())
            {
                const QRect row = actionGeometry(action);
                painter.drawEllipse(QPointF(row.right() - 24, row.center().y()), 7, 7);
            }
        }
    }
};
} // namespace

FunctionBar::FunctionBar(QWidget *parent) : QToolBar(parent)
{
    setObjectName(QStringLiteral("workspaceFunctionBar"));
    setMovable(false);
    setFloatable(false);
    setContextMenuPolicy(Qt::PreventContextMenu);
    setFixedHeight(72);
    setStyleSheet(
        QStringLiteral("QToolBar#workspaceFunctionBar { background: #F2F3F5; border: none; padding: 0 16px; }"
                       "QToolBar#workspaceFunctionBar QToolButton { background: transparent; border: none; "
                       "padding: 12px 20px; color: #34445C; font-size: 28px; font-weight: normal; }"
                       "QToolBar#workspaceFunctionBar QToolButton:hover { background: #E5E8ED; }"
                       "QToolBar#workspaceFunctionBar QToolButton:pressed { background: #DCE1E8; }"
                       "QToolBar#workspaceFunctionBar QToolButton::menu-indicator { image: none; }"));

    auto *button = new QToolButton(this);
    button->setText(QStringLiteral("滚动设置"));
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setCursor(Qt::PointingHandCursor);
    button->setPopupMode(QToolButton::InstantPopup);
    auto *menu = new ScrollSettingsMenu(button);
    menu->setStyleSheet(QStringLiteral("QMenu { background: #F7F8FA; border: none; padding: 8px; "
                                       "color: #34445C; font-size: 28px; font-weight: normal; }"
                                       "QMenu::item { min-height: 40px; padding: 16px 64px 16px 20px; }"
                                       "QMenu::item:selected { background: #E5E8ED; }"
                                       "QMenu::indicator { width: 0px; height: 0px; image: none; }"));
    auto *modes = new QActionGroup(menu);
    modes->setExclusive(true);
    auto *synchronized = menu->addAction(QStringLiteral("同步滚动"));
    auto *oneWay = menu->addAction(QStringLiteral("单向滚动"));
    for (auto *action : {synchronized, oneWay})
    {
        action->setCheckable(true);
        modes->addAction(action);
    }
    synchronized->setChecked(true);
    button->setMenu(menu);
    addWidget(button);
}
