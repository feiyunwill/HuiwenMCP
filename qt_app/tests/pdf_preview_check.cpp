/* 无 QtTest 依赖的桌面回归检查：真实渲染、滚动、缩放、切换与忙碌状态。 */
#include "sourceView/PdfPreviewWidget.h"
#include "sourceView/SourceDocumentView.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QPushButton>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QThread>
#include <cstdlib>
#include <functional>
#include <iostream>

namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

void waitFor(const std::function<bool()> &condition)
{
    QElapsedTimer timer;
    timer.start();
    do
    {
        QApplication::processEvents();
        if (condition())
            return;
        QThread::msleep(10);
    } while (timer.elapsed() < 10000);
    require(false, "等待预览结果超时");
}

void writePdf(const QString &path, const QByteArray &firstColor)
{
    const QByteArray first = firstColor + " rg 0 0 200 300 re f\n";
    const QByteArray second = "0 0 1 rg 0 0 200 300 re f\n";
    const QList<QByteArray> objects = {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R 5 0 R] /Count 2 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 300] /Resources << >> /Contents 4 0 R >>",
        "<< /Length " + QByteArray::number(first.size()) + " >>\nstream\n" + first + "endstream",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 300] /Resources << >> /Contents 6 0 R >>",
        "<< /Length " + QByteArray::number(second.size()) + " >>\nstream\n" + second + "endstream"};
    QByteArray pdf = "%PDF-1.4\n";
    QList<int> offsets;
    for (int i = 0; i < objects.size(); ++i)
    {
        offsets.append(pdf.size());
        pdf += QByteArray::number(i + 1) + " 0 obj\n" + objects[i] + "\nendobj\n";
    }
    const int xref = pdf.size();
    pdf += "xref\n0 7\n0000000000 65535 f \n";
    for (int offset : offsets)
        pdf += QByteArray::number(offset).rightJustified(10, '0') + " 00000 n \n";
    pdf +=
        "trailer\n<< /Size 7 /Root 1 0 R >>\nstartxref\n" + QByteArray::number(xref) + "\n%%EOF\n";
    QFile file(path);
    require(file.open(QIODevice::WriteOnly), "无法创建 PDF 样本");
    require(file.write(pdf) == pdf.size(), "PDF 样本写入失败");
}

bool hasColor(PdfPreviewWidget *view, QColor color)
{
    const QImage image = view->viewport()->grab().toImage();
    const qreal ratio = image.devicePixelRatio();
    const QPoint point(static_cast<int>(view->viewport()->width() / 2 * ratio),
                       static_cast<int>(60 * ratio));
    return image.rect().contains(point) && image.pixelColor(point) == color;
}
} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir directory;
    require(directory.isValid(), "临时目录创建失败");
    const QString first = directory.filePath(QStringLiteral("中文红色.pdf"));
    const QString second = directory.filePath(QStringLiteral("绿色.pdf"));
    writePdf(first, "1 0 0");
    writePdf(second, "0 1 0");
    SourceDocumentView source;
    source.resize(480, 320);
    source.show();
    auto *view = source.findChild<PdfPreviewWidget *>();
    QPushButton *button = nullptr;
    for (auto *candidate : source.findChildren<QPushButton *>())
        if (candidate->text() == QStringLiteral("预览"))
            button = candidate;
    require(view && button && !button->isVisible(), "未导入时预览入口不应显示");
    source.setDocuments({{"pdf", "中文红色.pdf", first, {}}});
    require(button->isVisible() && !view->isVisible(), "导入后应保留卡片并显示预览按钮");
    source.setBusy(true);
    require(!button->isEnabled(), "忙碌时预览入口应禁用");
    source.setBusy(false);
    button->click();
    waitFor([&] { return hasColor(view, Qt::red); });
    const int oldRange = view->verticalScrollBar()->maximum();
    require(oldRange > 0, "多页 PDF 应可滚动");
    source.resize(720, 320);
    waitFor([&]
            { return view->verticalScrollBar()->maximum() > oldRange && hasColor(view, Qt::red); });
    require(view->verticalScrollBar()->singleStep() == 40, "PDF 滚轮步长应使用文档阅读尺度");
    int reports = 0;
    int page = 0;
    double fraction = 0;
    QObject::connect(view, &PdfPreviewWidget::scrollPositionChanged,
                     [&](int current, double part, double)
                     {
                         ++reports;
                         page = current;
                         fraction = part;
                     });
    view->setScrollPosition(2, 0.25, 0.5);
    require(reports == 0, "同步带动的 PDF 滚动不能立即反向回传");
    view->publishScrollPosition();
    require(page == 2 && qAbs(fraction - 0.25) < 0.01, "PDF 应按页码和页内比例定位");
    view->setScrollPosition(0, 0, 0.5);
    require(qAbs(view->verticalScrollBar()->value() - view->verticalScrollBar()->maximum() * 0.5) <= 1,
            "无页码时应按总进度定位 PDF");
    view->verticalScrollBar()->setValue(view->verticalScrollBar()->maximum());
    waitFor([&] { return hasColor(view, Qt::blue); });
    button->click();
    require(!view->isVisible(), "返回文件后应显示卡片");
    button->click();
    source.setDocuments({{"pdf", "绿色.pdf", second, {}}});
    require(!view->isVisible(), "更换文件应回到卡片");
    button->click();
    waitFor([&] { return hasColor(view, Qt::green); });
    require(view->verticalScrollBar()->value() == 0, "更换文件后应从第一页开始");
    std::cout << "PDF_PREVIEW_CHECK_OK" << std::endl;
}
