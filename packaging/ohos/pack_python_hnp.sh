#!/usr/bin/env bash
# 将已解压的鸿蒙 arm64 Python 运行时打成应用私有 HNP。
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "用法: $0 <鸿蒙 Python 安装目录>" >&2
  exit 2
fi

runtime_dir=$(cd "$1" && pwd)
project_dir=$(cd "$(dirname "$0")/../.." && pwd)
python_bin="$runtime_dir/bin/python3"
markitdown_src="$project_dir/third_party/markitdown/packages/markitdown/src/markitdown"
if [[ ! -x "$python_bin" || ! -d "$runtime_dir/lib" ]]; then
  echo "运行时目录必须包含可执行的 bin/python3 和 lib/" >&2
  exit 2
fi
if [[ $(file -L -b "$python_bin") != *"ARM aarch64"* ]]; then
  echo "bin/python3 不是鸿蒙 arm64 ELF，请勿使用本机 macOS Python" >&2
  exit 2
fi
if [[ ! -f "$markitdown_src/__init__.py" ]]; then
  echo "缺少 MarkItDown 源码；请先运行 git submodule update --init" >&2
  exit 2
fi

hnpcli=${HNPCLI:-/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/toolchains/hnpcli}
if [[ ! -x "$hnpcli" ]]; then
  echo "找不到 hnpcli，可通过 HNPCLI 环境变量指定工具路径" >&2
  exit 2
fi

mkdir -p "$project_dir/hnp/arm64-v8a"
stage=$(mktemp -d)
pack_output=$(mktemp -d "$project_dir/hnp/arm64-v8a/.pack.XXXXXX")
dependency_stage=""
trap 'rm -rf "$stage" "$pack_output"; if [[ -n "$dependency_stage" ]]; then rm -rf "$dependency_stage"; fi' EXIT
cp -pR "$runtime_dir/." "$stage/"
mkdir -p "$stage/share/huiwen" "$project_dir/hnp/arm64-v8a"
site_packages="$stage/lib/python3.14/site-packages"
if [[ ! -d "$site_packages" ]]; then
  echo "当前打包脚本要求鸿蒙 Python 3.14 运行时" >&2
  exit 2
fi

# 只缓存目标依赖；业务及 fork 源码每次重新复制，不从缓存取出。
requirements="$project_dir/packaging/ohos/requirements-pdf.txt"
dependency_key=$(shasum -a 256 "$requirements" | cut -d ' ' -f 1)
cache_root="$project_dir/.hnp-cache/cp314-musllinux_1_2_aarch64"
dependency_cache="$cache_root/$dependency_key"
mkdir -p "$cache_root"
if [[ ! -f "$dependency_cache/.complete" ]]; then
  echo "[HUIWEN_HNP] DEPENDENCIES install key=$dependency_key"
  dependency_stage=$(mktemp -d "$cache_root/install.XXXXXX")
  "${HOST_PYTHON:-python3}" -m pip install --disable-pip-version-check --no-input --no-compile \
    --platform musllinux_1_2_aarch64 --implementation cp --python-version 3.14 --abi cp314 \
    --only-binary=:all: --target "$dependency_stage" -r "$requirements"
  touch "$dependency_stage/.complete"
  if [[ -d "$dependency_cache" ]]; then
    rm -rf "$dependency_cache"
  fi
  mv "$dependency_stage" "$dependency_cache"
  dependency_stage=""
else
  echo "[HUIWEN_HNP] DEPENDENCIES cached key=$dependency_key"
fi
cp -R "$dependency_cache/." "$site_packages/"
rm -f "$site_packages/.complete"
# 防止运行时目录中的旧源码与本次源码合并。
rm -rf "$site_packages/markitdown" "$site_packages/huiwen_converter"
cp -R "$markitdown_src" "$site_packages/markitdown"
patch -s -d "$site_packages" -p1 < "$project_dir/packaging/ohos/markitdown-no-magika.patch"
# 汇文公开 API 与第三方库同处 HNP；页面只通过 NAPI 调用固定命令。
cp -R "$project_dir/converter/huiwen_converter" "$site_packages/huiwen_converter"
mkdir -p "$stage/share/licenses/markitdown"
cp "$project_dir/third_party/markitdown/LICENSE" "$stage/share/licenses/markitdown/LICENSE"
cp "$project_dir/third_party/markitdown/packages/markitdown/ThirdPartyNotices.md" \
  "$stage/share/licenses/markitdown/ThirdPartyNotices.md"
cp "$project_dir/packaging/ohos/python_smoke.py" "$stage/share/huiwen/python_smoke.py"
cp "$project_dir/converter/pdf2md/convert.py" "$stage/share/huiwen/convert_pdf.py"
cp "$project_dir/packaging/ohos/hnp.json" "$stage/hnp.json"
echo "[HUIWEN_HNP] PACK current Python sources"
"$hnpcli" pack -i "$stage" -o "$pack_output"
if [[ ! -s "$pack_output/huiwen_python.hnp" ]]; then
  echo "HNP 打包未生成有效产物，保留旧文件但停止构建" >&2
  exit 1
fi
mv -f "$pack_output/huiwen_python.hnp" "$project_dir/hnp/arm64-v8a/huiwen_python.hnp"
echo "[HUIWEN_HNP] GENERATED $project_dir/hnp/arm64-v8a/huiwen_python.hnp"
