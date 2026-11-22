# Android ARM64 projects

Android tooling is optional for desktop builds. LevyeKit uses raylib's
NativeActivity backend on Android, links the existing game callbacks into
`libmain.so`, and keeps desktop dynamic loading and hot reload unchanged.

Install CMake, Ninja, Bash, a Java JDK with `keytool`, and Android SDK
platform-tools, platform 35, build-tools 36.0.0, and NDK 27.0.12077973.
Set `ANDROID_HOME` or `ANDROID_SDK_ROOT` if your SDK is outside
`~/Library/Android/sdk`. `ANDROID_NDK_HOME`, `ANDROID_BUILD_TOOLS`, and
`ANDROID_PLATFORM_JAR` override the corresponding defaults.

```sh
levye new MyGame --android
cd MyGame
levye project
levye build
levye run
levye build --android --release
levye run --android --release
```

The generated `levye.project` contains `[android] package` and
`[platform] default = "android"`. Optional launcher metadata can be added:

```toml
[display]
orientation = "portrait" # portrait or landscape on Android
logical_width = 720
logical_height = 1280
scale_mode = "fit"
filter = "linear"
target_fps = 60
vsync = false
safe_area = "inset"
```

```toml
[android]
package = "com.example.mygame"
app_name = "My Game"
version_name = "1.2.0"
version_code = 12
icon = "Icons/launcher.png"
adaptive_icon_foreground = "Icons/foreground.png"
adaptive_icon_background = "#202A44"
```

Image paths are relative to `[paths] assets` and must remain inside it. PNG
and JPEG are accepted; an invalid explicit image fails packaging. The adaptive
background may also be an image path. Hex colors use `#RRGGBB` or `#AARRGGBB`.
The packager generates five launcher densities and adaptive icon resources for
Android 8+. Omitted values use the target as label, version `1.0`/`1`, and a
generated LevyeKit icon. Inspect metadata with `aapt2 dump badging GAME.apk`.
The JDK must support Java source-file execution.

Android is the default for `build` and
`run`; `--desktop` and `--android` override it for one command. Use
`levye project --set-default desktop|android` to change it. Android runs rebuild
the APK before installing and launching it, so changed assets are included.
Android is core-only at present, so projects with enabled optional modules
cannot select Android as their default.
Desktop commands never look for Android tools. Android Debug and Release use
`Build/android-Debug` and `Build/android-Release`, with corresponding
`Targets/android-Debug` and `Targets/android-Release`, and one local development signing key in the
framework's ignored `Build/` directory. Normal Debug and Release APKs use that
development key. To sign a Release APK with your own key, add:

```toml
[android.signing]
keystore = "../private-keys/my-game-release.p12"
alias = "my-game"
```

Relative keystore paths resolve from the project directory. Absolute paths are
also accepted. The keystore must already exist; it may be outside `Assets/`.
Only `keystore` and `alias` are supported in `[android.signing]`. Keep the
keystore outside Git when possible. If it is inside a project, add its path
or patterns such as `*.jks` and `*.p12` to that project's `.gitignore` before
creating it. Never put passwords in `levye.project`.

Create a key with the JDK. For PKCS12, use the same password for the store and
key. In Bash, read it without echo rather than typing a literal password into
a shell command:

```bash
read -r -s -p 'Keystore password: ' LEVYE_ANDROID_STORE_PASSWORD; printf '\n'
LEVYE_ANDROID_KEY_PASSWORD="$LEVYE_ANDROID_STORE_PASSWORD"
export LEVYE_ANDROID_STORE_PASSWORD LEVYE_ANDROID_KEY_PASSWORD
mkdir -p ../private-keys
keytool -genkeypair -keystore ../private-keys/my-game-release.p12 \
  -storetype PKCS12 -alias my-game -keyalg RSA -keysize 3072 \
  -validity 10000 -dname 'CN=My Game,O=My Studio,C=CA' \
  -storepass:env LEVYE_ANDROID_STORE_PASSWORD \
  -keypass:env LEVYE_ANDROID_KEY_PASSWORD
levye build --android --release --sign
```

`--sign` requires Android Release. The CLI reads
`LEVYE_ANDROID_STORE_PASSWORD` and `LEVYE_ANDROID_KEY_PASSWORD`; if either is
absent, it prompts without echo in an interactive terminal. Noninteractive
builds must provide both variables. Passwords are passed to `apksigner` by
environment variable, never as command arguments. The packager verifies the
signed and 16 KiB aligned APK before publishing it at
`Targets/android-Release/bin/NAME.apk`. Verify independently with:

```sh
apksigner verify --print-certs Targets/android-Release/bin/NAME.apk
zipalign -c -P 16 4 Targets/android-Release/bin/NAME.apk
```

Back up the keystore, its alias, both passwords, and the certificate fingerprint
in secure storage. Losing the signing key can prevent updates to an installed
app. Android will not install a differently signed APK over an existing app
with the same package name; use a separate test package or preserve app data
before replacing an installation signed with the development key.

## Production exports

```sh
levye export --android          # APK (default)
levye export --android --apk    # APK
levye export --android --aab    # Android App Bundle
levye export --android --all    # APK and AAB
```

Exports always use production signing; no additional `--sign` flag is needed.
Use `build --android --release --sign` instead when you need only a signed APK
in `Targets/`, without versioned publication. Ordinary builds and runs use
development signing, even in Release.

If signing is not configured, an interactive export offers to generate a key
under `~/.levye/signing/<package>/` or import an existing key. It adds only an
`[android.signing]` section to `levye.project`; other manifest text is retained.
In noninteractive builds, configure `keystore` and `alias` first and provide
both password environment variables shown above. Back up any new signing key
before distribution. Packaging rejects keystore files in the asset directory
and removes signing paths and aliases from the packaged project manifest.

Exports use the following layout (only the selected formats are generated):

```text
Exports/Android/<version_name>/<target>-v<version_name>-<version_code>.apk
Exports/Android/<version_name>/<target>-v<version_name>-<version_code>.aab
```

For export filenames, `version_name` must begin with a letter or digit and
otherwise contain only letters, digits, `.`, `_`, `+`, or `-`.
Re-exporting a version replaces the selected artifacts after successful
packaging and validation; other versions remain available. `--all` shares a
native build, asset preparation and signing credential sequence. Both artifacts
are staged before publication. Packaging failures preserve previous exports;
publication rollback is best-effort, not a crash-atomic two-file transaction.
Increase `version_code` for each distributed update. Concurrent exports of the
same project are rejected. After an interrupted export, remove
`.levye-android-export.lock` only after confirming no export is still running;
inspect any retained recovery files before deleting them. `Exports/` is ignored
by generated projects.

## App Bundle tooling and verification

AAB and combined exports require `jarsigner`, `unzip`, and bundletool, in addition
to the Android tooling above. LevyeKit automatically downloads pinned bundletool
1.18.3 to `~/.levye/tools/android/bundletool.jar` when missing, using `curl`, and
checks its SHA-256 with `sha256sum` or `shasum`. A valid cached JAR is reused;
a checksum mismatch fails rather than silently replacing it. Remove an invalid
cached file and retry. First-time installation needs internet access.

`LEVYE_ANDROID_BUNDLETOOL` overrides the JAR path; an invalid explicit path
fails without downloading a replacement. Custom JARs are the developer's
responsibility. `LEVYE_ANDROID_BUNDLE_AAPT2` optionally overrides the AAPT2
used for protobuf resources; the tested default is the SDK build-tools AAPT2.
APK-only exports do not need bundletool. The AAB packager assembles protobuf
resources and native libraries, signs with `jarsigner`, validates with
bundletool and verifies the JAR signature before publication.

Verify an exported bundle independently (substitute the actual filename):

```sh
java -jar ~/.levye/tools/android/bundletool.jar validate --bundle=GAME.aab
jarsigner -verify GAME.aab
java -jar ~/.levye/tools/android/bundletool.jar dump config --bundle=GAME.aab
```

The bundle configuration requests `PAGE_ALIGNMENT_16K` for uncompressed native
libraries. To test an AAB on a connected device, generate and install APKs with
bundletool (use your configured key and alias if explicitly signing the APKs):

```sh
java -jar ~/.levye/tools/android/bundletool.jar build-apks \
  --bundle=GAME.aab --output=GAME.apks --overwrite
java -jar ~/.levye/tools/android/bundletool.jar install-apks --apks=GAME.apks
```

Without explicit signing options, bundletool uses its debug signing workflow;
these APKs may not update an existing production-signed installation. For Google
Play distribution, upload the signed AAB and configure Play App Signing. An
upload key and the app signing key used by Google Play can be different.

Static 16 KiB ELF and generated split-APK ZIP alignment checks have passed on
macOS. Actual 16 KiB device runtime, Google Play acceptance, and Android export
on Linux and Windows hosts have not yet been verified. Java deprecation and
self-signed certificate warnings were non-blocking with the tested toolchain.

## Project configuration and runtime

`levye build --android` configures the shared `Platforms/Android/AndroidMain.cpp` for an existing generated
desktop project if it is missing. An existing entry point is left intact.
For iOS simulator builds and source export, see [the iOS guide](../iOS/README.md).
The packaging script also accepts a custom configuration name and writes to
`Build/android-<name>` and `Targets/android-<name>`. The CLI currently selects
Debug or Release; invoke the script directly for other configurations.
When building offline, point `LEVYE_RAYLIB_SOURCE_DIR` and
`LEVYE_TOMLPLUSPLUS_SOURCE_DIR` at verified local dependency sources. An
existing Android build cache can be reused without these overrides.

Place files under the generated project's `Assets/` tree. Subdirectories and
filename case are preserved in the APK. Host-managed resources use the same
logical paths on both platforms:

```cpp
auto texture = Levye::Assets::LoadTexture("Textures/player.png");
auto sound = Levye::Audio::LoadSound("Audio/beep.wav");
```

For arbitrary text or binary files, raylib's `LoadFileText` and `LoadFileData`
can read `Assets/Data/file.txt` on both platforms. Release their returned data
with `UnloadFileText` or `UnloadFileData`. The Android asset root is read-only;
runtime writes require a separate writable application data path. File
watchers remain a desktop development feature; rebuild the APK after changing
Android assets.
Font and shader handles use the same relative paths through their existing
LevyeKit APIs. For models, which currently use raylib directly, use a path such
as `Assets/Models/robot.glb` on both platforms.

Android currently accepts core LevyeKit projects. If Serialization, ImGui,
Animation, or Physics is enabled, Android configuration reports the unsupported
module rather than dropping it. Desktop `levye new --with` and `levye add`
continue to work. A generated project uses its existing `Source/Game.cpp` and
any sources listed in `Source/CMakeLists.txt`; no Android gameplay copy is made.

The APK targets ARM64 and Android API 29 or newer. The packaging script signs
and checks the APK, and aligns its libraries and ZIP entries for 16 KiB pages.
Check a connected emulator with `adb devices`. For background/resume testing,
press Home and reopen the app. For platform diagnostics, inspect `adb logcat`.
Portrait and landscape layouts use the shared display viewport; the generated
Android logical resolution is 720x1280. Set `[display].orientation = "landscape"`
and choose a landscape logical size to lock the activity. Android builds reject
`auto` because raylib 6.2 retains stale drawable dimensions after rotation,
which visibly stretches the frame. `inset`
reports Android display-cutout safe insets for UI while game rendering uses
the drawable area. `edge_to_edge` exposes the entire visible area for UI.

The repository Sandbox remains a separate regression example:

```sh
bash Platforms/Android/package-sandbox.sh Debug
adb install -r Targets/android-Debug/bin/Sandbox.apk
adb shell am start -n com.levyekit.sandbox/android.app.NativeActivity
```

Its Android package intentionally includes only the core scenes; the script
reports that desktop optional modules are omitted. Generated projects use
`package-project.sh` through `levye build --android` and package their Assets
automatically.
