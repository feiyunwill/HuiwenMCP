/* Qt Widgets 工作区入口：默认最大化并保留系统边框，窗口实际曝光后通知鸿蒙关闭原首页。 */
#include "workspace/ConversionWindow.h"
#include <QApplication>
#include <QIcon>
#include <QTimer>
#include <QWindow>
#ifdef Q_OS_OPENHARMONY
#include "platform/harmony/qt_bridge.h"
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_OPENHARMONY
    // 只让悬浮按钮创建原生子节点，避免连带提升左右文档区域的层级。
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif
    QApplication app(argc, argv);
    app.setApplicationName("HuiwenMCP");
    app.setApplicationDisplayName(QStringLiteral("汇文MCP"));
    app.setWindowIcon(QIcon(QStringLiteral(":/branding/app_icon.png")));
    ConversionWindow window;
    window.showMaximized();
#ifdef Q_OS_OPENHARMONY
    QTimer readyTimer;
    readyTimer.setInterval(100);
    QObject::connect(&readyTimer, &QTimer::timeout, &window,
                     [&]
                     {
                         if (window.windowHandle() && window.windowHandle()->isExposed() &&
                             QtBridge::instance().request("ready"))
                             readyTimer.stop();
                     });
    readyTimer.start();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [] { QtBridge::instance().request("close"); });
#endif
    return app.exec();
}
