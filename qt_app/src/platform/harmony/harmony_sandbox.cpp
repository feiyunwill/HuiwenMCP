#include "harmony_sandbox.h"
#ifdef Q_OS_OPENHARMONY
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
std::mutex importMutex;

void fail(const QString &message)
{
    throw std::runtime_error(message.toStdString());
}

QString sandboxRoot(const QString &filesDir)
{
    const QFileInfo root(filesDir);
    if (filesDir.contains(QChar::Null) || !root.isAbsolute() || !root.isDir() || root.canonicalFilePath().isEmpty())
        fail(QStringLiteral("应用沙箱目录无效"));
    return root.canonicalFilePath();
}

void ensureDirectory(const QString &path, const QString &allowedRoot)
{
    const QString parent = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(parent) || QFileInfo(parent).canonicalFilePath() != allowedRoot ||
        !QDir().mkpath(path) || !QFileInfo(path).canonicalFilePath().startsWith(allowedRoot + "/"))
        fail(QStringLiteral("无法创建应用沙箱目录：") + path);
}
}

HarmonySandboxDocument HarmonySandbox::importDocument(const QString &filesDir, const QString &name, int sourceFd,
                                                       const QString &kind, bool forConversion)
{
    const QString root = sandboxRoot(filesDir);
    if (name.contains('/') || name.contains('\\') || name.contains(QChar::Null))
        fail(QStringLiteral("所选文件名无效"));
    const QFileInfo selected(name);
    const QString extension = selected.suffix().toLower();
    if (!(kind == "pdf" ? extension == "pdf" : kind == "md" && (extension == "md" || extension == "markdown")) ||
        (forConversion && kind != "pdf"))
        fail(QStringLiteral("请选择对应格式的文件"));
    QString stem = selected.completeBaseName();
    stem.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|\\x{0000}-\\x{001f}]")), "_");
    stem = stem.left(80).trimmed();
    if (!stem.isEmpty() && stem.at(stem.size() - 1).isHighSurrogate())
        stem.chop(1);
    if (stem.isEmpty() || stem == "." || stem == "..")
        stem = QStringLiteral("document");
    const QString day = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    const QString inputDir = QDir(filesDir).filePath("inputs/" + day);
    const QString outputDir = QDir(filesDir).filePath("outputs/" + day);
    ensureDirectory(inputDir, root + "/inputs");
    if (forConversion)
        ensureDirectory(outputDir, root + "/outputs");

    struct stat sourceInfo {};
    if (sourceFd < 0 || fstat(sourceFd, &sourceInfo) != 0 || !S_ISREG(sourceInfo.st_mode))
        fail(QStringLiteral("无法读取所选文件"));
    QFile target;
    HarmonySandboxDocument document;
    {
        std::lock_guard<std::mutex> lock(importMutex);
        for (int index = 1; index <= 10000; ++index)
        {
            const QString suffix = index == 1 ? QString() : QStringLiteral(" (%1)").arg(index);
            document.inputPath = inputDir + "/" + stem + suffix + "." + extension;
            document.outputPath = forConversion ? outputDir + "/" + stem + suffix + ".md" : QString();
            if (QFileInfo::exists(document.inputPath) ||
                (!document.outputPath.isEmpty() && QFileInfo::exists(document.outputPath)))
                continue;
            target.setFileName(document.inputPath);
            if (target.open(QIODevice::WriteOnly | QIODevice::NewOnly))
                break;
            if (!QFileInfo::exists(document.inputPath))
                fail(QStringLiteral("无法创建沙箱文件：") + target.errorString());
        }
        if (!target.isOpen())
            fail(QStringLiteral("同名文件过多，请更换文件名"));
    }
    try
    {
        char buffer[64 * 1024];
        off_t sourceOffset = 0;
        for (;;)
        {
            const ssize_t length = pread(sourceFd, buffer, sizeof(buffer), sourceOffset);
            if (length < 0)
            {
                if (errno == EINTR)
                    continue;
                fail(QStringLiteral("读取所选文件失败：") + QString::fromLocal8Bit(std::strerror(errno)));
            }
            if (length == 0)
                break;
            qint64 offset = 0;
            while (offset < length)
            {
                const qint64 written = target.write(buffer + offset, length - offset);
                if (written <= 0)
                    fail(QStringLiteral("写入沙箱文件失败：") + target.errorString());
                offset += written;
            }
            sourceOffset += length;
        }
        if (!target.flush())
            fail(QStringLiteral("保存沙箱文件失败：") + target.errorString());
    }
    catch (...)
    {
        target.close();
        target.remove();
        throw;
    }
    target.close();
    qInfo().noquote() << "HUIWEN_OUTPUT stage=import_success platform=harmony input=" << document.inputPath
                     << "output=" << document.outputPath;
    return document;
}

QString HarmonySandbox::validateOutput(const QString &filesDir, const QString &path)
{
    const QString root = sandboxRoot(filesDir);
    const QFileInfo output(path);
    const QString canonical = output.canonicalFilePath();
    QFile file(path);
    if (path.contains(QChar::Null) || path.contains('\\') || !output.isAbsolute() ||
        QDir::cleanPath(path) != path || !canonical.startsWith(root + "/outputs/") ||
        output.suffix().compare(QStringLiteral("md"), Qt::CaseInsensitive) != 0 ||
        !output.isFile() || output.size() <= 0 || !file.open(QIODevice::ReadOnly) || file.read(1).isEmpty())
        fail(QStringLiteral("Markdown 文件不属于本应用输出目录，或文件不存在、为空、无法读取"));
    qInfo().noquote() << "HUIWEN_OUTPUT stage=locate_verified platform=harmony directory=" << output.absolutePath()
                     << "file=" << output.fileName() << "bytes=" << output.size();
    return output.absoluteFilePath();
}
#endif
