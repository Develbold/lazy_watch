#!/usr/bin/env bash
set -euo pipefail

mkdir -p screenshots

screenshot_at() {
    local platform="$1" fake_time="$2" outfile="$3"
    echo "=== $platform @ $fake_time -> screenshots/$outfile ==="
    pebble kill 2>/dev/null || true
    sleep 1
    faketime "$fake_time" pebble install --emulator "$platform"
    sleep 2
    pebble screenshot --no-open --emulator "$platform" "screenshots/$outfile"
    echo "  saved screenshots/$outfile"
}

# emery — four specific times
screenshot_at emery "12:00:00" emery_noon.png
screenshot_at emery "00:00:00" emery_midnight.png
screenshot_at emery "01:30:00" emery_half_two.png
screenshot_at emery "07:37:00" emery_longest.png

# Other platforms at the longest string time for hardware comparison
for platform in aplite basalt diorite; do
    screenshot_at "$platform" "07:37:00" "${platform}.png"
done

echo "=== done: $(ls screenshots/*.png | wc -l) screenshots ==="
