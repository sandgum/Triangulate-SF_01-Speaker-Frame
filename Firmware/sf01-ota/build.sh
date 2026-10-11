#!/bin/bash
# Builds "SF01 OTA.app" next to this script. Rebuilds only when the sources change.
set -euo pipefail
cd "$(dirname "$0")"

APP="build/SF01 OTA.app"
BIN="$APP/Contents/MacOS/sf01-ota"

if [[ -x "$BIN" && "$BIN" -nt main.swift && "$BIN" -nt Info.plist ]]; then
    exit 0
fi

mkdir -p "$APP/Contents/MacOS"
cp Info.plist "$APP/Contents/Info.plist"
swiftc -O main.swift -o "$BIN"

# A stable signing identity keeps the macOS Bluetooth permission attached to
# the app across rebuilds; an ad-hoc signature would have to be re-approved.
IDENTITY=$(security find-identity -v -p codesigning | awk -F'"' '/Apple Development/ {print $2; exit}')
if [[ -n "$IDENTITY" ]] && codesign --force --sign "$IDENTITY" "$APP" 2>/dev/null; then
    echo "built and signed as: $IDENTITY"
else
    codesign --force --sign - "$APP"
    echo "built with an ad-hoc signature"
fi
