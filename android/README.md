# Android

This is a minimal Gradle project that packages the engine's native library and game assets
into an installable `.apk` (see [issue #58](https://github.com/jaidonlybbert/ENGINE/issues/58)).
It has no Java/Kotlin code - `AndroidManifest.xml` points straight at
`android.app.NativeActivity`, which loads `libEngine.so` and hands off to
`android_native_app_glue`'s `android_main()` (`src/main_android.cpp`).

Gradle does **not** build the native library itself - this project's C++ dependencies are
managed by Conan, not Gradle's own CMake integration, so the native side is built the same
way [`.github/workflows/android-ndk.yml`](../.github/workflows/android-ndk.yml) builds it,
via [`build_native.sh`](build_native.sh), which then stages the resulting `.so` and the
repo's game assets (`textures/`, `models/`, `gltf/`, plus shaders compiled to SPIR-V) into
this project's `app/src/main/jniLibs` and `app/src/main/assets`.

## Prerequisites

- **Android SDK** (e.g. via Android Studio's SDK Manager)
- **Android NDK r27c** specifically (matches [`profiles/Android-Clang-arm64`](../profiles/Android-Clang-arm64) and CI) - installable side-by-side via `sdkmanager "ndk;27.2.12479018"`, or Android Studio's SDK Manager → SDK Tools → NDK (Side by side) → pick 27.2.12479018
- **Conan 2** and **CMake**, same as the desktop build (see the root [README](../README.md))
- **`VULKAN_SDK`** set (same env var the desktop build already uses) - needed for `glslc` to compile shaders
- **JDK 17+** for Gradle itself (Android Studio manages this automatically; from a terminal you need your own)

## Build

```bash
cd android
./build_native.sh /path/to/ndk/27.2.12479018   # or: export ANDROID_NDK_HOME=...
./gradlew assembleDebug
```

The APK lands at `app/build/outputs/apk/debug/app-debug.apk`. Install it with
`adb install app/build/outputs/apk/debug/app-debug.apk`, or open this `android/` directory
as a project in Android Studio and hit Run.

`build_native.sh` only needs to be re-run when engine source or assets change - `./gradlew
assembleDebug` alone re-packages whatever is already staged.

## What's still missing

Producing an APK doesn't mean there's anything to see yet - see
[issue #59](https://github.com/jaidonlybbert/ENGINE/issues/59): there's no persistent
`VkRenderer`/render loop wired into `android_main()` yet, so the app currently just runs a
handful of one-shot startup checks (Vulkan surface creation, asset loading, the ImGui
Android backend) and logs the results via logcat, rather than rendering anything.
