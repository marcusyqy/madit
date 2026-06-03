#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"

BUILD_DIR=.build
BUILD_TYPE=debug

if [ "${1:-}" = "release" ]; then
  BUILD_TYPE=release
fi

if [ "$BUILD_TYPE" = "release" ]; then
  CFLAGS='-Wall -Wextra -O2 -g -std=c2x -DNDEBUG'
  SDL_CFG=Release
else
  CFLAGS='-Wall -Wextra -O0 -g -std=c2x'
  SDL_CFG=Debug
fi

DEFINES='-DVK_NO_PROTOTYPES'

if [ "$(uname -s)" = "Darwin" ]; then
  RPATH_ORIGIN='@executable_path'
else
  RPATH_ORIGIN='$ORIGIN'
fi

mkdir -p "$BUILD_DIR"

if [ ! -f "$BUILD_DIR/libSDL3.dylib" ] && [ ! -f "$BUILD_DIR/libSDL3.0.dylib" ] && [ ! -f "$BUILD_DIR/libSDL3.a" ]; then
  echo "Building SDL3..."
  cmake -S third_party/SDL -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$SDL_CFG"
  cmake --build "$BUILD_DIR" --config "$SDL_CFG"
fi

if [ -f "$BUILD_DIR/libSDL3.dylib" ]; then
  SDL_LIB="$PWD/$BUILD_DIR/libSDL3.dylib"
elif [ -f "$BUILD_DIR/libSDL3.0.dylib" ]; then
  SDL_LIB="$PWD/$BUILD_DIR/libSDL3.0.dylib"
elif [ -f "$BUILD_DIR/libSDL3.a" ]; then
  SDL_LIB="$PWD/$BUILD_DIR/libSDL3.a"
else
  echo "SDL3 lib not found in .build"
  exit 1
fi

if [ ! -f "$BUILD_DIR/libvma.a" ]; then
  echo "Building VMA..."
  clang++ -Wall -Wextra -O2 -g -std=c++20 $DEFINES -Ithird_party -c third_party/vma/vma.cpp -o "$BUILD_DIR/vma.o"
  ar rcs "$BUILD_DIR/libvma.a" "$BUILD_DIR/vma.o"
fi

if [ ! -f "$BUILD_DIR/libfreetype.a" ]; then
  echo "Building FreeType..."
  sh scripts/build_freetype.sh "$BUILD_TYPE"
fi

SDK_LIB_DIR=""

if [ -n "${VULKAN_SDK:-}" ]; then
  if [ -d "$VULKAN_SDK/lib" ]; then
    SDK_LIB_DIR="$VULKAN_SDK/lib"
  elif [ -d "$VULKAN_SDK/macOS/lib" ]; then
    SDK_LIB_DIR="$VULKAN_SDK/macOS/lib"
  fi
fi

if [ -z "$SDK_LIB_DIR" ]; then
  SDK_LIB_DIR="$(ls -1d \
    "$HOME"/VulkanSDK/*/macOS/lib \
    "$HOME"/VulkanSDK/*/x86_64/lib \
    "$HOME"/VulkanSDK/*/lib \
    2>/dev/null | sort -V | tail -n 1 || true)"
fi

if [ -z "$SDK_LIB_DIR" ] && [ "$(uname -s)" = "Linux" ]; then
  for dir in /usr/lib /usr/lib64 /usr/local/lib /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu; do
    if [ -f "$dir/libshaderc_shared.so" ] || [ -f "$dir/libshaderc.so" ] || [ -f "$dir/libshaderc_combined.a" ]; then
      SDK_LIB_DIR="$dir"
      break
    fi
  done
fi

if [ -z "$SDK_LIB_DIR" ]; then
  echo "Could not find Vulkan SDK lib directory (set VULKAN_SDK to override)"
  exit 1
fi

if [ -f "$SDK_LIB_DIR/libshaderc_shared.dylib" ]; then
  SHADERC_LIB="$SDK_LIB_DIR/libshaderc_shared.dylib"
elif [ -f "$SDK_LIB_DIR/libshaderc_shared.so" ]; then
  SHADERC_LIB="$SDK_LIB_DIR/libshaderc_shared.so"
elif [ -f "$SDK_LIB_DIR/libshaderc.so" ]; then
  SHADERC_LIB="$SDK_LIB_DIR/libshaderc.so"
elif [ -f "$SDK_LIB_DIR/libshaderc_combined.a" ]; then
  SHADERC_LIB="$SDK_LIB_DIR/libshaderc_combined.a"
elif [ -f "$SDK_LIB_DIR/libshaderc_shared.a" ]; then
  SHADERC_LIB="$SDK_LIB_DIR/libshaderc_shared.a"
else
  echo "shaderc lib not found in $SDK_LIB_DIR"
  exit 1
fi

clang $CFLAGS $DEFINES \
   -I../mb -Ithird_party/SDL/include -Isrc -Ithird_party -Ithird_party/freetype/include \
  -c src/build.c -o "$BUILD_DIR/madit.o"

clang++ "$BUILD_DIR/madit.o" -o "$BUILD_DIR/madit" \
  "$BUILD_DIR/libvma.a" \
  "$BUILD_DIR/libfreetype.a" \
  "$SDL_LIB" \
  "$SHADERC_LIB" \
  -Wl,-rpath,"$RPATH_ORIGIN" \
  -Wl,-rpath,"$SDK_LIB_DIR" || {
  rm -f "$BUILD_DIR/madit"
  exit 1
}

# Make runtime asset paths work whether running from repo root or .build.
ln -sfn ../data "$BUILD_DIR/data"
