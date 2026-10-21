/* Qt 的 PDF 转 MD API：异步调用固定 Python 转换入口并返回文件路径。 */
#pragma once
#include <QFutureWatcher>
#include <QObject>

struct ConversionResult
{
    int code;
    QString message;
    QString outputPath;
};

class ConversionService : public QObject
{
    Q_OBJECT
  public:
    explicit ConversionService(QObject *parent = nullptr);
    void convertPdfToMd(const QString &inputPath, const QString &outputPath);
  signals:
    void succeeded(const QString &path, const QString &message);
    void failed(int code, const QString &message);

  private:
    QFutureWatcher<ConversionResult> watcher_;
};
