# LevyeKit iOS simulator and source export

Requires macOS, Xcode, CMake, and an installed iOS simulator runtime. The CLI
builds arm64 simulator applications with the existing raylib-iOS/ANGLE pipeline.
The iOS build uses static game callbacks; desktop hot reload remains separate.

```sh
levye new MyGame --ios
cd MyGame
levye build --ios
levye run --ios
levye build --ios --release
levye run --ios --release
```

`levye new MyGame` creates a Desktop project without mobile sections. A later
`levye build --ios` adds `[ios]` once without changing `[platform].default`.
`levye project --set-default ios` adds the section and selects iOS for bare
`levye build` and `levye run`. `run` rebuilds the simulator app before install
so source and assets are current. Simulator applications appear in `Targets/ios-Debug/bin/` or
`Targets/ios-Release/bin/`.

Use the shared display section for orientation and presentation:

```toml
[display]
orientation = "portrait" # portrait, landscape, or auto
logical_width = 720
logical_height = 1280
scale_mode = "fit"
filter = "linear"
target_fps = 60
vsync = false
safe_area = "inset"
```

The optional `[ios]` section supports:

```toml
[ios]
bundle_id = "com.example.mygame"
deployment_target = "15.0"
display_name = "My Game"
version_name = "1.0.0"
version_code = 1
```

Bundle IDs, deployment targets (iOS 15.0 or newer), and versions are validated
before building. `display_name` defaults to `[project].name`; versions default
to `1.0.0` and `1`. The project target remains the `.app` filename. Assets use
`[paths].assets`; the app bundles them beside `levye.project`. `[display]`
controls the shared logical viewport. `inset` reports UIKit safe-area insets
as logical UI bounds; `edge_to_edge` allows UI throughout the visible drawable.
Touch and audio use the framework's
existing raylib integration. Optional modules are not yet supported on iOS.

To export a self-contained source project:

```sh
levye export --ios
```

The output is `Exports/iOS/<target>/`, containing `Game/`, `Framework/`, and
`Dependencies/`, plus build instructions and licenses. Move the whole directory
and run its documented CMake command to generate an Xcode project there.
Generated Xcode projects contain absolute paths, so regeneration after moving
the source export is required. The export does not contain signing credentials.

For a physical device, configure a separate Xcode build with the `iphoneos`
SDK and arm64, then supply your Apple development team and provisioning in
Xcode:

```sh
cmake -S . -B Build/device -G Xcode -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64
open Build/device/LevyeIOSExport.xcodeproj
```

Use these commands from an exported source directory. Device building and
signing have not been verified here.

If `levye run --ios` reports no simulator, install a runtime in Xcode and check
`xcrun simctl list devices available`. If configuration fails, verify the
selected Xcode installation with `xcodebuild -version` and the simulator SDK
with `xcrun --sdk iphonesimulator --show-sdk-path`.
