#!/usr/bin/env bash
set -euo pipefail
set +x

version="1.18.3"
expected_sha="a099cfa1543f55593bc2ed16a70a7c67fe54b1747bb7301f37fdfd6d91028e29"

url="https://github.com/google/bundletool/releases/download/$version/bundletool-all-$version.jar"

# An explicit override never triggers automatic installation.
if [[ -n "${LEVYE_ANDROID_BUNDLETOOL:-}" ]]; then
  if [[ ! -f "$LEVYE_ANDROID_BUNDLETOOL" ||
        ! -r "$LEVYE_ANDROID_BUNDLETOOL" ]]; then
    echo "Bundletool not found: $LEVYE_ANDROID_BUNDLETOOL" >&2
    exit 2
  fi

  printf '%s\n' "$LEVYE_ANDROID_BUNDLETOOL"
  exit 0
fi

cache_dir="$HOME/.levye/tools/android"
bundletool="$cache_dir/bundletool.jar"

verify_checksum() {
  local actual

  if command -v sha256sum >/dev/null; then
    actual="$(sha256sum "$1" | awk '{print $1}')"
  elif command -v shasum >/dev/null; then
    actual="$(shasum -a 256 "$1" | awk '{print $1}')"
  else
    echo "Missing SHA-256 verification tool." >&2
    return 1
  fi

  [[ "$actual" == "$expected_sha" ]]
}

# Reuse an existing verified installation.
if [[ -f "$bundletool" ]]; then
  if ! verify_checksum "$bundletool"; then
    echo "Cached bundletool checksum mismatch: $bundletool" >&2
    echo "Remove the invalid file and retry." >&2
    exit 2
  fi

  printf '%s\n' "$bundletool"
  exit 0
fi

if ! command -v curl >/dev/null; then
  echo "Missing curl; required to download bundletool." >&2
  exit 2
fi

umask 077
mkdir -p "$cache_dir"

temporary="$(mktemp "$cache_dir/.bundletool.XXXXXXXX")"

trap 'rm -f "$temporary"' EXIT

echo "[Levye] Downloading bundletool $version..." >&2

curl --fail --location --silent --show-error \
  --proto '=https' \
  --proto-redir '=https' \
  --retry 2 \
  --connect-timeout 15 \
  --max-time 300 \
  --output "$temporary" \
  "$url"

if ! verify_checksum "$temporary"; then
  echo "Downloaded bundletool checksum mismatch." >&2
  exit 2
fi

mv -f "$temporary" "$bundletool"

echo "[Levye] Bundletool $version installed." >&2

printf '%s\n' "$bundletool"