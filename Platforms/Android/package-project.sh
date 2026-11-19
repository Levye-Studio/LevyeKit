#!/usr/bin/env bash
# Package a generated project's existing Game sources and Assets directory.
set -euo pipefail
set +x # Never trace password checks or password-bearing environment variables.

if [[ $# -ne 6 && $# -ne 12 && $# -ne 15 ]]; then
  echo "Usage: package-project.sh PROJECT_DIR CONFIGURATION TARGET PACKAGE ASSET_DIR ORIENTATION [APP_NAME VERSION_NAME VERSION_CODE ICON FOREGROUND BACKGROUND [--sign KEYSTORE ALIAS]]" >&2
  exit 2
fi

project_dir="$(cd "$1" && pwd -P)"
configuration="$2"
target="$3"
package="$4"
asset_name="$5"
orientation="$6"
app_name="${7:-}"
version_name="${8:-1.0}"
version_code="${9:-1}"
icon="${10:-}"
foreground="${11:-}"
background="${12:-#202A44}"
production_sign=false
signing_keystore=""
signing_alias=""
if [[ $# -eq 15 ]]; then
  if [[ "${13}" != --sign || "$configuration" != Release || -z "${14}" || -z "${15}" ]]; then
    echo "Production signing requires --sign KEYSTORE ALIAS and a Release build." >&2
    exit 2
  fi
  production_sign=true
  signing_keystore="${14}"
  signing_alias="${15}"
  if [[ ! -f "$signing_keystore" ]]; then
    echo "Android signing keystore does not exist: $signing_keystore" >&2
    exit 2
  fi
fi
framework_root="$(cd "$(dirname "$0")/../.." && pwd)"

if [[ ! "$configuration" =~ ^[A-Za-z][A-Za-z0-9_-]*$ ]] ||
   [[ ! "$target" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] ||
   [[ ! "$package" =~ ^[A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+$ ]] ||
   [[ "$orientation" != portrait && "$orientation" != landscape ]]; then
  echo "Invalid Android configuration, target, or package in levye.project." >&2
  exit 2
fi
if [[ ! "$version_code" =~ ^[1-9][0-9]*$ ]] || [[ -z "$version_name" ]]; then
  echo "Android version_code must be positive and version_name must be nonempty." >&2
  exit 2
fi
# A failed production signing attempt must not leave an older development-signed
# Release APK at the expected output path.
if "$production_sign"; then
  rm -f "$project_dir/Targets/android-$configuration/bin/$target.apk"
fi
case "$asset_name" in
  ""|/*|../*|*/../*|*/..) echo "Android asset path must stay inside the project." >&2; exit 2 ;;
esac

assets="$project_dir/$asset_name"
if [[ ! -d "$assets" ]]; then
  echo "Asset directory is missing: $assets" >&2
  exit 2
fi
assets_real="$(cd "$assets" && pwd -P)"
case "$assets_real" in
  "$project_dir"/*) ;;
  *) echo "Asset directory must be inside the project: $assets" >&2; exit 2 ;;
esac
if [[ -n "$(find "$assets" -type l -print -quit)" ]]; then
  echo "Android asset packaging does not follow symbolic links: $assets" >&2
  exit 2
fi
if [[ -n "$(find "$assets" -type f \( -iname '*.jks' -o -iname '*.p12' -o -iname '*.pfx' -o -iname '*.keystore' \) -print -quit)" ]]; then
  echo "Android assets contain a keystore file; move signing keys outside Assets." >&2
  exit 2
fi
if "$production_sign"; then
  if [[ -L "$signing_keystore" ]]; then
    echo "Android signing keystore must not be a symbolic link." >&2
    exit 2
  fi
  signing_real="$(cd "$(dirname "$signing_keystore")" && pwd -P)/$(basename "$signing_keystore")"
  case "$signing_real" in
    "$assets_real"/*)
      echo "Android signing keystore must be outside the asset directory." >&2
      exit 2 ;;
  esac
fi

sdk_root="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}"
build_tools="${ANDROID_BUILD_TOOLS:-$sdk_root/build-tools/36.0.0}"
platform_jar="${ANDROID_PLATFORM_JAR:-$sdk_root/platforms/android-35/android.jar}"
ndk_root="${ANDROID_NDK_HOME:-$sdk_root/ndk/27.0.12077973}"
for tool in cmake ninja zip keytool java bash; do
  command -v "$tool" >/dev/null || { echo "Missing required tool: $tool" >&2; exit 2; }
done
for tool in "$build_tools/aapt2" "$build_tools/zipalign" "$build_tools/apksigner" \
            "$ndk_root/build/cmake/android.toolchain.cmake" "$platform_jar"; do
  [[ -e "$tool" ]] || { echo "Missing Android SDK/NDK component: $tool" >&2; exit 2; }
done

if "$production_sign"; then
  if [[ -z "${LEVYE_ANDROID_STORE_PASSWORD:-}" || -z "${LEVYE_ANDROID_KEY_PASSWORD:-}" ]]; then
    if [[ ! -t 0 || ! -r /dev/tty ]]; then
      echo "Production signing needs LEVYE_ANDROID_STORE_PASSWORD and LEVYE_ANDROID_KEY_PASSWORD in noninteractive builds." >&2
      exit 2
    fi
    # Disable terminal echo before showing a prompt, so immediate input cannot echo.
    stty -echo < /dev/tty
    trap 'stty echo < /dev/tty 2>/dev/null || true' EXIT
    if [[ -z "${LEVYE_ANDROID_STORE_PASSWORD:-}" ]]; then
      printf 'Android keystore password: ' > /dev/tty
      IFS= read -r LEVYE_ANDROID_STORE_PASSWORD < /dev/tty || exit 2
      printf '\n' > /dev/tty
    fi
    if [[ -z "${LEVYE_ANDROID_KEY_PASSWORD:-}" ]]; then
      printf 'Android key password: ' > /dev/tty
      IFS= read -r LEVYE_ANDROID_KEY_PASSWORD < /dev/tty || exit 2
      printf '\n' > /dev/tty
    fi
    stty echo < /dev/tty
    trap - EXIT
  fi
  if [[ -z "$LEVYE_ANDROID_STORE_PASSWORD" || -z "$LEVYE_ANDROID_KEY_PASSWORD" ]]; then
    echo "Android signing passwords cannot be empty." >&2
    exit 2
  fi
  store_password="$LEVYE_ANDROID_STORE_PASSWORD"
  key_password="$LEVYE_ANDROID_KEY_PASSWORD"
  unset LEVYE_ANDROID_STORE_PASSWORD LEVYE_ANDROID_KEY_PASSWORD
  if ! LEVYE_ANDROID_STORE_PASSWORD="$store_password" keytool -list -keystore "$signing_keystore" \
       -storepass:env LEVYE_ANDROID_STORE_PASSWORD >/dev/null 2>&1; then
    echo "Cannot open Android signing keystore; check its path and store password." >&2
    exit 2
  fi
  if ! LEVYE_ANDROID_STORE_PASSWORD="$store_password" keytool -list -keystore "$signing_keystore" -alias "$signing_alias" \
       -storepass:env LEVYE_ANDROID_STORE_PASSWORD >/dev/null 2>&1; then
    echo "Android signing alias is missing or invalid: $signing_alias" >&2
    exit 2
  fi
fi

build_dir="$project_dir/Build/android-$configuration"
output_dir="$project_dir/Targets/$(basename "$build_dir")"
work_dir="$build_dir/apk"
mkdir -p "$work_dir/lib/arm64-v8a" "$output_dir/bin"

set -- -S "$project_dir" -B "$build_dir" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ndk_root/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29 -DANDROID_STL=c++_static \
  -DPLATFORM=Android -DCMAKE_BUILD_TYPE="$configuration" \
  -DLEVYE_WITH_ANDROID=ON \
  -DLEVYE_ROOT="$framework_root" \

if [[ -n "${LEVYE_RAYLIB_SOURCE_DIR:-}" ]]; then
  set -- "$@" "-DFETCHCONTENT_SOURCE_DIR_RAYLIB=$LEVYE_RAYLIB_SOURCE_DIR"
fi
if [[ -n "${LEVYE_TOMLPLUSPLUS_SOURCE_DIR:-}" ]]; then
  set -- "$@" "-DFETCHCONTENT_SOURCE_DIR_TOMLPLUSPLUS=$LEVYE_TOMLPLUSPLUS_SOURCE_DIR"
fi
cmake "$@"
cmake --build "$build_dir" --target "${target}Android" -j 8

libraries="$output_dir/lib"
cp "$libraries/libmain.so" "$libraries/libraylib.so" "$work_dir/lib/arm64-v8a/"

payload="$work_dir/assets"

# Prepare the APK asset directory.
mkdir -p "$payload/$(dirname "$asset_name")"

# Package the project's game assets.
rm -rf "$payload/$asset_name"
cp -R "$assets" "$payload/$asset_name"

# Package the authoritative project configuration.
project_manifest="$project_dir/levye.project"

if [[ ! -f "$project_manifest" || -L "$project_manifest" ]]; then
    echo "Missing or invalid project manifest: $project_manifest" >&2
    exit 2
fi

# Signing identity is a host-side build setting, never an APK asset.
awk '
  {
    line = $0
    sub(/\r$/, "", line)
    if (line ~ /^[[:space:]]*\[android\.(signing|"signing"|\047signing\047)\][[:space:]]*(#.*)?$/) {
      signing = 1
      next
    }
    if (line ~ /^[[:space:]]*\[/) {
      signing = 0
      android = (line ~ /^[[:space:]]*\[android\][[:space:]]*(#.*)?$/)
    }
    if (signing) next
    if (inline_signing) {
      if (line ~ /}/) inline_signing = 0
      next
    }
    if (android && line ~ /^[[:space:]]*(signing|"signing"|\047signing\047)[[:space:]]*=/) {
      inline_signing = (line !~ /}/)
      next
    }
    print
  }
' "$project_manifest" > "$payload/levye.project"

rm -rf "$work_dir/res" "$work_dir/compiled-resources.zip"
java -Djava.awt.headless=true "$framework_root/Platforms/Android/GenerateLauncherResources.java" \
  "$assets_real" "$work_dir/res" "$work_dir/AndroidManifest.xml" \
  "$target" "$package" "$orientation" "$app_name" "$version_name" \
  "$version_code" "$icon" "$foreground" "$background"
"$build_tools/aapt2" compile --dir "$work_dir/res" -o "$work_dir/compiled-resources.zip"

"$build_tools/aapt2" link --auto-add-overlay -o "$work_dir/base.apk" \
  --manifest "$work_dir/AndroidManifest.xml" -A "$payload" \
  -R "$work_dir/compiled-resources.zip" \
  -I "$platform_jar" --min-sdk-version 29 --target-sdk-version 35
(
  cd "$work_dir"
  zip -q -0 base.apk lib/arm64-v8a/libmain.so lib/arm64-v8a/libraylib.so
)
"$build_tools/zipalign" -f -P 16 4 "$work_dir/base.apk" "$work_dir/aligned.apk"
if "$production_sign"; then
  keystore="$signing_keystore"
  alias="$signing_alias"
  store_password_source=env:LEVYE_ANDROID_STORE_PASSWORD
  key_password_source=env:LEVYE_ANDROID_KEY_PASSWORD
else
  keystore="$framework_root/Build/android-debug.keystore"
  alias=androiddebugkey
  mkdir -p "$(dirname "$keystore")"
  development_password=android
  if [[ ! -f "$keystore" ]]; then
    LEVYE_ANDROID_DEVELOPMENT_PASSWORD="$development_password" keytool -genkeypair -keystore "$keystore" \
      -storepass:env LEVYE_ANDROID_DEVELOPMENT_PASSWORD \
      -keypass:env LEVYE_ANDROID_DEVELOPMENT_PASSWORD \
      -alias "$alias" -keyalg RSA -keysize 2048 -validity 3650 \
      -dname 'CN=Android Debug,O=LevyeKit,C=CA' >/dev/null
  fi
  store_password_source=env:LEVYE_ANDROID_DEVELOPMENT_PASSWORD
  key_password_source=env:LEVYE_ANDROID_DEVELOPMENT_PASSWORD
fi
final_apk="$output_dir/bin/$target.apk"
candidate_apk="$work_dir/signed.apk"
rm -f "$candidate_apk" "$final_apk"
signing_failed=false
if "$production_sign"; then
  LEVYE_ANDROID_STORE_PASSWORD="$store_password" \
  LEVYE_ANDROID_KEY_PASSWORD="$key_password" \
    "$build_tools/apksigner" sign --ks "$keystore" --ks-key-alias "$alias" \
      --ks-pass "$store_password_source" --key-pass "$key_password_source" \
      --out "$candidate_apk" "$work_dir/aligned.apk" >/dev/null 2>&1 || signing_failed=true
else
  LEVYE_ANDROID_DEVELOPMENT_PASSWORD="$development_password" \
    "$build_tools/apksigner" sign --ks "$keystore" --ks-key-alias "$alias" \
      --ks-pass "$store_password_source" --key-pass "$key_password_source" \
      --out "$candidate_apk" "$work_dir/aligned.apk" >/dev/null 2>&1 || signing_failed=true
fi
if [[ "$signing_failed" == true ]]; then
  rm -f "$candidate_apk"
  echo "Android APK signing failed; check the key alias and key password." >&2
  exit 2
fi
if ! "$build_tools/apksigner" verify "$candidate_apk" ||
   ! "$build_tools/zipalign" -c -P 16 4 "$candidate_apk"; then
  rm -f "$candidate_apk"
  echo "Android APK signature or alignment verification failed." >&2
  exit 2
fi
mv -f "$candidate_apk" "$final_apk"
echo "Created $final_apk"
