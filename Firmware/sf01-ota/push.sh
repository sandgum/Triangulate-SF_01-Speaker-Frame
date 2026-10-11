#!/bin/bash
#
# Push firmware to the SF_01 speaker over Bluetooth — no USB cable.
#
#   ./push.sh                 push the current build of the firmware project
#   ./push.sh path/to/fw.bin  push a specific image
#   ./push.sh --no-confirm    push, but leave the new image on probation: the
#                             speaker rolls back after 3 minutes, or on reset,
#                             unless you then run --confirm
#   ./push.sh --confirm       keep an image that is on probation
#   ./push.sh --scan          check the speaker is reachable and report what
#                             it is running
#
# Exit status is 0 only if the tool reports success — for a normal push, that
# means the speaker rebooted into the new build and confirmed it.
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
APP="$HERE/build/SF01 OTA.app"
BIN="$HERE/../SF_01 ESP32 Firmware/build/sf01_speaker.bin"

"$HERE/build.sh" || exit 1

MODE=push
EXTRA=()
for arg in "$@"; do
    case "$arg" in
        --scan)       MODE=scan ;;
        --confirm)    MODE=confirm ;;
        --no-confirm) EXTRA+=(--no-confirm) ;;
        -*)           echo "unknown option: $arg" >&2; exit 2 ;;
        *)            BIN="$arg" ;;
    esac
done

if [[ "$MODE" == push ]]; then
    [[ -f "$BIN" ]] || { echo "no firmware image at: $BIN" >&2; exit 1; }
    # The app is started by LaunchServices, not from this directory, so it
    # needs an absolute path.
    ARGS=(push "$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")" ${EXTRA[@]+"${EXTRA[@]}"})
else
    ARGS=("$MODE")
fi

LOG="$(mktemp -t sf01-ota)"
trap 'rm -f "$LOG"' EXIT

# Started through `open` so macOS treats it as its own app for the Bluetooth
# permission. -W waits for it to quit; its output comes back through the log,
# which is echoed here as it grows.
open -n -W "$APP" --args "${ARGS[@]}" --log "$LOG" &
OPEN_PID=$!

printed=0
show_new() {
    local total
    total=$(wc -l < "$LOG" | tr -d ' ')
    if (( total > printed )); then
        sed -n "$((printed + 1)),${total}p" "$LOG"
        printed=$total
    fi
}
while kill -0 "$OPEN_PID" 2>/dev/null; do
    sleep 0.5
    show_new
done
show_new

grep -q "RESULT: OK" "$LOG"
