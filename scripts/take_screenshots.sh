#!/usr/bin/env bash
set -euo pipefail

# pebble-emery QEMU machine has an audio device (-audio driver=sdl,id=audio0).
# SDL audio init can block waiting on the system audio backend, preventing the
# firmware from ever starting. The dummy driver initialises instantly.
export SDL_AUDIODRIVER=dummy

TIME=${1:-}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
mapfile -t PLATFORMS < <(jq -r '.pebble.targetPlatforms[]' "$PROJECT_DIR/package.json")
PBW="$PROJECT_DIR/build/$(basename "$PROJECT_DIR").pbw"

# Use pebble-tool's own Python so pebble_tool modules are importable
PEBBLE_PYTHON="$(dirname "$(readlink -f "$(which pebble)")")/python3"

pebble kill 2>/dev/null || true
pebble build

"$PEBBLE_PYTHON" "$SCRIPT_DIR/capture_screenshots.py" \
    "${PLATFORMS[@]}" \
    --pbw "$PBW" \
    --output-dir "$PROJECT_DIR/screenshots" \
    ${TIME:+--time "$TIME"}
