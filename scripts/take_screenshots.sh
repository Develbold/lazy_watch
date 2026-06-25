#!/usr/bin/env bash
set -euo pipefail

mkdir -p screenshots
pebble screenshot --no-open --all-platforms
