/* 工作区顶部功能条；本阶段只提供滚动设置菜单，不依赖预览或转换服务。 */
#pragma once
#include <QToolBar>

class FunctionBar : public QToolBar
{
  public:
    explicit FunctionBar(QWidget *parent = nullptr);
};
