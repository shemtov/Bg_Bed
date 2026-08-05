#!/usr/bin/env python3
# ============================================================
#                        TEST  001
# ============================================================
"""
Switcher device discovery - TEST 001

TEST_NUMBER 1 - printed when the script runs.

WHY THIS FILE EXISTS
    aioswitcher 6.1.3 installs a discover_devices.exe wrapper that is
    broken - it tries to `from scripts.discover_devices import main`
    and that module is not shipped with the wheel, so it dies with
    ModuleNotFoundError. This script calls the library directly and
    sidesteps the wrapper entirely.

WHAT IT DOES
    Switcher devices broadcast a status message roughly every 4
    seconds. This listens for those broadcasts and prints everything
    each device announces.

HOW TO RUN
    python switcher_discover.py            listens for 60 seconds
    python switcher_discover.py 30         listens for 30 seconds

WHAT WE ARE LOOKING FOR
    device_id     needed to talk to the Breeze
    ip_address    needed to talk to the Breeze
    remote_id     the Breeze's IR remote profile (Breeze only)
    device_type   confirms it is a BREEZE

    It also prints the MAC address, which is private - blank it out
    before sharing this output anywhere public.

IF NOTHING APPEARS
    - Windows Firewall may be blocking incoming UDP. Allow python.exe,
      or check ports 10002, 10003, 20002 and 20003.
    - The PC must be on the SAME network as the Breeze, not a guest
      SSID and not a different subnet.
    - Some routers block broadcast traffic between wireless clients
      (often called AP isolation or client isolation).
"""

import asyncio
import sys
from dataclasses import asdict, is_dataclass
from pprint import PrettyPrinter

try:
    from aioswitcher.bridge import SwitcherBridge
    from aioswitcher.device import SwitcherBase
except ImportError:
    print("aioswitcher is not installed. Run:  pip install aioswitcher")
    sys.exit(1)

TEST_NUMBER = 1

printer = PrettyPrinter(indent=4)
seen = {}


def on_device_found(device) -> None:
    """Called by the bridge every time a device broadcast arrives."""
    dev_id = getattr(device, "device_id", "?")

    # Each device broadcasts every ~4 seconds. Print each one once,
    # otherwise the screen fills with repeats of the same device.
    if dev_id in seen:
        return
    seen[dev_id] = True

    print("=" * 62)
    print(f"  DEVICE FOUND  #{len(seen)}")
    print("=" * 62)
    if is_dataclass(device):
        printer.pprint(asdict(device))
    else:
        printer.pprint(vars(device))
    print()


async def run(delay: int) -> None:
    print(f"=== SWITCHER DISCOVERY - TEST {TEST_NUMBER:03d} ===")
    print(f"listening for Switcher broadcasts for {delay} seconds...")
    print("(devices announce themselves about every 4 seconds)\n")
    async with SwitcherBridge(on_device_found):
        await asyncio.sleep(delay)

    print("=" * 62)
    if seen:
        print(f"done - {len(seen)} device(s) found")
        print("\nSend the device_id, ip_address and remote_id.")
        print("You can blank the mac_address before sharing.")
    else:
        print("done - NOTHING FOUND")
        print("\nCheck, in this order:")
        print("  1. Windows Firewall - allow python.exe, UDP 20002/20003")
        print("  2. Is this PC on the same WiFi as the Breeze?")
        print("  3. Router client isolation / AP isolation turned on?")
    print("=" * 62)


if __name__ == "__main__":
    delay = 60
    if len(sys.argv) > 1:
        try:
            delay = int(sys.argv[1])
        except ValueError:
            print("usage: python switcher_discover.py [seconds]")
            sys.exit(1)
    try:
        asyncio.run(run(delay))
    except KeyboardInterrupt:
        print("\nstopped")

# ============================================================
#                  TEST  001   (end of file)
# ============================================================
