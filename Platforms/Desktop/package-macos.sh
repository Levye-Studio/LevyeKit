#!/usr/bin/env bash
set -euo pipefail
[[ $# == 9 ]] || exit 2
root="$1"; target="$2"; asset="$3"; manifest="$4"
app_name="$5"; bundle_id="$6"; version_name="$7"; version_code="$8"; icon="$9"
fail() { echo "[Levye] $*" >&2; exit 2; }
[[ "$target" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || fail 'Invalid target'
case "$asset" in ''|/*|.|..|../*|*/../*|*/..|*/./*|./*|*/.|levye.project) fail 'Invalid asset path';; esac
assets="$root/$asset"
[[ -d "$assets" && ! -L "$assets" ]] || fail 'Missing assets'
case "$(cd "$assets" && pwd -P)" in "$root"/*) ;; *) fail 'Assets outside project';; esac
[[ -z "$(find "$assets" -type l -print -quit)" ]] || fail 'Asset symlinks are unsupported'
[[ -z "$(find "$assets" -type f \( -iname '*.p12' -o -iname '*.jks' -o -iname '*.pfx' -o -iname '*.keystore' \) -print -quit)" ]] || fail 'Keystore in assets'
for tool in otool install_name_tool codesign lipo plutil; do command -v "$tool" >/dev/null || fail "Missing $tool"; done
bin="$root/Targets/Release/bin/$target"; lib="$root/Targets/Release/lib"
[[ -f "$bin" && -f "$lib/Game.dylib" ]] || fail 'Missing Release host or game module'
arch="$(lipo -archs "$bin")"
case "$arch" in arm64|x86_64) ;; *) fail "Unsupported architecture: $arch";; esac
out="$root/Exports/Desktop/macOS-$arch"
for dir in "$root/Exports" "$root/Exports/Desktop" "$out"; do
  [[ ! -L "$dir" ]] || fail "Symlink export directory: $dir"
  mkdir -p "$dir"
done
stage="$(mktemp -d "$out/.stage.XXXXXXXX")"
cleanup() { rm -rf "$stage"; }
trap cleanup EXIT
app="$stage/$target.app"; contents="$app/Contents"; frameworks="$contents/Frameworks"
mkdir -p "$contents/MacOS" "$frameworks" "$contents/Resources/$(dirname "$asset")"
cp "$bin" "$contents/MacOS/$target"
cp -R "$assets" "$contents/Resources/$asset"
cp "$manifest" "$contents/Resources/levye.project"
# Copy built dylibs, dereferencing version aliases so each referenced filename exists.
for file in "$lib"/*.dylib; do
  [[ -f "$file" ]] || continue
  cp -L "$file" "$frameworks/$(basename "$file")"
done
cat > "$contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>$target</string>


<key>CFBundlePackageType</key><string>APPL</string>


<key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
# plutil encodes arbitrary display names safely rather than interpolating XML.
plist="$contents/Info.plist"
plutil -insert CFBundleIdentifier -string "$bundle_id" "$plist"
plutil -insert CFBundleName -string "$app_name" "$plist"
plutil -insert CFBundleDisplayName -string "$app_name" "$plist"
plutil -insert CFBundleVersion -string "$version_code" "$plist"
plutil -insert CFBundleShortVersionString -string "$version_name" "$plist"
if [[ -n "$icon" ]]; then
  case "$icon" in /*|.|..|../*|*/../*|*/..|*.PNG) fail 'Icon must be an asset-relative PNG path';; esac
  [[ "$icon" == *.png && -f "$assets/$icon" ]] || fail 'Missing PNG desktop icon'
  for tool in sips iconutil; do command -v "$tool" >/dev/null || fail "Missing $tool"; done
  source_icon="$assets/$icon"
  width="$(sips -g pixelWidth "$source_icon" | awk '/pixelWidth:/{print $2}')"
  height="$(sips -g pixelHeight "$source_icon" | awk '/pixelHeight:/{print $2}')"
  [[ "$width" == "$height" && "$width" =~ ^[0-9]+$ && "$width" -ge 1024 ]] || fail 'Desktop icon must be square and at least 1024 pixels'
  iconset="$stage/AppIcon.iconset"
  mkdir "$iconset"
  for size in 16 32 128 256 512; do
    sips -z "$size" "$size" "$source_icon" --out "$iconset/icon_${size}x${size}.png" >/dev/null
    double=$((size * 2))
    sips -z "$double" "$double" "$source_icon" --out "$iconset/icon_${size}x${size}@2x.png" >/dev/null
  done
  iconutil -c icns "$iconset" -o "$contents/Resources/AppIcon.icns"
  plutil -insert CFBundleIconFile -string AppIcon.icns "$plist"
fi
plutil -lint "$plist"
for file in "$contents/MacOS/$target" "$frameworks"/*.dylib; do
  chmod u+w "$file"
  if [[ "$file" == "$frameworks/"* ]]; then
    install_name_tool -id "@loader_path/$(basename "$file")" "$file"
    prefix='@loader_path'
  else
    prefix='@executable_path/../Frameworks'
  fi
  # Read Mach-O load commands, retaining spaces inside install names.
  deps="$(otool -L "$file" | sed -n '2,$s/^[[:space:]]*\(.*\) (compatibility version .*$/\1/p')"
  while IFS= read -r dep; do
    [[ -n "$dep" ]] || continue
    case "$dep" in /usr/lib/*|/System/Library/*) continue;; esac
    name="$(basename "$dep")"
    [[ -f "$frameworks/$name" ]] || fail "Unbundled dependency: $dep ($file)"
    replacement="$prefix/$name"
    [[ "$dep" == "$replacement" ]] || install_name_tool -change "$dep" "$replacement" "$file"
  done <<< "$deps"
  # Remove build-machine search paths: all non-system loads are now explicit.
  rpaths="$(otool -l "$file" | awk '/cmd LC_RPATH/{found=1;next} found && /path /{sub(/^ *path /, "");sub(/ \(offset [0-9]+\)$/, "");print;found=0}')"
  while IFS= read -r rpath; do
    [[ -z "$rpath" ]] || install_name_tool -delete_rpath "$rpath" "$file"
  done <<< "$rpaths"
done
for file in "$frameworks"/*.dylib; do codesign --force --sign - "$file"; done
codesign --force --sign - "$app"
codesign --verify --deep --strict "$app"
destination="$out/$target.app"
[[ ! -L "$destination" ]] || fail 'Symlink export destination'
if [[ -e "$destination" ]]; then
  [[ -d "$destination" ]] || fail 'Invalid export destination'
  mv "$destination" "$stage/previous.app"
fi
if ! mv "$app" "$destination"; then
  if [[ -d "$stage/previous.app" ]]; then
    if ! mv "$stage/previous.app" "$destination"; then
      trap - EXIT
      fail "Rollback failed; recover previous app from $stage"
    fi
  fi
  fail 'Publication failed'
fi
echo "[Levye] Desktop exported: $destination"
