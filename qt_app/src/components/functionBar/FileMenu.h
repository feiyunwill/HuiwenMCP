/* 文件菜单只维护界面状态并发出请求，文件选择、目录打开和预览由工作区协调。 */
#pragma once
#include "SelectionMenu.h"
class QAction;

class FileMenu : public SelectionMenu
{
    Q_OBJECT
  public:
    explicit FileMenu(QWidget *parent = nullptr);
    void setDocumentState(bool hasDocument, bool busy);

  signals:
    void previewModeRequested(bool preview);
    void replacementRequested();
    void directoryRequested(bool output);

  private:
    QAction *placeholderAction_;
    QAction *previewAction_;
    QAction *inputDirectoryAction_;
    QAction *outputDirectoryAction_;
    QAction *replacementAction_;
};
