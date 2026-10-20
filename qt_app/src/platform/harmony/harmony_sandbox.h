#pragma once
#include <QString>

#ifdef Q_OS_OPENHARMONY
struct HarmonySandboxDocument
{
    QString inputPath;
    QString outputPath;
};

class HarmonySandbox
{
  public:
    static HarmonySandboxDocument importDocument(const QString &filesDir, const QString &name, int sourceFd,
                                                  const QString &kind, bool forConversion);
    static QString validateOutput(const QString &filesDir, const QString &path);
};
#endif
