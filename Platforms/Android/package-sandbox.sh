#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
sdk_root="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}"
build_tools="${ANDROID_BUILD_TOOLS:-$sdk_root/build-tools/36.0.0}"
platform_jar="${ANDROID_PLATFORM_JAR:-$sdk_root/platforms/android-35/android.jar}"
ndk_root="${ANDROID_NDK_HOME:-$sdk_root/ndk/27.0.12077973}"
configuration="${1:-Debug}"
build_dir="${2:-$repo_root/Build/android-$configuration}"
output_dir="$repo_root/Targets/$(basename "$build_dir")"
echo "Sandbox Android builds core scenes only; optional modules in the desktop manifest are not included."
mkdir -p "$output_dir/bin"

cmake -S "$repo_root" -B "$build_dir" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ndk_root/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29 -DANDROID_STL=c++_static \
  -DPLATFORM=Android -DCMAKE_BUILD_TYPE="$configuration" \
  -DLEVYE_WITH_ANDROID=ON \
  -DLEVYE_BUILD_EXAMPLES=ON -DLEVYE_BUILD_TOOLS=OFF \
  -DLEVYE_WITH_PHYSICS=OFF -DLEVYE_WITH_ANIMATION=OFF \
  -DLEVYE_WITH_IMGUI=OFF -DLEVYE_WITH_SERIALIZATION=OFF
cmake --build "$build_dir" --target SandboxAndroid -j 8

libraries="$output_dir/lib"
work_dir="$build_dir/apk"
mkdir -p "$work_dir/lib/arm64-v8a"
cp "$libraries/libmain.so" "$libraries/libraylib.so" "$work_dir/lib/arm64-v8a/"
mkdir -p "$work_dir/assets"
rm -rf "$work_dir/assets/Assets"
cp -R "$repo_root/Examples/Sandbox/Assets" "$work_dir/assets/Assets"
"$build_tools/aapt2" link -o "$work_dir/base.apk" \
  --manifest "$repo_root/Platforms/Android/AndroidManifest.xml" \
  -A "$work_dir/assets" \
  -I "$platform_jar" --min-sdk-version 29 --target-sdk-version 35
(
  cd "$work_dir"
  zip -q -0 base.apk lib/arm64-v8a/libmain.so lib/arm64-v8a/libraylib.so
)
"$build_tools/zipalign" -f -P 16 4 "$work_dir/base.apk" "$work_dir/aligned.apk"
keystore="$repo_root/Build/android-debug.keystore"
if [[ ! -f "$keystore" ]]; then
  keytool -genkeypair -keystore "$keystore" -storepass android -keypass android \
    -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 3650 \
    -dname 'CN=Android Debug,O=LevyeKit,C=CA' >/dev/null
fi
"$build_tools/apksigner" sign --ks "$keystore" --ks-key-alias androiddebugkey \
  --ks-pass pass:android --key-pass pass:android \
  --out "$output_dir/bin/Sandbox.apk" "$work_dir/aligned.apk"
"$build_tools/apksigner" verify "$output_dir/bin/Sandbox.apk"
echo "Created $output_dir/bin/Sandbox.apk"
