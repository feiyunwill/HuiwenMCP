/*
 * 汇文 MCP 的私有 HNP Python 进程启动器。
 * 定位应用自带解释器，异步任务中执行固定脚本并收集运行结果。
 */
#include "python_runner.h"
#include "hilog/log.h"
#include <QJsonDocument>
#include <QJsonObject>

#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char **environ;

namespace
{
constexpr char kBundleName[] = "cn.com.HuiWenMCP";
constexpr char kPackageDir[] = "huiwen_python.org/huiwen_python_1.0.0";
constexpr char kResultPrefix[] = "HUIWEN_CONVERT_RESULT ";

void CaptureConversionResult(const char *data, size_t size, std::string &pending, bool &skipLine,
                             std::string &result)
{
    for (size_t index = 0; index < size; ++index)
    {
        const char character = data[index];
        if (character == '\n')
        {
            if (!skipLine && pending.size() >= sizeof(kResultPrefix) - 1)
                result = std::move(pending);
            pending.clear();
            skipLine = false;
        }
        else if (!skipLine)
        {
            if (pending.size() < sizeof(kResultPrefix) - 1 && character != kResultPrefix[pending.size()])
            {
                pending.clear();
                skipLine = true;
            }
            else
                pending += character;
        }
    }
}

void LogPythonLine(const std::string &line, LogLevel &level)
{
    if (line.find("[HUIWEN_PY][ERROR]") == 0 || line.find("[HUIWEN_PY][CRITICAL]") == 0 ||
        line.find("Traceback (most recent call last):") == 0)
        level = LOG_ERROR;
    else if (line.find("[HUIWEN_PY][WARNING]") == 0)
        level = LOG_WARN;
    else if (line.find("[HUIWEN_PY][INFO]") == 0 || line.find("HUIWEN_") == 0)
        level = LOG_INFO;
    OH_LOG_Print(LOG_APP, level, 0x3F00, "HuiwenPython", "%{public}s", line.c_str());
}

void ForwardPythonOutput(std::string &pending, LogLevel &level, bool flush)
{
    while (!pending.empty())
    {
        const size_t newline = pending.find('\n');
        if (newline == std::string::npos && pending.size() <= 1024 && !flush)
            break;
        size_t length = newline == std::string::npos ? pending.size() : newline;
        if (length > 1024)
        {
            length = 1024;
            // Split long lines at UTF-8 character boundaries to avoid HiLog truncation.
            while (length > 0 && (static_cast<unsigned char>(pending[length]) & 0xC0) == 0x80)
                --length;
            if (length == 0)
                length = 1024;
        }
        LogPythonLine(pending.substr(0, length), level);
        pending.erase(0, length + (newline == length ? 1 : 0));
    }
}

std::string FindPythonRoot()
{
    const char *home = std::getenv("HNP_PRIVATE_HOME");
    const std::string base = (home != nullptr && home[0] != '\0') ? home : "/data/app";
    const std::array<std::string, 2> roots = {
        base + "/" + kBundleName + "/" + kPackageDir,
        base + "/" + kPackageDir,
    };
    for (const auto &root : roots)
    {
        if (access((root + "/bin/python3").c_str(), X_OK) == 0)
        {
            return root;
        }
    }
    throw std::runtime_error("未找到私有 HNP 解释器；请确认 huiwen_python.hnp 已随 HAP 安装。HNP_PRIVATE_HOME=" + base);
}
std::string RunPythonScript(const char *scriptName, const std::vector<std::string> &paths, const char *successMarker,
                            const char *action, int *exitCode = nullptr)
{
    const std::string root = FindPythonRoot();
    const std::string python = root + "/bin/python3";
    const std::string script = root + "/share/huiwen/" + scriptName;
    OH_LOG_Print(LOG_APP, LOG_INFO, 0x3F00, "HuiwenPython", "PYTHON_START executable=%{public}s script=%{public}s",
                 python.c_str(), script.c_str());
    if (access(script.c_str(), R_OK) != 0)
    {
        throw std::runtime_error("HNP 中缺少脚本: " + script);
    }

    int outputPipe[2];
    if (pipe(outputPipe) != 0)
    {
        throw std::runtime_error("创建 Python 输出管道失败: " + std::string(std::strerror(errno)));
    }
    posix_spawn_file_actions_t actions;
    int code = posix_spawn_file_actions_init(&actions);
    if (code != 0)
    {
        close(outputPipe[0]);
        close(outputPipe[1]);
        throw std::runtime_error("准备 Python 进程失败: " + std::string(std::strerror(code)));
    }
    posix_spawn_file_actions_addclose(&actions, outputPipe[0]);
    posix_spawn_file_actions_adddup2(&actions, outputPipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, outputPipe[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, outputPipe[1]);

    // 参数直接传给固定脚本，不经过 Shell。
    std::vector<char *> args = {const_cast<char *>(python.c_str()), const_cast<char *>("-B"), const_cast<char *>("-u"),
                                const_cast<char *>(script.c_str())};
    for (const auto &path : paths)
    {
        args.push_back(const_cast<char *>(path.c_str()));
    }
    args.push_back(nullptr);
    pid_t child = -1;
    code = posix_spawn(&child, python.c_str(), &actions, nullptr, args.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(outputPipe[1]);
    if (code != 0)
    {
        close(outputPipe[0]);
        throw std::runtime_error("启动 HNP Python 失败: " + std::string(std::strerror(code)));
    }

    std::string output;
    std::string pendingLog;
    std::string pendingResult;
    std::string conversionResult;
    bool skipResultLine = false;
    LogLevel logLevel = LOG_INFO;
    char buffer[512];
    std::string readError;
    for (;;)
    {
        const ssize_t length = read(outputPipe[0], buffer, sizeof(buffer));
        if (length > 0)
        {
            output.append(buffer, static_cast<size_t>(length));
            if (exitCode != nullptr)
                CaptureConversionResult(buffer, static_cast<size_t>(length), pendingResult, skipResultLine,
                                        conversionResult);
            pendingLog.append(buffer, static_cast<size_t>(length));
            ForwardPythonOutput(pendingLog, logLevel, false);
            if (output.size() > 4096)
            {
                output.erase(0, output.size() - 4096);
            }
        }
        else if (length == 0)
        {
            break;
        }
        else if (errno != EINTR)
        {
            readError = std::strerror(errno);
            break;
        }
    }
    close(outputPipe[0]);
    if (exitCode != nullptr)
        CaptureConversionResult("\n", 1, pendingResult, skipResultLine, conversionResult);
    ForwardPythonOutput(pendingLog, logLevel, true);
    int childStatus = 0;
    while (waitpid(child, &childStatus, 0) == -1)
    {
        if (errno != EINTR)
        {
            throw std::runtime_error("等待 Python 退出失败: " + std::string(std::strerror(errno)));
        }
    }
    if (!readError.empty())
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0x3F00, "HuiwenPython", "PYTHON_READ_FAILED errno=%{public}s",
                     readError.c_str());
        throw std::runtime_error("读取 Python 输出失败: " + readError);
    }
    if (!WIFEXITED(childStatus))
    {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0x3F00, "HuiwenPython", "PYTHON_EXIT abnormal status=%{public}d", childStatus);
        throw std::runtime_error("Python 进程未正常退出: " + output);
    }
    OH_LOG_Print(LOG_APP, WEXITSTATUS(childStatus) == 0 ? LOG_INFO : LOG_ERROR, 0x3F00, "HuiwenPython",
                 "PYTHON_EXIT code=%{public}d marker=%{public}d", WEXITSTATUS(childStatus),
                 output.find(successMarker) != std::string::npos);
    if (exitCode != nullptr)
    {
        *exitCode = WEXITSTATUS(childStatus);
        return conversionResult;
    }
    if (WEXITSTATUS(childStatus) != 0)
    {
        throw std::runtime_error(std::string(action) + "退出码 " + std::to_string(WEXITSTATUS(childStatus)) + ": " +
                                 output);
    }
    if (output.find(successMarker) == std::string::npos)
    {
        throw std::runtime_error(std::string(action) + "未返回预期结果: " + output);
    }
    return output;
}
} // namespace

std::string RunPythonSmoke()
{
    return RunPythonScript("python_smoke.py", {}, "HUIWEN_PYTHON_OK", "Python 自检");
}

PdfConversionResult RunPdfConversionResult(const std::string &inputPath, const std::string &outputPath)
{
    if (inputPath.empty() || outputPath.empty())
        return {1001, "转化失败：文件路径不正确，请重新选择 PDF。", {}};
    int exitCode = -1;
    const std::string output = RunPythonScript("convert_pdf.py", {inputPath, outputPath},
                                               "HUIWEN_CONVERT_OK", "PDF 转换", &exitCode);
    if (output.compare(0, sizeof(kResultPrefix) - 1, kResultPrefix) != 0)
        return {2001, "转化失败：转换进程未返回结果，请重新转化。", {}};
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(
        QByteArray::fromStdString(output.substr(sizeof(kResultPrefix) - 1)), &parseError);
    const auto result = document.object();
    const auto code = result.value("code");
    const auto message = result.value("message").toString();
    const auto path = result.value("outputPath").toString();
    if (parseError.error != QJsonParseError::NoError || !document.isObject() || !code.isDouble() ||
        code.toDouble() != code.toInt(-1) || code.toInt(-1) < 0 || message.trimmed().isEmpty() ||
        !result.value("outputPath").isString() || (code.toInt() == 0) != (exitCode == 0) ||
        (code.toInt() == 0 ? path.toStdString() != outputPath : !path.isEmpty()))
        return {2001, "转化失败：转换结果异常，请重新转化。", {}};
    if (code.toInt() == 0 && access(outputPath.c_str(), R_OK) != 0)
        return {2002, "转化失败：生成的 Markdown 无法读取，请重新转化。", {}};
    return {code.toInt(), message.toStdString(), path.toStdString()};
}

std::string RunPdfConversion(const std::string &inputPath, const std::string &outputPath)
{
    const auto result = RunPdfConversionResult(inputPath, outputPath);
    if (result.code != 0)
    {
        throw std::runtime_error("[code=" + std::to_string(result.code) + "] " + result.message);
    }
    return result.outputPath;
}
