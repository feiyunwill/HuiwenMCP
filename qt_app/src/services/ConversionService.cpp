/* 转换服务复用鸿蒙 PythonRunner；桌面以固定脚本参数调用同一 Python API。 */
#include "ConversionService.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QtConcurrent>
#include <exception>
#ifdef Q_OS_OPENHARMONY
#include "python_runner.h"
#endif

ConversionService::ConversionService(QObject *parent) : QObject(parent)
{
    connect(&watcher_, &QFutureWatcherBase::finished, this,
            [this]
            {
                const auto result = watcher_.result();
                if (result.code == 0)
                {
                    qInfo().noquote() << "HUIWEN_OUTPUT stage=convert_success HUIWEN_QT_CONVERT_OK code=" << result.code
                                     << "message=" << result.message << "path=" << result.outputPath
                                     << "bytes=" << QFileInfo(result.outputPath).size();
                    emit succeeded(result.outputPath, result.message);
                }
                else
                {
                    qWarning().noquote() << "HUIWEN_OUTPUT stage=convert_failed HUIWEN_QT_CONVERT_FAILED code=" << result.code
                                        << "message=" << result.message;
                    emit failed(result.code, result.message);
                }
            });
}

void ConversionService::convertPdfToMd(const QString &inputPath, const QString &outputPath)
{
    if (watcher_.isRunning())
        return;
    qInfo().noquote() << "HUIWEN_OUTPUT stage=convert_start HUIWEN_QT_CONVERT_START input=" << inputPath << "output=" << outputPath;
    watcher_.setFuture(QtConcurrent::run(
        [inputPath, outputPath]() -> ConversionResult
        {
            try
            {
#ifdef Q_OS_OPENHARMONY
                const auto result = RunPdfConversionResult(inputPath.toStdString(), outputPath.toStdString());
                if (result.code != 0)
                    return {result.code, QString::fromStdString(result.message), {}};
                // 进程正常退出不等于文件落盘成功，读取最终文件后才通知界面成功。
                const QFileInfo output(outputPath);
                QFile file(outputPath);
                if (!output.isAbsolute() || output.suffix().compare(QStringLiteral("md"), Qt::CaseInsensitive) != 0 ||
                    !output.isFile() || output.size() <= 0 || !file.open(QIODevice::ReadOnly) || file.read(1).isEmpty())
                {
                    qWarning().noquote() << "HUIWEN_OUTPUT stage=verify_failed path=" << outputPath
                                         << "exists=" << output.exists() << "bytes=" << output.size();
                    return {2002, QStringLiteral("转化失败：Markdown 文件未正确生成，请重新转化。"), {}};
                }
                qInfo().noquote() << "HUIWEN_OUTPUT stage=verified path=" << output.absoluteFilePath()
                                 << "directory=" << output.absolutePath() << "bytes=" << output.size();
                return {0, QString::fromStdString(result.message), output.absoluteFilePath()};
#else
                const QString python = qEnvironmentVariable("HUIWEN_PYTHON", "python3");
                const QString script = qEnvironmentVariable("HUIWEN_CONVERT_SCRIPT");
                if (script.isEmpty())
                    return {2001, QStringLiteral("请设置 HUIWEN_CONVERT_SCRIPT 为 convert.py 的绝对路径"), {}};
                QProcess process;
                process.start(python, {"-B", script, inputPath, outputPath});
                if (!process.waitForStarted())
                    return {2001, process.errorString(), {}};
                process.waitForFinished(-1);
                if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 ||
                    !QFileInfo::exists(outputPath))
                {
                    const QString error = QString::fromUtf8(process.readAllStandardError());
                    return {2001, error.isEmpty() ? QStringLiteral("Python 未生成 MD 文件") : error, {}};
                }
                return {0, QStringLiteral("转化成功，Markdown 文件已生成。"), outputPath};
#endif
            }
            catch (const std::exception &error)
            {
                qWarning().noquote() << "HUIWEN_OUTPUT stage=runtime_failed detail=" << error.what();
                return {2001, QStringLiteral("转化失败：转换服务暂不可用，请重新转化；若仍失败，请重启应用。"), {}};
            }
        }));
}
