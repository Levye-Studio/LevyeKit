#!/usr/bin/env bash
set -euo pipefail
[[ $# == 9 ]] || exit 2
root="$1"
target="$2"
asset="$3"
manifest="$4"
app_name="$5"
bundle_id="$6"
version_name="$7"
version_code="$8"
icon="$9"

fail() {
  echo "[Levye] $*" >&2
  exit 2
}

[[ "$target" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || fail 'Invalid target'
case "$asset" in
  ''|/*|.|..|../*|*/../*|*/..|*/./*|./*|*/.|levye.project|levye.project/*|bin|bin/*|lib|lib/*|Launch.sh|Launch.sh/*|*.desktop)
    fail 'Invalid asset path'
    ;;
esac
assets="$root/$asset"
[[ -d "$assets" && ! -L "$assets" ]] || fail 'Missing assets'
case "$(cd "$assets" && pwd -P)" in "$root"/*) ;; *) fail 'Assets outside project';; esac
[[ -z "$(find "$assets" -type l -print -quit)" ]] || fail 'Asset symlinks are unsupported'
[[ -z "$(find "$assets" -type f \( -iname '*.p12' -o -iname '*.jks' -o -iname '*.pfx' -o -iname '*.keystore' \) -print -quit)" ]] || fail 'Keystore in assets'
[[ -z "$(find "$assets" ! -type f ! -type d -print -quit)" ]] || fail 'Special asset files are unsupported'
for tool in patchelf readelf ldd; do command -v "$tool" >/dev/null || fail "Missing $tool"; done
bin="$root/Targets/Release/bin/$target"; lib="$root/Targets/Release/lib"
[[ -f "$bin" && -f "$lib/Game.so" ]] || fail 'Missing Release host or game module'
case "$(LC_ALL=C readelf -h "$bin" | awk -F: '/Machine:/{gsub(/^ +/, "", $2); print $2}')" in
  'Advanced Micro Devices X86-64') arch=x86_64;;
  AArch64) arch=aarch64;;
  *) fail 'Unsupported ELF architecture';;
esac
out="$root/Exports/Desktop/Linux-$arch"
for dir in "$root/Exports" "$root/Exports/Desktop" "$out"; do
  [[ ! -L "$dir" ]] || fail "Symlink export directory: $dir"
  mkdir -p "$dir"
done
stage="$(mktemp -d "$out/.stage.XXXXXXXX")"
trap 'rm -rf "$stage"' EXIT
app="$stage/$target"
mkdir -p "$app/bin" "$app/lib" "$app/$(dirname "$asset")"
cp "$bin" "$app/bin/$target"
cp -R "$assets" "$app/$asset"
cp "$manifest" "$app/levye.project"
# Preserve versioned filenames and aliases as regular files for portable copies.
for file in "$lib"/*.so "$lib"/*.so.*; do
  [[ -f "$file" ]] || continue
  cp -L "$file" "$app/lib/$(basename "$file")"
done
for file in "$app/bin/$target" "$app/lib/"*; do
  chmod u+w "$file"
  if [[ "$file" == "$app/lib/"* ]]; then
    patchelf --set-rpath '$ORIGIN' "$file"
  else
    patchelf --set-rpath '$ORIGIN/../lib' "$file"
  fi
  while IFS= read -r dep; do
    [[ "$dep" != */* ]] || fail "Absolute or relative DT_NEEDED path: $dep"
    # Project libraries must be bundled; other dependencies are system requirements.
    if [[ -f "$lib/$dep" && ! -f "$app/lib/$dep" ]]; then fail "Missing bundled library: $dep"; fi
  done < <(patchelf --print-needed "$file")
done
# Resolve dependencies with build-machine environment overrides removed.
for file in "$app/bin/$target" "$app/lib/"*; do
  report="$(env -u LD_LIBRARY_PATH -u LD_PRELOAD -u LD_AUDIT LC_ALL=C ldd "$file" 2>&1)" || fail "Dependency check failed: $file: $report"
  [[ "$report" != *'not found'* ]] || fail "Unresolved dependency: $report"
  while IFS= read -r resolved; do
    case "$resolved" in "$app/lib/"*|/usr/lib/*|/usr/lib64/*|/lib/*|/lib64/*) ;; *) fail "Nonportable dependency: $resolved";; esac
  done < <(printf '%s\n' "$report" | sed -n 's/^[[:space:]]*[^ ]* => \(\/.*\) (0x[0-9a-fA-F]*)$/\1/p')
done
cat > "$app/Launch.sh" <<LAUNCH
#!/usr/bin/env bash
set -euo pipefail
root=\"\$(cd -- \"\$(dirname -- \"\${BASH_SOURCE[0]}\")\" && pwd -P)\"
cd -- \"\$root\"
exec \"\$root/bin/$target\" \"\$@\"
LAUNCH
chmod +x "$app/Launch.sh" "$app/bin/$target"
# Keep metadata relocatable; desktop-menu installation is a separate step.
[[ "$app_name" != *$'\n'* && "$app_name" != *$'\r'* ]] || fail 'Invalid application name'
if [[ -n "$icon" ]]; then
  case "$icon" in /*|.|..|../*|*/../*|*/..) fail 'Invalid icon path';; esac
  [[ "$icon" == *.png && -f "$assets/$icon" ]] || fail 'Missing PNG desktop icon'
fi
printf 'Name=%s\nBundleId=%s\nVersion=%s\nVersionCode=%s\nIcon=%s\n' "$app_name" "$bundle_id" "$version_name" "$version_code" "$icon" > "$app/lib/desktop-metadata.txt"
destination="$out/$target"
[[ ! -L "$destination" ]] || fail 'Symlink export destination'
if [[ -e "$destination" ]]; then
  [[ -d "$destination" ]] || fail 'Invalid export destination'
  mv "$destination" "$stage/previous"
fi
if ! mv "$app" "$destination"; then
  if [[ -d "$stage/previous" ]] && ! mv "$stage/previous" "$destination"; then
    trap - EXIT
    fail "Rollback failed; recover previous export from $stage"
  fi
  fail 'Publication failed'
fi
echo "[Levye] Desktop exported: $destination"
