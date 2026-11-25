#!/usr/bin/env bash
set -euo pipefail
# Pin the toolchain; changing this version requires a fresh Web build cache.
version=4.0.20
# Install individual SDK tools, not the aggregate SDK (which bundles Python).
compiler=releases-c387d7a7e9537d0041d2c3ae71b7538cc978104e-64bit
node=node-22.16.0-64bit
cache="$HOME/.levye/tools/web"
sdk="$cache/emsdk-$version"
python=''
candidates=(
  /opt/homebrew/bin/python3.14
  /opt/homebrew/bin/python3.13
  /opt/homebrew/bin/python3
  /usr/local/bin/python3
  python3.14 python3.13 python3.12 python3.11 python3.10 python3.15 python3
)
if [[ -n "${LEVYE_WEB_PYTHON:-}" ]]; then
  candidates=("$LEVYE_WEB_PYTHON")
fi
for candidate in "${candidates[@]}"; do
  if command -v "$candidate" >/dev/null 2>&1 && "$candidate" -c 'import sys; sys.exit(sys.version_info < (3,10))' 2>/dev/null; then
    python="$(command -v "$candidate")"
    break
  fi
done
if [[ -z "$python" ]]; then
  echo '[Levye] Python 3.10+ required. On macOS: brew install python@3.13' >&2
  exit 1
fi
shim="$(mktemp -d)"
trap 'rm -rf "$shim"' EXIT
ln -s "$python" "$shim/python3"
export PATH="$shim:$(dirname "$python"):$PATH"
export EMSDK_PYTHON="$python"
mkdir -p "$cache"
if [[ ! -f "$sdk/.levye-system-python-ready" ]]; then
  lock="$cache/.install-$version"
  mkdir "$lock" || { echo '[Levye] SDK installation already in progress (or stale lock).' >&2; exit 1; }
  trap 'rmdir "$lock"; rm -rf "$shim"' EXIT
  if [[ ! -d "$sdk/.git" ]]; then
    git -c advice.detachedHead=false clone --depth 1 --branch "$version" https://github.com/emscripten-core/emsdk.git "$sdk"
  fi
  "$python" "$sdk/emsdk.py" install "$node" "$compiler"
  "$python" "$sdk/emsdk.py" activate "$node" "$compiler"
  touch "$sdk/.levye-system-python-ready"
  rmdir "$lock"; trap 'rm -rf "$shim"' EXIT
fi
# Activation is local to this process, not the developer's shell.
source "$sdk/emsdk_env.sh"
# Keep using the developer-provided interpreter after SDK activation.
export EMSDK_PYTHON="$python"
export PATH="$shim:$(dirname "$python"):$PATH"
"$@"
