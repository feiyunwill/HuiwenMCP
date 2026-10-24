/* 鸿蒙沙箱适配：独立偏移读取授权 fd，原子保存输入文件，复用跨平台目录校验。 */
#include "harmony_sandbox.h"
#ifdef Q_OS_OPENHARMONY
#include "sandbox/SandboxStore.h"
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QSaveFile>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
void fail(const QString &message)
{
    throw std::runtime_error(message.toStdString());
}
}

HarmonySandboxDocument HarmonySandbox::importDocument(const QString &filesDir, const QString &name,
                                                      int sourceFd, const QString &kind, const QString &mode)
{
    struct stat sourceInfo {};
    if (sourceFd < 0 || fstat(sourceFd, &sourceInfo) != 0 || !S_ISREG(sourceInfo.st_mode))
        fail(QStringLiteral("无法读取所选文件"));
    auto document = SandboxStore::createDocument(filesDir, name, kind, mode);
    try
    {
        QSaveFile target(document.inputPath);
        if (!target.open(QIODevice::WriteOnly))
            fail(QStringLiteral("无法创建沙箱文件：") + target.errorString());
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
        if (!target.commit())
            fail(QStringLiteral("保存沙箱文件失败：") + target.errorString());
        SandboxStore::markUsed(filesDir, document.taskPath);
        document.lastUsedAt = QFileInfo(document.taskPath).lastModified().toMSecsSinceEpoch();
    }
    catch (...)
    {
        SandboxStore::discardDocument(document);
        throw;
    }
    qInfo().noquote() << "HUIWEN_OUTPUT stage=import_success platform=harmony task=" << document.taskPath
                      << "mode=" << mode << "input=" << document.inputPath
                      << "output=" << document.outputPath;
    return document;
}

QString HarmonySandbox::validateOutput(const QString &filesDir, const QString &path)
{
    const QString validated = SandboxStore::validateOutput(filesDir, path);
    const QFileInfo output(validated);
    qInfo().noquote() << "HUIWEN_OUTPUT stage=locate_verified platform=harmony directory=" << output.absolutePath()
                     << "file=" << output.fileName() << "bytes=" << output.size();
    return validated;
}
#endif
