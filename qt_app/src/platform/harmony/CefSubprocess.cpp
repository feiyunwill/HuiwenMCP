// 与通达适配层约定一致：libcefsimple.so 导出 CefMain，供原生子进程调用。
#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "log/tongda_log.h"
#include <hilog/log.h>

// 包装层在主进程和子进程均使用此日志 ABI，不依赖 Qt 或通达业务目录。
namespace tongda_log
{
__attribute__((visibility("default"))) void Write(int severity, const char *, int, const std::string &message)
{
    OH_LOG_Print(LOG_APP, severity >= 2 ? LOG_ERROR : LOG_INFO, 0xFF00, "HuiwenCEF", "%{public}s",
                 message.c_str());
}
} // namespace tongda_log

extern "C" __attribute__((visibility("default"))) int CefMain(int argc, char *argv[])
{
    const auto commandLine = CefCommandLine::CreateCommandLine();
    commandLine->InitFromArgv(argc, argv);
    const std::string processType = commandLine->GetSwitchValue("type").ToString();
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "HuiwenCEF", "SUBPROCESS start type=%{public}s argc=%{public}d",
                 processType.c_str(), argc);
    // 子进程只执行 Chromium 工作循环；不创建 Qt 应用，也不再次初始化浏览器主进程。
    const int exitCode = CefExecuteProcess(CefMainArgs(argc, argv), nullptr, nullptr);
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "HuiwenCEF", "SUBPROCESS exit type=%{public}s code=%{public}d",
                 processType.c_str(), exitCode);
    return exitCode;
}
