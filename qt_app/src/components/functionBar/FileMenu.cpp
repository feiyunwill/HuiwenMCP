/* 当前仅接入转化模式；查看方式与业务模式分别维护，非当前模式不提供更换文件。 */
#include "FileMenu.h"
#include <QAction>
#include <QActionGroup>

FileMenu::FileMenu(QWidget *parent) : SelectionMenu(parent)
{
    auto *viewMenu = new SelectionMenu(this);
    viewMenu->setTitle(QStringLiteral("查看模式"));
    addMenu(viewMenu);
    auto *views = new QActionGroup(viewMenu);
    views->setExclusive(true);
    placeholderAction_ = viewMenu->addAction(QStringLiteral("占位图模式"));
    previewAction_ = viewMenu->addAction(QStringLiteral("预览模式"));
    auto *editView = viewMenu->addAction(QStringLiteral("编辑模式"));
    for (auto *action : {placeholderAction_, previewAction_, editView})
    {
        action->setCheckable(true);
        views->addAction(action);
    }
    previewAction_->setChecked(true);
    // TODO: 接入编辑视图；当前仅提供占位图和只读预览。
    editView->setEnabled(false);
    editView->setToolTip(QStringLiteral("编辑视图尚未接入"));
    connect(placeholderAction_, &QAction::triggered, this,
            [this]
            {
                setPreviewMode(false);
            });
    connect(previewAction_, &QAction::triggered, this,
            [this]
            {
                setPreviewMode(true);
            });

    auto *directoryMenu = new SelectionMenu(this);
    directoryMenu->setTitle(QStringLiteral("文件目录"));
    addMenu(directoryMenu);
    inputDirectoryAction_ = directoryMenu->addAction(QStringLiteral("输入目录"));
    outputDirectoryAction_ = directoryMenu->addAction(QStringLiteral("输出目录"));
    connect(inputDirectoryAction_, &QAction::triggered, this,
            [this]
            {
                emit directoryRequested(false);
            });
    connect(outputDirectoryAction_, &QAction::triggered, this,
            [this]
            {
                emit directoryRequested(true);
            });

    auto *modeMenu = new SelectionMenu(this);
    modeMenu->setTitle(QStringLiteral("切换模式"));
    addMenu(modeMenu);
    auto *modes = new QActionGroup(modeMenu);
    modes->setExclusive(true);
    auto *previewMode = modeMenu->addAction(QStringLiteral("预览模式"));
    auto *editMode = modeMenu->addAction(QStringLiteral("编辑模式"));
    // TODO: 接入预览、编辑业务模式切换；暂不改变工作区或创建其他模式的任务。
    previewMode->setEnabled(false);
    editMode->setEnabled(false);
    previewMode->setToolTip(QStringLiteral("业务模式切换尚未接入"));
    editMode->setToolTip(QStringLiteral("业务模式切换尚未接入"));
    auto *conversionMenu = new SelectionMenu(modeMenu);
    conversionMenu->setTitle(QStringLiteral("转化模式"));
    auto *conversionMode = modeMenu->addMenu(conversionMenu);
    for (auto *action : {previewMode, editMode, conversionMode})
    {
        action->setCheckable(true);
        modes->addAction(action);
    }
    conversionMode->setChecked(true);
    // 仅当前业务模式拥有更换入口；未来模式切换时再同步菜单归属。
    replacementAction_ = conversionMenu->addAction(QStringLiteral("更换文件"));
    connect(replacementAction_, &QAction::triggered, this, &FileMenu::replacementRequested);
    setDocumentState(false, false);
}

// 菜单点击与程序切换共用入口，只改变查看方式，不改变转化业务模式。
void FileMenu::setPreviewMode(bool preview)
{
    (preview ? previewAction_ : placeholderAction_)->setChecked(true);
    emit previewModeRequested(preview);
}

void FileMenu::setDocumentState(bool hasDocument, bool busy)
{
    placeholderAction_->setEnabled(!busy);
    previewAction_->setEnabled(hasDocument && !busy);
    inputDirectoryAction_->setEnabled(hasDocument);
    outputDirectoryAction_->setEnabled(hasDocument);
    replacementAction_->setEnabled(!busy);
}
