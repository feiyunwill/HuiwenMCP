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
                if (result.second.isEmpty())
                {
                    qInfo().noquote() << "HUIWEN_OUTPUT stage=convert_success HUIWEN_QT_CONVERT_OK path=" << result.first
                                     << "bytes=" << QFileInfo(result.first).size();
                    emit succeeded(result.first);
                }
                else
                {
                    qWarning().noquote() << "HUIWEN_OUTPUT stage=convert_failed HUIWEN_QT_CONVERT_FAILED error=" << result.second;
                    emit failed(result.second);
                }
            });
}

void ConversionService::convertPdfToMd(const QString &inputPath, const QString &outputPath)
{
    if (watcher_.isRunning())
        return;
    qInfo().noquote() << "HUIWEN_OUTPUT stage=convert_start HUIWEN_QT_CONVERT_START input=" << inputPath << "output=" << outputPath;
    watcher_.setFuture(QtConcurrent::run(
        [inputPath, outputPath]() -> QPair<QString, QString>
        {
            try
            {
#ifdef Q_OS_OPENHARMONY
                const QString convertedPath = QString::fromStdString(
                    RunPdfConversion(inputPath.toStdString(), outputPath.toStdString()));
                if (convertedPath != outputPath)
                    return {{}, QStringLiteral("转换返回的路径与预期输出路径不一致")};
                // 进程正常退出不等于文件落盘成功，读取最终文件后才通知界面成功。
                const QFileInfo output(outputPath);
                QFile file(outputPath);
                if (!output.isAbsolute() || output.suffix().compare(QStringLiteral("md"), Qt::CaseInsensitive) != 0 ||
                    !output.isFile() || output.size() <= 0 || !file.open(QIODevice::ReadOnly) || file.read(1).isEmpty())
                {
                    qWarning().noquote() << "HUIWEN_OUTPUT stage=verify_failed path=" << outputPath
                                         << "exists=" << output.exists() << "bytes=" << output.size();
                    return {{}, QStringLiteral("Markdown 输出文件不存在、为空或不可读取：") + outputPath};
                }
                qInfo().noquote() << "HUIWEN_OUTPUT stage=verified path=" << output.absoluteFilePath()
                                 << "directory=" << output.absolutePath() << "bytes=" << output.size();
                return {output.absoluteFilePath(), {}};
#else
                const QString python = qEnvironmentVariable("HUIWEN_PYTHON", "python3");
                const QString script = qEnvironmentVariable("HUIWEN_CONVERT_SCRIPT");
                if (script.isEmpty())
                    return {{}, QStringLiteral("请设置 HUIWEN_CONVERT_SCRIPT 为 convert.py 的绝对路径")};
                QProcess process;
                process.start(python, {"-B", script, inputPath, outputPath});
                if (!process.waitForStarted())
                    return {{}, process.errorString()};
                process.waitForFinished(-1);
                if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 ||
                    !QFileInfo::exists(outputPath))
                {
                    const QString error = QString::fromUtf8(process.readAllStandardError());
                    return {{}, error.isEmpty() ? QStringLiteral("Python 未生成 MD 文件") : error};
                }
                return {outputPath, {}};
#endif
            }
            catch (const std::exception &error)
            {
                return {{}, QString::fromUtf8(error.what())};
            }
        }));
}
