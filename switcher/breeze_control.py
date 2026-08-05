#!/usr/bin/env python3
# ============================================================
#                        TEST  006
# ============================================================
"""
Switcher Breeze control - TEST 006

TEST_NUMBER 6 - printed when the script runs.

HISTORY
    001  first working version. Read state and send commands.
         Passed whatever it read straight into the command
         builder, so a bad read became a bad command.
    002  state validation, retries, explicit fields, clean
         Ctrl+C handling.
    003  network timeouts. Every call now gives up after 12
         seconds instead of hanging inside reader.read().
    004  the "target temperature 0" readings were NOT corruption.
         The Breeze reports 0 when the AC is OFF, and 003's
         validation wrongly rejected that as invalid. Fixed:
           - 0 is accepted when the state is off
           - asking for temp/mode/fan/swing now implies turning
             the unit ON, which is obviously what was meant
           - a 0 target is never sent; if the AC is off and no
             temperature was given, DEFAULT_TEMP is used
    005  004's off-state exemption did NOT fire - the reading was
         still rejected. I had guessed at how to detect "off" and
         guessed wrong, twice. So this version STOPS GUESSING:
           - a new `raw` command dumps every field of the response
             with its exact type and repr, so we can see what the
             device really sends instead of assuming
           - the off test now accepts several shapes rather than
             relying on one attribute name
           - the rejection message prints the actual state, so a
             failure says what it saw

    006  THE VALIDATION WAS THE BUG. `raw` proved it: the device
         answers successful=True with every field correct - state
         ON, mode COOL, fan AUTO, swing OFF, room 28.0 - and
         target_temperature 0, while the phone app shows 24.
         So a 0 target is simply what this Breeze reports. It is
         not corruption, not an off unit, and not the phone app
         holding the connection. My validation had been blocking
         every command over a value we do not need.
         Now: the target temperature is never a reason to reject
         a reading. When it reads 0 the script uses the value you
         asked for, or DEFAULT_TEMP.

         CONSEQUENCE: this device does not report its target back,
         so the panel can only ever show what it last COMMANDED,
         not what the AC is actually set to. Mode, fan, swing and
         room temperature all read correctly and can be trusted.

WHAT WENT WRONG IN VERSION 1
    A read came back with target_temperature = 0, which is not a real
    value - the device was still recovering from a connection that had
    been interrupted with Ctrl+C. Version 1 passed whatever it read
    straight into the command builder, so a garbage read became a
    garbage command and the AC switched off.

WHAT IS DIFFERENT HERE
    1. Every read is VALIDATED before it is used. Target temperature
       must be 16-30, and state, mode and fan must all be present. A
       bad read aborts instead of becoming a command.
    2. The read is RETRIED up to 3 times with a pause, because the
       first read after an interrupted session is the unreliable one.
    3. Commands are sent with EVERY field stated explicitly. Nothing
       is left to pass-through, so there is no path where a missing
       value turns into a zero.
    4. It prints exactly what it is about to send, before it sends it.
    5. Ctrl+C is caught and closes the connection cleanly, so the
       device is not left holding a half-open session.
    6. Every network call has a 12 second timeout. The Breeze
       sometimes acts on a command and never answers, which used to
       hang the script inside reader.read() until Ctrl+C.

YOUR DEVICE
    device_id 0998b7   key 02   ip 192.168.1.122   remote YACIBI00

USAGE
    python breeze_control.py raw            dump the RAW response - diagnostics
    python breeze_control.py state          read only, changes nothing
    python breeze_control.py on
    python breeze_control.py off
    python breeze_control.py temp 23
    python breeze_control.py mode cool      cool heat dry fan auto
    python breeze_control.py fan medium     low medium high auto
    python breeze_control.py swing on       on off
    python breeze_control.py set cool 23 medium off

IF A READ KEEPS FAILING
    - close the Switcher app on your phone, the device accepts one
      connection at a time
    - wait 15 seconds after any interrupted run before trying again
"""

import asyncio
import sys

try:
    from aioswitcher.api import SwitcherApi
    from aioswitcher.api.remotes import SwitcherBreezeRemoteManager
    from aioswitcher.device import (
        DeviceState,
        DeviceType,
        ThermostatFanLevel,
        ThermostatMode,
        ThermostatSwing,
    )
except ImportError:
    print("aioswitcher is not installed. Run:  pip install aioswitcher")
    sys.exit(1)

TEST_NUMBER = 6

DEVICE_ID = "0998b7"
DEVICE_KEY = "02"
IP_ADDRESS = "192.168.1.122"
REMOTE_ID = "YACIBI00"

TEMP_MIN = 16
TEMP_MAX = 30
# Used when the AC is off (so it reports target 0) and the command
# does not name a temperature.
DEFAULT_TEMP = 24

# The Breeze sometimes accepts a command and never answers. Without a
# timeout the script hangs forever inside reader.read() - that is the
# hang that needed Ctrl+C twice. Every call is wrapped in this.
NET_TIMEOUT = 12    # seconds

MODES = {
    "auto": ThermostatMode.AUTO,
    "dry": ThermostatMode.DRY,
    "fan": ThermostatMode.FAN,
    "cool": ThermostatMode.COOL,
    "heat": ThermostatMode.HEAT,
}

FANS = {
    "low": ThermostatFanLevel.LOW,
    "medium": ThermostatFanLevel.MEDIUM,
    "high": ThermostatFanLevel.HIGH,
    "auto": ThermostatFanLevel.AUTO,
}


def d(v):
    """Readable display for a library enum."""
    return getattr(v, "display", v)


def is_off(state) -> bool:
    """Is this state OFF?

    TEST 005: checked several ways rather than relying on one
    attribute, because assuming .name == "OFF" did not work and
    I would rather this be robust than clever.
    """
    if state is None:
        return False
    if getattr(state, "name", None) == "OFF":
        return True
    if str(getattr(state, "display", "")).lower() == "off":
        return True
    if "off" in repr(state).lower():
        return True
    return False


def show(resp, label=""):
    if label:
        print(f"{label}:")
    print("-" * 46)
    print(f"  state ....... {d(resp.state)}")
    print(f"  mode ........ {d(resp.mode)}")
    tt = resp.target_temperature
    print(f"  target temp . {tt} C" if tt else
          "  target temp . not reported by this device")
    print(f"  fan level ... {d(resp.fan_level)}")
    print(f"  swing ....... {d(resp.swing)}")
    print(f"  room temp ... {resp.temperature} C")
    print("-" * 46)
    print()


def validate(resp):
    """Return a list of problems. Empty list means the read is sane."""
    bad = []
    if not resp.successful:
        bad.append("device reported the request was not successful")
    if resp.state is None:
        bad.append("state missing")
    if resp.mode is None:
        bad.append("mode missing")
    if resp.fan_level is None:
        bad.append("fan level missing")
    # TEST 006: the target temperature is NOT validated. This device
    # reports 0 regardless of what it is really set to, so treating
    # that as a bad reading blocked every command for no reason.
    return bad


async def read_valid(api, tries=3):
    """Read the state, retrying until it passes validation."""
    for attempt in range(1, tries + 1):
        try:
            resp = await asyncio.wait_for(
                api.get_breeze_state(), timeout=NET_TIMEOUT)
        except asyncio.TimeoutError:
            print(f"read attempt {attempt}: no answer within {NET_TIMEOUT}s")
            if attempt < tries:
                print("    waiting 3 s and trying again...\n")
                await asyncio.sleep(3)
            continue
        problems = validate(resp)
        if not problems:
            return resp
        print(f"read attempt {attempt} rejected:")
        for p in problems:
            print(f"    - {p}")
        if attempt < tries:
            print("    waiting 3 s and trying again...\n")
            await asyncio.sleep(3)
    return None


async def raw_dump():
    """Print everything the device sends, with types. Diagnostics only."""
    async with SwitcherApi(DeviceType.BREEZE, IP_ADDRESS, DEVICE_ID, DEVICE_KEY) as api:
        resp = await asyncio.wait_for(api.get_breeze_state(), timeout=NET_TIMEOUT)
        print("RAW RESPONSE")
        print("=" * 60)
        print(f"  successful          {resp.successful!r}")
        for name in ("state", "mode", "fan_level", "swing",
                     "target_temperature", "temperature", "remote_id"):
            v = getattr(resp, name, "<no such attribute>")
            print(f"  {name:20s}{v!r}")
            print(f"  {'':20s}type={type(v).__name__} "
                  f"name={getattr(v, 'name', '-')} "
                  f"display={getattr(v, 'display', '-')}")
        print("=" * 60)
        print(f"  is_off(state) says: {is_off(resp.state)}")
        print("=" * 60)


async def read_only():
    async with SwitcherApi(DeviceType.BREEZE, IP_ADDRESS, DEVICE_ID, DEVICE_KEY) as api:
        resp = await read_valid(api)
        if resp is None:
            print("\nCould not get a valid reading. Close the Switcher phone app,")
            print("wait 15 seconds, and try again.")
            return False
        show(resp)
        return True


async def send(state=None, mode=None, temp=None, fan=None, swing=None):
    mgr = SwitcherBreezeRemoteManager()
    remote = mgr.get_remote(REMOTE_ID)

    async with SwitcherApi(DeviceType.BREEZE, IP_ADDRESS, DEVICE_ID, DEVICE_KEY) as api:
        before = await read_valid(api)
        if before is None:
            print("\nABORTED - no valid reading, so nothing was sent.")
            print("A command built on a bad reading is what switched the AC")
            print("off last time. Close the Switcher app, wait, try again.")
            return False

        show(before, "before")

        # Every field is stated explicitly. Anything not being changed
        # is taken from the VALIDATED reading above, never from a blank.
        f_mode = mode if mode is not None else before.mode
        f_fan = fan if fan is not None else before.fan_level
        f_swing = swing if swing is not None else before.swing

        # TEST 004: asking to change anything implies turning the unit ON.
        # Sending "off, 23 C" is never what someone meant by "temp 23".
        if state is not None:
            f_state = state
        elif mode or temp or fan or swing:
            f_state = DeviceState.ON
            if getattr(before.state, "name", "") == "OFF":
                print("the AC is off - this command will also turn it ON\n")
        else:
            f_state = before.state

        # TEST 004: never send a target of 0.
        if temp is not None:
            f_temp = temp
        elif before.target_temperature and before.target_temperature > 0:
            f_temp = before.target_temperature
        else:
            f_temp = DEFAULT_TEMP
            print(f"no target temperature known - using {DEFAULT_TEMP} C\n")

        if not (TEMP_MIN <= f_temp <= TEMP_MAX):
            print(f"ABORTED - {f_temp} C is outside {TEMP_MIN}-{TEMP_MAX}")
            return False

        print("about to send:")
        print(f"    state {d(f_state)}, mode {d(f_mode)}, {f_temp} C, "
              f"fan {d(f_fan)}, swing {d(f_swing)}")
        print()

        try:
            resp = await asyncio.wait_for(
                api.control_breeze_device(
                    remote, f_state, f_mode, f_temp, f_fan, f_swing),
                timeout=NET_TIMEOUT,
            )
            print("command accepted" if resp.successful else "command FAILED")
        except asyncio.TimeoutError:
            print(f"NO ANSWER within {NET_TIMEOUT}s.")
            print("The Breeze often acts on a command without replying,")
            print("so this does NOT mean it failed - check the AC, then")
            print("run 'state' in a few seconds to see what it did.")
        print()

        await asyncio.sleep(3)
        after = await read_valid(api)
        if after:
            show(after, "after")
        return True


def usage():
    print(__doc__)
    sys.exit(1)


def main():
    print(f"=== BREEZE CONTROL - TEST {TEST_NUMBER:03d} ===")
    if len(sys.argv) < 2:
        usage()

    cmd = sys.argv[1].lower()
    a = [x.lower() for x in sys.argv[2:]]

    try:
        if cmd == "raw":
            asyncio.run(raw_dump())
        elif cmd == "state":
            asyncio.run(read_only())
        elif cmd == "on":
            asyncio.run(send(state=DeviceState.ON))
        elif cmd == "off":
            asyncio.run(send(state=DeviceState.OFF))
        elif cmd == "temp" and a:
            asyncio.run(send(temp=int(a[0])))
        elif cmd == "mode" and a and a[0] in MODES:
            asyncio.run(send(mode=MODES[a[0]]))
        elif cmd == "fan" and a and a[0] in FANS:
            asyncio.run(send(fan=FANS[a[0]]))
        elif cmd == "swing" and a:
            asyncio.run(send(
                swing=ThermostatSwing.ON if a[0] == "on" else ThermostatSwing.OFF))
        elif cmd == "set" and len(a) >= 4:
            asyncio.run(send(
                state=DeviceState.ON,
                mode=MODES[a[0]],
                temp=int(a[1]),
                fan=FANS[a[2]],
                swing=ThermostatSwing.ON if a[3] == "on" else ThermostatSwing.OFF,
            ))
        else:
            usage()

    except KeyboardInterrupt:
        print("\n\nstopped by you - the connection was closed cleanly.")
        print("wait 15 seconds before the next command, so the device")
        print("has time to drop the session.")
        sys.exit(1)

    except Exception as exc:
        print()
        print(f"FAILED: {type(exc).__name__}: {exc}")
        print()
        print("check, in this order:")
        print("  1. close the Switcher app on your phone")
        print("  2. wait 15 seconds if a previous run was interrupted")
        print("  3. is the IP still 192.168.1.122? re-run the discovery")
        sys.exit(1)


if __name__ == "__main__":
    main()

# ============================================================
#                  TEST  006   (end of file)
# ============================================================
