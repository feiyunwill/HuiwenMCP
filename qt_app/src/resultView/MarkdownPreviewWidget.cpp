/* 直接读取 MD 原文，交给内嵌 CEF 的离线页面渲染。 */
#include "MarkdownPreviewWidget.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QHideEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMoveEvent>
#include <QPointer>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTextCodec>
#include <QTimer>
#include <QVBoxLayout>

#ifdef Q_OS_OPENHARMONY
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_load_handler.h"
#include "include/cef_menu_model.h"
#include "include/cef_request_handler.h"
#include "include/cef_task.h"
#include <unordered_map>
#include "ohos/adapter_c/adapter_c.h"
#include <algorithm>
#include <atomic>
#include <functional>
#include <hilog/log.h>
#include <thread>
#include <vector>

namespace
{
class UiTask final : public CefTask
{
  public:
    explicit UiTask(std::function<void()> action) : action_(std::move(action)) {}
    void Execute() override { action_(); }

  private:
    std::function<void()> action_;
    IMPLEMENT_REFCOUNTING(UiTask);
};

void postCef(std::function<void()> action) { CefPostTask(TID_UI, new UiTask(std::move(action))); }

class CefRuntime
{
  public:
    enum State
    {
        Idle,
        Starting,
        Ready,
        Failed,
        Stopped
    };
    std::atomic<State> state{Idle};
    std::atomic<bool> stopping{false};
    // 仅在 CEF UI 线程访问。
    std::vector<CefRefPtr<CefBrowser>> browsers;
    int creating = 0;
    static CefRuntime &instance()
    {
        static CefRuntime runtime;
        return runtime;
    }
    void start();
    void stop()
    {
        stopping = true;
        if (state == Ready)
            postCef(
                [this]
                {
                    if (browsers.empty() && creating == 0)
                        CefQuitMessageLoop();
                    else
                    {
                        const auto pending = browsers;
                        for (const auto &browser : pending)
                            browser->GetHost()->CloseBrowser(true);
                    }
                });
        if (thread_.joinable())
            thread_.join();
    }
    void closed(CefRefPtr<CefBrowser> browser)
    {
        browsers.erase(std::remove_if(browsers.begin(), browsers.end(),
                                      [browser](const auto &item) { return item->IsSame(browser); }),
                       browsers.end());
        if (stopping && browsers.empty() && creating == 0)
            CefQuitMessageLoop();
    }
    ~CefRuntime() { stop(); }

  private:
    std::thread thread_;
};

class PreviewApp final : public CefApp, public CefBrowserProcessHandler
{
  public:
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
    void OnContextInitialized() override
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "HuiwenCEF", "RUNTIME context initialized");
        auto &runtime = CefRuntime::instance();
        runtime.state = CefRuntime::Ready;
        if (runtime.stopping)
            CefQuitMessageLoop();
    }

  private:
    IMPLEMENT_REFCOUNTING(PreviewApp);
};

void CefRuntime::start()
{
    if (state != Idle)
        return;
    state = Starting;
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [] { instance().stop(); });
    thread_ = std::thread(
        [this]
        {
            const std::vector<std::string> arguments = {
                "huiwen-cef",
                "--use-gl=egl",
                "--enable-features=UseOzonePlatform",
                "--ozone-platform=ohos",
                "--in-process-gpu",
                "--no-zygote",
                "--no-sandbox",
                "--bundle-installation-dir=/data/storage/el1/bundle/entry/resources/resfile/",
                "--user-data-dir=/data/storage/el2/base/cache/huiwen-cef",
                "--lang=zh-CN"};
            std::vector<char *> argv;
            for (const auto &argument : arguments)
                argv.push_back(const_cast<char *>(argument.c_str()));
            CefSettings settings;
            settings.no_sandbox = true;
            CefString(&settings.cache_path) = "/data/storage/el2/base/cache/huiwen-cef";
            if (!CefInitialize(CefMainArgs(static_cast<int>(argv.size()), argv.data()), settings,
                               new PreviewApp, nullptr))
            {
                OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "HuiwenCEF", "RUNTIME initialize failed");
                state = Failed;
                return;
            }
            CefRunMessageLoop();
            state = Stopped;
            CefShutdown();
        });
}

class PreviewClient final : public CefClient,
                            public CefLifeSpanHandler,
                            public CefLoadHandler,
                            public CefRequestHandler,
                            public CefContextMenuHandler
{
  public:
    using Notice = std::function<void(bool, const QString &)>;
    PreviewClient(QString url, QString text, Notice notice)
        : url_(std::move(url)), text_(std::move(text)), notice_(std::move(notice))
    {
    }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
    void OnBeforeContextMenu(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefContextMenuParams>,
                             CefRefPtr<CefMenuModel> model) override
    {
        model->Clear();
    }
    void create(CefWindowHandle parent, const QRect &bounds)
    {
        if (closing_ || CefRuntime::instance().stopping)
            return;
        CefWindowInfo info;
        info.SetAsChild(parent, CefRect(bounds.x(), bounds.y(), bounds.width(), bounds.height()));
        CefBrowserSettings settings;
        settings.background_color = CefColorSetARGB(255, 245, 247, 251);
        ++CefRuntime::instance().creating;
        if (!CefBrowserHost::CreateBrowser(info, this, url_.toStdString(), settings, nullptr, nullptr))
        {
            --CefRuntime::instance().creating;
            notice_(false, QStringLiteral("无法创建 Markdown 预览窗口，请返回文件后重试"));
            if (CefRuntime::instance().stopping && CefRuntime::instance().browsers.empty() &&
                CefRuntime::instance().creating == 0)
                CefQuitMessageLoop();
        }
    }
    void geometry(const QRect &bounds, bool visible, bool active)
    {
        if (!browser_)
            return;
        using namespace EMBEDDED_WINDOW_ADAPTER;
        auto *node = reinterpret_cast<Node *>(browser_->GetHost()->GetWindowHandle());
        if (!node)
            return;
        EWAdapterC::moveNode(node, bounds.x(), bounds.y());
        EWAdapterC::resizeNode(node, bounds.width(), bounds.height());
        EWAdapterC::setNodeVisibility(node, visible && loaded_ && !closing_);
        browser_->GetHost()->WasHidden(!visible || !active);
        browser_->GetHost()->NotifyMoveOrResizeStarted();
        browser_->GetHost()->WasResized();
    }
    void close()
    {
        closing_ = true;
        if (browser_)
        {
            using namespace EMBEDDED_WINDOW_ADAPTER;
            auto *node = reinterpret_cast<Node *>(browser_->GetHost()->GetWindowHandle());
            if (node)
                EWAdapterC::hideNode(node);
            browser_->GetHost()->CloseBrowser(true);
        }
    }
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
    {
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "HuiwenCEF", "PREVIEW browser created id=%{public}d",
                     browser->GetIdentifier());
        browser_ = browser;
        --CefRuntime::instance().creating;
        CefRuntime::instance().browsers.push_back(browser);
        using namespace EMBEDDED_WINDOW_ADAPTER;
        auto *node = reinterpret_cast<Node *>(browser->GetHost()->GetWindowHandle());
        if (node)
            EWAdapterC::hideNode(node);
        browser->GetHost()->SetAccessibilityState(STATE_DISABLED);
        if (closing_ || CefRuntime::instance().stopping)
            close();
    }
    bool DoClose(CefRefPtr<CefBrowser>) override { return false; }
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
    {
        CefRuntime::instance().closed(browser);
        browser_ = nullptr;
    }
    void OnLoadEnd(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, int) override
    {
        if (!frame->IsMain() || closing_ || frame->GetURL().ToString() != url_.toStdString())
            return;
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "HuiwenCEF", "PREVIEW main frame loaded");
        const QByteArray argument = QJsonDocument(QJsonArray{text_}).toJson(QJsonDocument::Compact);
        const QString script =
            QStringLiteral("try { window.HuiwenMarkdown.showPreview(%1[0]); } "
                           "catch (error) { document.getElementById('markdown-body').textContent = "
                           "'预览失败：' + String(error.message); }")
                .arg(QString::fromUtf8(argument));
        frame->ExecuteJavaScript(script.toStdString(), url_.toStdString(), 0);
        loaded_ = true;
        notice_(true, {});
    }
    void OnLoadError(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, ErrorCode code,
                     const CefString &message, const CefString &) override
    {
        if (frame->IsMain() && code != ERR_ABORTED && !closing_)
        {
            OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "HuiwenCEF", "PREVIEW load failed code=%{public}d",
                         static_cast<int>(code));
            notice_(false,
                    QStringLiteral("Markdown 预览加载失败：") + QString::fromStdString(message.ToString()));
        }
    }
    void OnRenderProcessTerminated(CefRefPtr<CefBrowser>, TerminationStatus status) override
    {
        if (closing_)
            return;
        loaded_ = false;
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "HuiwenCEF", "PREVIEW renderer terminated status=%{public}d",
                     static_cast<int>(status));
        notice_(false, QStringLiteral("Markdown 渲染进程已退出（状态 %1），请返回文件后重试")
                           .arg(static_cast<int>(status)));
    }
    bool OnBeforeBrowse(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefRequest> request, bool,
                        bool) override
    {
        // 文档中的链接不能导航到外部页面或任意本地文件。
        return request->GetURL().ToString() != url_.toStdString();
    }
    bool OnBeforePopup(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, const CefString &, const CefString &,
                       WindowOpenDisposition, bool, const CefPopupFeatures &, CefWindowInfo &,
                       CefRefPtr<CefClient> &, CefBrowserSettings &, CefRefPtr<CefDictionaryValue> &,
                       bool *) override
    {
        return true;
    }

  private:
    QString url_;
    QString text_;
    Notice notice_;
    CefRefPtr<CefBrowser> browser_;
    bool closing_ = false;
    bool loaded_ = false;
    IMPLEMENT_REFCOUNTING(PreviewClient);
};
} // namespace
#endif

class MarkdownPreviewWidget::Private
{
  public:
    QLabel *status = nullptr;
    QTimer *timer = nullptr;
    QElapsedTimer waiting;
    QString text;
    QString url;
    bool valid = false;
    bool active = true;
    int generation = 0;
    int bottomInset = 0;
#ifdef Q_OS_OPENHARMONY
    CefRefPtr<PreviewClient> client;
#endif
};

MarkdownPreviewWidget::MarkdownPreviewWidget(QWidget *parent) : QWidget(parent), d_(new Private)
{
    d_->status = new QLabel(this);
    d_->status->setAlignment(Qt::AlignCenter);
    d_->status->setWordWrap(true);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(d_->status);
    d_->timer = new QTimer(this);
    d_->timer->setInterval(100);
    connect(d_->timer, &QTimer::timeout, this, &MarkdownPreviewWidget::openBrowser);
}

MarkdownPreviewWidget::~MarkdownPreviewWidget()
{
    if (window() != this)
        window()->removeEventFilter(this);
    closeBrowser();
}

void MarkdownPreviewWidget::setDocument(const QString &path)
{
    closeBrowser();
    d_->valid = false;
    d_->text.clear();
    d_->url.clear();
    d_->status->show();
    d_->status->clear();
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 16 * 1024 * 1024)
    {
        d_->status->setText(QStringLiteral("无法预览：文件无法读取或超过 16 MiB"));
        return;
    }
    QTextCodec::ConverterState state;
    const QByteArray bytes = file.readAll();
    d_->text = QTextCodec::codecForName("UTF-8")->toUnicode(bytes.constData(), bytes.size(), &state);
    if (state.invalidChars)
    {
        d_->status->setText(QStringLiteral("无法预览：Markdown 文件不是有效的 UTF-8 文本"));
        return;
    }
    QFile page(QStringLiteral(":/markdown/preview.html"));
    if (!page.open(QIODevice::ReadOnly))
    {
        d_->status->setText(QStringLiteral("缺少预览资源，请重新执行 Run 构建"));
        return;
    }
    d_->url = QStringLiteral("data:text/html;base64,") + QString::fromLatin1(page.readAll().toBase64());
    d_->valid = true;
    d_->status->setText(QStringLiteral("正在加载 Markdown 预览…"));
    if (isVisible())
        openBrowser();
}

void MarkdownPreviewWidget::openBrowser()
{
    if (!d_->valid || !isVisible())
        return;
#ifdef Q_OS_OPENHARMONY
    if (d_->client)
    {
        if (d_->waiting.elapsed() > 15000)
        {
            closeBrowser();
            d_->status->show();
            d_->status->setText(QStringLiteral("Markdown 页面加载超时，请返回文件后重试"));
        }
        return;
    }
    auto &runtime = CefRuntime::instance();
    runtime.start();
    if (!d_->timer->isActive())
    {
        d_->waiting.start();
        d_->timer->start();
    }
    if (runtime.state == CefRuntime::Failed || runtime.state == CefRuntime::Stopped)
    {
        d_->timer->stop();
        d_->status->setText(QStringLiteral("浏览器初始化失败，请重新启动应用"));
        return;
    }
    if (d_->waiting.elapsed() > 15000)
    {
        d_->timer->stop();
        d_->status->setText(QStringLiteral("浏览器初始化超时，请返回文件后重试"));
        return;
    }
    if (runtime.state != CefRuntime::Ready)
        return;
    const int generation = d_->generation;
    QPointer<MarkdownPreviewWidget> self(this);
    d_->client = new PreviewClient(d_->url, d_->text,
                                   [self, generation](bool ready, const QString &error)
                                   {
                                       QMetaObject::invokeMethod(
                                           qApp,
                                           [self, generation, ready, error]
                                           {
                                               if (!self || self->d_->generation != generation)
                                                   return;
                                               self->d_->timer->stop();
                                               self->d_->status->setVisible(!ready);
                                               if (!ready)
                                                   self->d_->status->setText(error);
                                               self->syncGeometry();
                                           },
                                           Qt::QueuedConnection);
                                   });
    const auto client = d_->client;
    const auto parent = static_cast<CefWindowHandle>(window()->winId());
    const QRect bounds = browserRect();
    postCef([client, parent, bounds] { client->create(parent, bounds); });
#else
    d_->status->setText(QStringLiteral("当前桌面构建尚未配置 CEF，Markdown 预览在鸿蒙工作区启用"));
#endif
}

void MarkdownPreviewWidget::closeBrowser()
{
    ++d_->generation;
    d_->timer->stop();
#ifdef Q_OS_OPENHARMONY
    if (d_->client)
    {
        const auto client = d_->client;
        if (CefRuntime::instance().state == CefRuntime::Ready)
            postCef([client] { client->close(); });
        d_->client = nullptr;
    }
#endif
}

void MarkdownPreviewWidget::syncGeometry()
{
#ifdef Q_OS_OPENHARMONY
    if (d_->client && CefRuntime::instance().state == CefRuntime::Ready)
    {
        const auto client = d_->client;
        const QRect bounds = browserRect();
        const bool visible =
            !bounds.isEmpty() && isVisible() && window()->isVisible() && !window()->isMinimized();
        const bool active = d_->active;
        postCef([client, bounds, visible, active] { client->geometry(bounds, visible, active); });
    }
#endif
}

QRect MarkdownPreviewWidget::browserRect() const
{
    return QRect(mapTo(window(), QPoint(0, 0)), QSize(width(), qMax(0, height() - d_->bottomInset)));
}

void MarkdownPreviewWidget::setBottomInset(int inset)
{
    d_->bottomInset = qMax(0, inset);
    syncGeometry();
}

void MarkdownPreviewWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    window()->installEventFilter(this);
    openBrowser();
    syncGeometry();
}

void MarkdownPreviewWidget::hideEvent(QHideEvent *event)
{
    closeBrowser();
    QWidget::hideEvent(event);
}

void MarkdownPreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    syncGeometry();
}

void MarkdownPreviewWidget::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    syncGeometry();
}

bool MarkdownPreviewWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == window())
    {
        if (event->type() == QEvent::WindowActivate)
            d_->active = true;
        else if (event->type() == QEvent::WindowDeactivate)
            d_->active = false;
#ifdef Q_OS_OPENHARMONY
        // 先关闭浏览器子节点，再允许 QPA 销毁宿主窗口。
        if (event->type() == QEvent::Close)
            CefRuntime::instance().stop();
#endif
        if (event->type() == QEvent::Resize || event->type() == QEvent::Move ||
            event->type() == QEvent::WindowStateChange || event->type() == QEvent::WindowActivate ||
            event->type() == QEvent::WindowDeactivate)
            syncGeometry();
    }
    return QWidget::eventFilter(watched, event);
}
