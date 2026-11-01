#!/bin/sh
# Homebrew FreeType 2.15 links /usr/lib/libhvf.dylib. macOS 15.3 does not
# ship that library, so any process that loads Poppler aborts at startup.
# This points FreeType at a stub that reports failure. Ordinary fonts still
# extract. Run it again after `brew upgrade freetype`.
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cc -dynamiclib -o /opt/homebrew/lib/libhvf.dylib "$root/third_party/libhvf_stub.c"
codesign --force --sign - /opt/homebrew/lib/libhvf.dylib

ft=$(python3 -c 'import os; print(os.path.realpath("/opt/homebrew/opt/freetype/lib/libfreetype.6.dylib"))')
if otool -L "$ft" | grep -q '/usr/lib/libhvf.dylib'; then
  install_name_tool -change /usr/lib/libhvf.dylib /opt/homebrew/lib/libhvf.dylib "$ft"
  codesign --force --sign - "$ft"
fi

echo "FreeType now loads /opt/homebrew/lib/libhvf.dylib"
