#!/usr/bin/env python3
"""
Install a Pebble watchface and capture a screenshot per platform in a single
connection. Separate pebble-tool subprocesses (pebble install + pebble screenshot)
close the connection before the firmware finishes launching the installed watchface;
this script replicates the _capture_all_platforms flow without that split.
"""

import argparse
import datetime
import os
import shutil
import sys
import time
import png

# pebble's run_tool() prepends the SDK toolchain bin to PATH so qemu-pebble is
# findable. We bypass run_tool(), so replicate that setup before importing any
# emulator code that calls subprocess with 'qemu-pebble'.
from pebble_tool.util import get_persist_dir
from pebble_tool.sdk import sdk_version as _sdk_version
_sdk = _sdk_version()
if _sdk is not None:
    _toolchain_bin = os.path.join(get_persist_dir(), "SDKs", _sdk, "toolchain", "bin")
    os.environ['PATH'] = "{}:{}".format(_toolchain_bin, os.environ.get('PATH', ''))

from pebble_tool.commands.install import ToolAppInstaller
from pebble_tool.commands.screenshot import ScreenshotCommand
from pebble_tool.sdk import sdk_manager, get_sdk_persist_dir
from pebble_tool.sdk.emulator import ManagedEmulatorTransport
from libpebble2.communication import PebbleConnection


class _Args:
    no_correction = False
    scale = 1


def _capture(platform, pbw_path, hour, minute, output_path):
    target_sdk = sdk_manager.get_current_sdk()
    persist_dir = get_sdk_persist_dir(platform, target_sdk)
    if os.path.exists(persist_dir):
        shutil.rmtree(persist_dir)

    transport = ManagedEmulatorTransport(platform, None, False)
    pebble = PebbleConnection(transport)
    pebble.connect()
    pebble.run_async()

    try:
        # pypkjs accepts WebSocket immediately but isn't yet ready to relay install
        # bundles to a freshly-booted QEMU. Without this delay install is silently dropped.
        time.sleep(5)

        ToolAppInstaller(pebble, pbw_path, quiet=True).install()
        # install() returns on ACK, not on watchface launch; give firmware time to start the app
        time.sleep(2)

        # Send the target time twice with a settle delay (same as _set_time_1010)
        target = datetime.datetime.now().replace(
            hour=hour, minute=minute, second=0, microsecond=0
        )
        ScreenshotCommand._set_time(pebble, target)
        time.sleep(0.35)
        ScreenshotCommand._set_time(pebble, target)
        time.sleep(1.5)

        sc = ScreenshotCommand()
        sc.pebble = pebble
        image = sc._grab_processed_image(_Args(), show_progress=False)

        os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
        png.from_array(image, mode='RGBA;8').save(output_path)
        print("Saved screenshot to {}".format(output_path))
    finally:
        ScreenshotCommand._close_pebble_connection(pebble)
        ScreenshotCommand._shutdown_platform_emulator(platform, None)


def main():
    parser = argparse.ArgumentParser(
        description="Install and screenshot a Pebble watchface on each emulator platform."
    )
    parser.add_argument('platforms', nargs='+', help="Platform names (e.g. basalt emery)")
    parser.add_argument('--pbw', required=True, help="Path to the .pbw file")
    parser.add_argument('--time', default=None, metavar='HH:MM',
                        help="Emulator time as HH:MM or HH:MM:SS (default: current time)")
    parser.add_argument('--output-dir', default='screenshots')
    args = parser.parse_args()

    if args.time:
        parts = args.time.split(':')
        hour, minute = int(parts[0]), int(parts[1])
        time_label = '{}-{:02d}'.format(parts[0], int(parts[1]))
    else:
        now = datetime.datetime.now()
        hour, minute = now.hour, now.minute
        time_label = now.strftime('%H-%M')

    errors = []
    for platform in args.platforms:
        output_path = os.path.join(args.output_dir, '{}_{}.png'.format(platform, time_label))
        print('\n=== Capturing {} ==='.format(platform))
        try:
            _capture(platform, args.pbw, hour, minute, output_path)
        except Exception as e:
            print('ERROR: {} failed: {}'.format(platform, e), file=sys.stderr)
            errors.append(platform)

    if errors:
        print('\nFailed platforms: {}'.format(', '.join(errors)), file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
