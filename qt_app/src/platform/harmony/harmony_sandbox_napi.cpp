#include "harmony_sandbox.h"
#ifdef Q_OS_OPENHARMONY
#include <napi/native_api.h>
#include <climits>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <unistd.h>

namespace
{
struct ImportWork
{
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    QString root;
    QString name;
    QString kind;
    int fd = -1;
    bool forConversion = false;
    HarmonySandboxDocument document;
    std::string error;

    ~ImportWork()
    {
        if (fd >= 0)
            close(fd);
    }
};

bool readString(napi_env env, napi_value value, QString &result)
{
    napi_valuetype type;
    size_t length = 0;
    if (napi_typeof(env, value, &type) != napi_ok || type != napi_string ||
        napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok || length == 0 || length > 4096)
        return false;
    QByteArray bytes(static_cast<int>(length + 1), '\0');
    size_t copied = 0;
    if (napi_get_value_string_utf8(env, value, bytes.data(), bytes.size(), &copied) != napi_ok ||
        copied != length || bytes.indexOf('\0') != static_cast<int>(length))
        return false;
    result = QString::fromUtf8(bytes.constData(), static_cast<int>(length));
    return true;
}

napi_value stringValue(napi_env env, const QString &text)
{
    const auto bytes = text.toUtf8();
    napi_value value = nullptr;
    napi_create_string_utf8(env, bytes.constData(), bytes.size(), &value);
    return value;
}

void reject(napi_env env, napi_deferred deferred, const std::string &message)
{
    napi_value error;
    napi_create_error(env, stringValue(env, QStringLiteral("HARMONY_SANDBOX_IMPORT_FAILED")),
                      stringValue(env, QString::fromStdString(message)), &error);
    napi_reject_deferred(env, deferred, error);
}

void executeImport(napi_env, void *data)
{
    auto *task = static_cast<ImportWork *>(data);
    try
    {
        task->document = HarmonySandbox::importDocument(task->root, task->name, task->fd, task->kind,
                                                        task->forConversion);
    }
    catch (const std::exception &error)
    {
        task->error = error.what();
    }
}

void completeImport(napi_env env, napi_status status, void *data)
{
    std::unique_ptr<ImportWork> task(static_cast<ImportWork *>(data));
    if (status == napi_ok && task->error.empty())
    {
        napi_value result;
        napi_create_object(env, &result);
        napi_set_named_property(env, result, "inputPath", stringValue(env, task->document.inputPath));
        napi_set_named_property(env, result, "outputPath", stringValue(env, task->document.outputPath));
        napi_resolve_deferred(env, task->deferred, result);
    }
    else
        reject(env, task->deferred, task->error.empty() ? "沙箱导入任务未完成" : task->error);
    napi_delete_async_work(env, task->work);
}

napi_value importDocument(napi_env env, napi_callback_info info)
{
    size_t count = 5;
    napi_value values[5];
    auto task = std::make_unique<ImportWork>();
    double fd = -1;
    if (napi_get_cb_info(env, info, &count, values, nullptr, nullptr) != napi_ok || count != 5 ||
        !readString(env, values[0], task->root) || !readString(env, values[1], task->name) ||
        napi_get_value_double(env, values[2], &fd) != napi_ok || !(fd >= 0 && fd <= INT_MAX) ||
        fd != static_cast<int>(fd) || !readString(env, values[3], task->kind) ||
        napi_get_value_bool(env, values[4], &task->forConversion) != napi_ok)
    {
        napi_throw_type_error(env, "HARMONY_SANDBOX_INVALID_ARGUMENT", "需要沙箱目录、文件名、有效文件描述符、文档类型及转换标记");
        return nullptr;
    }
    task->fd = fcntl(static_cast<int>(fd), F_DUPFD_CLOEXEC, 0);
    if (task->fd < 0)
    {
        napi_throw_error(env, "HARMONY_SANDBOX_INVALID_FD", "所选文件已无法读取，请重新选择");
        return nullptr;
    }
    napi_value promise;
    if (napi_create_promise(env, &task->deferred, &promise) != napi_ok)
    {
        napi_throw_error(env, nullptr, "无法创建沙箱导入任务");
        return nullptr;
    }
    const auto resource = stringValue(env, QStringLiteral("HarmonySandboxImport"));
    napi_status status = napi_create_async_work(env, nullptr, resource, executeImport, completeImport, task.get(),
                                                &task->work);
    if (status == napi_ok)
        status = napi_queue_async_work(env, task->work);
    if (status != napi_ok)
    {
        reject(env, task->deferred, "无法启动沙箱导入任务，请重试");
        if (task->work != nullptr)
            napi_delete_async_work(env, task->work);
    }
    else
        task.release();
    return promise;
}

napi_value validateOutput(napi_env env, napi_callback_info info)
{
    size_t count = 2;
    napi_value values[2];
    QString root, path;
    if (napi_get_cb_info(env, info, &count, values, nullptr, nullptr) != napi_ok || count != 2 ||
        !readString(env, values[0], root) || !readString(env, values[1], path))
    {
        napi_throw_type_error(env, "HARMONY_SANDBOX_INVALID_ARGUMENT", "需要沙箱目录和输出文件路径");
        return nullptr;
    }
    try
    {
        return stringValue(env, HarmonySandbox::validateOutput(root, path));
    }
    catch (const std::exception &error)
    {
        napi_throw_error(env, "HARMONY_SANDBOX_OUTPUT_INVALID", error.what());
        return nullptr;
    }
}
}

void InitHarmonySandbox(napi_env env, napi_value exports)
{
    napi_property_descriptor methods[] = {
        {"harmonyImportDocument", nullptr, importDocument, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"harmonyValidateOutput", nullptr, validateOutput, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, 2, methods);
}
#endif
