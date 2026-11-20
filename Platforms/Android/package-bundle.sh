#!/usr/bin/env bash
set -euo pipefail
set +x

if [[ $# -ne 7 ]]; then
  echo "Usage: package-bundle.sh WORK_DIR OUTPUT AAPT2 PLATFORM_JAR BUNDLETOOL KEYSTORE ALIAS" >&2
  exit 2
fi

work_dir="$1"
output="$2"
aapt2="$3"
platform_jar="$4"
bundletool="$5"
keystore="$6"
alias="$7"

for tool in java jarsigner zip unzip mktemp; do
  command -v "$tool" >/dev/null || {
    echo "Missing required tool: $tool" >&2
    exit 2
  }
done

if [[ ! -f "$bundletool" ]]; then
  echo "Bundletool not found: $bundletool" >&2
  exit 2
fi

if [[ -z "${LEVYE_ANDROID_STORE_PASSWORD:-}" ||
      -z "${LEVYE_ANDROID_KEY_PASSWORD:-}" ]]; then
  echo "AAB production signing credentials are missing." >&2
  exit 2
fi

bundle_dir="$(mktemp -d "$work_dir/bundle.XXXXXXXX")"

trap 'rm -rf "$bundle_dir"' EXIT

module_dir="$bundle_dir/base"
proto_apk="$bundle_dir/proto.apk"
module_zip="$bundle_dir/base.zip"
candidate="$bundle_dir/signed.aab"

mkdir -p "$module_dir/manifest"

echo "[Levye] Linking Android protobuf resources..."

"$aapt2" link \
  --proto-format \
  --auto-add-overlay \
  -o "$proto_apk" \
  --manifest "$work_dir/AndroidManifest.xml" \
  -A "$work_dir/assets" \
  -R "$work_dir/compiled-resources.zip" \
  -I "$platform_jar" \
  --min-sdk-version 29 \
  --target-sdk-version 35

unzip -q "$proto_apk" -d "$module_dir"

mv "$module_dir/AndroidManifest.xml" \
   "$module_dir/manifest/AndroidManifest.xml"

cp -R "$work_dir/lib" "$module_dir/lib"

echo "[Levye] Assembling Android base module..."

(
  cd "$module_dir"
  zip -q -r "$module_zip" .
)

bundle_config="$bundle_dir/BundleConfig.json"

cat > "$bundle_config" <<'EOF'
{
  "optimizations": {
    "uncompressNativeLibraries": {
      "enabled": true,
      "alignment": "PAGE_ALIGNMENT_16K"
    }
  }
}
EOF

echo "[Levye] Building Android App Bundle..."

java -jar "$bundletool" build-bundle \
  --modules="$module_zip" \
  --output="$candidate" \
  --config="$bundle_config"

echo "[Levye] Signing Android App Bundle..."

if ! jarsigner \
    -keystore "$keystore" \
    -storepass:env LEVYE_ANDROID_STORE_PASSWORD \
    -keypass:env LEVYE_ANDROID_KEY_PASSWORD \
    "$candidate" "$alias" >/dev/null 2>&1; then
  echo "Android App Bundle signing failed." >&2
  exit 2
fi

echo "[Levye] Validating Android App Bundle..."

java -jar "$bundletool" validate \
  --bundle="$candidate"

if ! jarsigner -verify "$candidate"; then
  echo "Android App Bundle signature verification failed." >&2
  exit 2
fi

mkdir -p "$(dirname "$output")"

mv -f "$candidate" "$output"

echo "Created $output"