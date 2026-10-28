/* 工作区顶部功能条：组装文件与滚动菜单，不依赖预览或转换服务。 */
#pragma once
#include <QToolBar>
class FileMenu;

class FunctionBar : public QToolBar
{
  public:
    explicit FunctionBar(QWidget *parent = nullptr);
    FileMenu *fileMenu() const;

  private:
    FileMenu *fileMenu_;
};
