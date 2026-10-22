"""从项目内 CEF SDK 准备汇文预览使用的 HAR 和资源，不重编 CEF 或 Qt。"""
import json
from prepare_qt import ROOT, copy_changed


def prepare():
    sdk = ROOT / "third_party/cef/sdk"
    source = sdk / "web_engine"
    target = ROOT / ".native-tools/cef-web-engine"
    required = [sdk / "include/cef_app.h", source / "Index.ets",
                source / "src/main/resources/resfile/icudtl.dat"]
    required += [ROOT / "entry/libs/arm64-v8a" / name
                 for name in ("libcef.so", "libadapter.so", "libadapter_c.so", "libc++_shared.so")]
    for path in required:
        if not path.is_file():
            raise SystemExit(f"缺少 CEF 预览依赖，请先准备项目内 CEF SDK：{path}")
    # 原始 SDK 保持不变；HAR 副本使用本应用的模块清单和原生类型声明。
    skip = {"build", ".hvigor", "oh_modules", "node_modules", "resfile"}
    overrides = {"oh-package.json5", "src/main/module.json5"}
    for path in sorted(source.rglob("*")):
        relative = path.relative_to(source)
        if path.is_file() and not skip.intersection(relative.parts) and relative.as_posix() not in overrides:
            copy_changed(path, target / relative)
    adapter_types = ROOT / "entry/src/main/cpp/types/libadapter_c"
    for path in adapter_types.iterdir():
        if path.is_file():
            copy_changed(path, target / "src/main/cpp/types/libadapter_c" / path.name)
    manifest = {"module": {"name": "web_engine", "type": "har", "deviceTypes": ["tablet", "2in1"]}}
    package = {
        "name": "web_engine", "version": "1.0.0", "main": "Index.ets", "license": "Apache-2.0",
        "dependencies": {
            "inversify": "6.0.1", "reflect-metadata": "0.1.13",
            "libadapter.so": "file:./src/main/cpp/types/libadapter",
            "libadapter_c.so": "file:./src/main/cpp/types/libadapter_c"
        }
    }
    for name, value in (("src/main/module.json5", manifest), ("oh-package.json5", package)):
        path = target / name
        path.parent.mkdir(parents=True, exist_ok=True)
        text = json.dumps(value, ensure_ascii=False, indent=2) + "\n"
        if not path.exists() or path.read_text() != text:
            path.write_text(text)
    # CEF 原生读取 resfile；Markdown 页面则由 Qt qrc 提供，不假定 rawfile 可直接 fopen。
    resources = source / "src/main/resources/resfile"
    for path in resources.rglob("*"):
        if path.is_file():
            copy_changed(path, ROOT / "entry/src/main/resources/resfile" / path.relative_to(resources))
    for path in (sdk / "licenses").glob("*"):
        if path.is_file():
            copy_changed(path, ROOT / "entry/src/main/resources/rawfile/third_party_licenses/cef" / path.name)
    copy_changed(ROOT / "third_party/cef/licenses/HAR-DEPENDENCIES.txt",
                 ROOT / "entry/src/main/resources/rawfile/third_party_licenses/cef/HAR-DEPENDENCIES.txt")
    print(f"[HUIWEN_CEF] PREVIEW_RESOURCES ready module={target}")


if __name__ == "__main__":
    prepare()
