#!/usr/bin/env bash
# Builds the Android native library via the same Conan + CMake + NDK pipeline
# .github/workflows/android-ndk.yml uses, then copies it and the engine's game assets into
# this Gradle project's source sets (see issue #58 - Gradle itself doesn't build the native
# side; it only packages what this script stages under app/src/main/).
#
# Usage: ./build_native.sh <path-to-NDK-r27c>
# (or set ANDROID_NDK_HOME). VULKAN_SDK must also be set (same env var the desktop build
# already relies on - see root CMakeLists.txt) so glslc can compile shaders to SPIR-V; that
# step is architecture-independent host tooling, so it's done here rather than by the
# Android CMake branch itself (which would need glslc available in CI too - out of scope
# for this script).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ANDROID_DIR="$REPO_ROOT/android"
NATIVE_OUT="$ANDROID_DIR/.native-build"
ABI="arm64-v8a"

NDK_PATH="${ANDROID_NDK_HOME:-${1:-}}"
if [ -z "$NDK_PATH" ]; then
    echo "Usage: $0 <path-to-NDK-r27c> (or set ANDROID_NDK_HOME)" >&2
    exit 1
fi

if [ -z "${VULKAN_SDK:-}" ]; then
    echo "VULKAN_SDK must be set (needed for glslc to compile shaders)." >&2
    exit 1
fi
GLSLC="$VULKAN_SDK/Bin/glslc"
[ -x "$GLSLC" ] || GLSLC="$VULKAN_SDK/bin/glslc"
if [ ! -x "$GLSLC" ]; then
    echo "glslc not found under \$VULKAN_SDK ($VULKAN_SDK)." >&2
    exit 1
fi

# CMake's default generator on Windows is Visual Studio, which can't cross-compile for
# Android - Ninja is required, but isn't guaranteed to be on PATH (unlike CI's Ubuntu
# runners, which default to Unix Makefiles and already have `make`). The Android SDK's
# `cmake` package bundles one; fall back to that if a standalone `ninja` isn't found.
NINJA="$(command -v ninja || true)"
if [ -z "$NINJA" ]; then
    NINJA="$(ls -1 "${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"/cmake/*/bin/ninja.exe 2>/dev/null | head -1)"
fi
if [ -z "$NINJA" ] || [ ! -x "$NINJA" ]; then
    echo "ninja not found (checked PATH and \$ANDROID_SDK_ROOT/cmake/*/bin) - install it or set ANDROID_SDK_ROOT." >&2
    exit 1
fi

# conan install below regenerates CMakeUserPresets.json in the repo root regardless of
# --output-folder (that's inherent to Conan's CMakeToolchain generator), adding an include
# for this script's own preset alongside the desktop build's. This script drives CMake with
# an explicit -DCMAKE_TOOLCHAIN_FILE instead of that preset, so it doesn't need the extra
# include - restore the file afterwards so this script is safe to re-run without leaving
# repo-root state behind (see the same issue noted while implementing this script).
USER_PRESETS="$REPO_ROOT/CMakeUserPresets.json"
USER_PRESETS_BACKUP=""
if [ -f "$USER_PRESETS" ]; then
    USER_PRESETS_BACKUP="$(mktemp)"
    cp "$USER_PRESETS" "$USER_PRESETS_BACKUP"
fi
restore_user_presets() {
    if [ -n "$USER_PRESETS_BACKUP" ]; then
        cp "$USER_PRESETS_BACKUP" "$USER_PRESETS"
        rm -f "$USER_PRESETS_BACKUP"
    else
        rm -f "$USER_PRESETS"
    fi
}
trap restore_user_presets EXIT

echo "==> Conan install (Android-Clang-arm64)"
conan config install "$REPO_ROOT/profiles/Android-Clang-arm64" --type=file --target-folder=profiles
conan install "$REPO_ROOT" -pr:h Android-Clang-arm64 -pr:b default \
    -c tools.android:ndk_path="$NDK_PATH" --output-folder "$NATIVE_OUT" --build=missing

echo "==> CMake configure + build"
cmake -S "$REPO_ROOT" -B "$NATIVE_OUT/cmakebuild" \
    -DCMAKE_TOOLCHAIN_FILE="$NATIVE_OUT/build/generators/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release -G Ninja -DCMAKE_MAKE_PROGRAM="$NINJA"
cmake --build "$NATIVE_OUT/cmakebuild" --parallel

echo "==> Staging native library"
mkdir -p "$ANDROID_DIR/app/src/main/jniLibs/$ABI"
cp "$NATIVE_OUT/cmakebuild/libEngine.so" "$ANDROID_DIR/app/src/main/jniLibs/$ABI/libEngine.so"

# libEngine.so is dynamically linked against libc++_shared.so (the Android-Clang-arm64
# profile sets compiler.libcxx=c++_shared) - unlike libandroid.so/liblog.so/libvulkan.so
# etc, this isn't a system library the OS already provides, so it has to be bundled into
# the APK too or the dynamic linker fails to load libEngine.so at all before any of our
# own code runs (no crash log under our own tag - just nothing, since NativeActivity's
# dlopen() never succeeds). Confirm this is still true if the toolchain/profile changes via:
#   llvm-readelf -d libEngine.so | grep NEEDED
LIBCXX_SHARED="$(ls -1 "$NDK_PATH"/toolchains/llvm/prebuilt/*/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so 2>/dev/null | head -1)"
if [ -z "$LIBCXX_SHARED" ]; then
    echo "libc++_shared.so not found under \$NDK_PATH/toolchains/llvm/prebuilt/*/sysroot - check NDK_PATH." >&2
    exit 1
fi
cp "$LIBCXX_SHARED" "$ANDROID_DIR/app/src/main/jniLibs/$ABI/libc++_shared.so"

echo "==> Staging assets"
ASSETS_DIR="$ANDROID_DIR/app/src/main/assets"
rm -rf "$ASSETS_DIR"
mkdir -p "$ASSETS_DIR"
cp -r "$REPO_ROOT/textures" "$ASSETS_DIR/textures"
cp -r "$REPO_ROOT/models" "$ASSETS_DIR/models"
cp -r "$REPO_ROOT/gltf" "$ASSETS_DIR/gltf"

echo "==> Compiling shaders"
mkdir -p "$ASSETS_DIR/shaders"
for shader in "$REPO_ROOT"/shaders/*.vert "$REPO_ROOT"/shaders/*.frag; do
    name="$(basename "$shader")"
    "$GLSLC" "$shader" -o "$ASSETS_DIR/shaders/$name.spv"
done

echo "Done. Native lib + assets are staged under $ANDROID_DIR/app/src/main. Run:"
echo "  cd $ANDROID_DIR && ./gradlew assembleDebug"
