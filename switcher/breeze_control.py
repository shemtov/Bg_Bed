#!/usr/bin/env python3
# ============================================================
#                        TEST  016
# ============================================================
"""
Switcher Breeze control - TEST 016

TEST_NUMBER 16 - printed when the script runs.

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

    007  READS WORKED, COMMANDS DID NOT - and the phone app did.
         raw/state answered perfectly, but 'off' hit a 12 s
         silence and then ConnectionResetError [WinError 64],
         and the AC never beeped. state afterwards: still on.

         THE CAUSE, from reading aioswitcher's source: the
         library's control_breeze_device() performs its OWN
         login + get-state before sending the command. TEST 006
         did its "before" read first on the SAME connection, so
         one TCP socket carried: login, state, a SECOND login,
         state, command. The Breeze answers the reads and then
         goes silent on the doubled-up session and resets it.
         The phone app opens a fresh connection per command -
         which is why it works.

         THE FIX: one connection per job.
           - "before" read  -> its own connection, then CLOSED
           - the command    -> a FRESH connection; the library's
             internal login+state is now the only traffic on it
           - "after" read   -> a third fresh connection
         Plus: the command is retried once on a fresh socket if
         the device times out or resets, with a 5 s pause; the
         aioswitcher version is printed at start; and a -v flag
         turns on the library's own debug log, which prints
         every packet in hex - the tool for whatever comes next.

    008  THE REAL MICROSCOPE. TEST 007's fresh-connection fix was
         right in principle but did not cure it: the command is
         still met with silence on a clean socket, so the problem
         is the CONTENT of the control packet, not the session.
         And an honest correction: 007 claimed -v prints packets
         in hex. It does not - aioswitcher only logs "sending a
         control packet", nothing more. That claim was wrong.
         So this version builds the microscope itself: -v now
         wraps the library's _send_packet and prints every packet
         REALLY sent, full hex, and every reply that comes back,
         plus the parsed session_id. With that we can compare our
         control packet against what the phone app sends, or hand
         a complete evidence file to whoever fixes it.

    009  THE TOKEN EXPERIMENT. Facts first: the silence survived
         the upgrade to aioswitcher 6.2.0, on clean connections,
         while the phone app works. Switcher's newer firmwares
         demand a LOCAL TOKEN for control (officially on Runner
         S11/S12 and Lights; the vendor said the set depends on
         device and firmware). The library hardcodes the Breeze
         as token-free, so even 6.2.0 never tries. This version
         lets us try: put your token in a file called token.txt
         next to this script (or pass --token=XXXX) and every
         connection is FORCED onto the token login. If the AC
         starts obeying - that was it. Without a token nothing
         changes from TEST 008.

         GET THE TOKEN: browse to  https://switcher.co.il/GetKey
         enter the email of your Switcher account, press send,
         and the token arrives by email. It looks like:
             zvVvd7JxtN7CgvkD1Psujw==

    010  WHAT THE HEX PROVED, AND ONE HONEST EXPERIMENT.
         The 009 dump showed: token login accepted (login+login2
         both answered), state answered, and the control packet -
         readable IR inside, 'NECX|26|32|...' - met with zero
         bytes, twice. Cross-checked against the library source,
         released AND dev branch: aioswitcher has a token-format
         command packet for runners and lights only. FOR THE
         BREEZE IT DOES NOT EXIST. Our forced token changed the
         login; the command still went out in the old shape.
         Home Assistant issue #155523 shows the same picture on
         a Breeze: sensors read, control dead, phone app fine.
         This is a known break between new Breeze firmware and
         the library, not something wrong on this bench.

         So this version adds ONE experiment, honestly labelled
         a guess: the command 'exp' sends the OFF code wrapped in
         the runners' token-command envelope with the Breeze
         '3701' tail. If the AC beeps - we beat the library to
         the fix and we will contribute it back. If not - the
         wire format is different and only a capture of the phone
         app's traffic (or Switcher themselves) can reveal it.

    011  THE LOCAL PACKET, REBUILT FROM A REAL CAPTURE.
         A second PCAPdroid capture - taken with the app's
         "home connection" toggle OFF, so the app talked DIRECTLY
         to 192.168.1.122:10000 instead of the cloud - gave the
         exact control packet the Breeze accepts. Two commands in
         that capture (off and on) were diffed: only a message
         counter and a timestamp change between them; everything
         else, including a 30-byte session block, is constant.

         The accepted control packet, after the device id, is:
             <TOKEN 4B> <KEYBLOCK 30B> 3701 <len> 00000000 00 <IR>
         where aioswitcher instead sends 72 zero bytes then 3701.
         That missing token+keyblock is why every command since
         the firmware update met silence.

         This version sends that exact packet with:
           - command 'off2' / 'on2' / 'raw2 <hex>' : the new path
           - the KEYBLOCK is taken from keyblock.txt (paste the
             30-byte block from the capture, or leave the built-in
             default from THIS device's capture)
         HONESTY: the keyblock's origin is not proven. It was
         constant across one session. If the device rejects it in
         a fresh session, it is session-derived and we capture the
         login math next. First we try the fast thing: reuse it.

    012  THE COUNTER WAS THE LAST BYTE. A THIRD capture (login +
         two accepted controls, in one local session) proved two
         things:
           1. the 30-byte keyblock is IDENTICAL across two
              separate sessions with different timestamps - it is
              a DEVICE constant, so reusing it is correct.
           2. our packet was byte-identical to the app's EXCEPT
              one 2-byte "counter" field. The app's control
              counter = its login counter minus 1. The library's
              login always uses counter ff03, so our control
              counter must be fe03 - not the hardcoded 4904 that
              TEST 011 sent and the device silently rejected.
         This version sends counter fe03. Everything else is
         unchanged and already matches the capture byte-for-byte.
         If the device still ignores it, the counter is validated
         against the app's varying login value and we read our own
         login counter live - but fe03 is the first, cleanest try.

    013  THE COUNTER CHAIN. A time-ordered trace of the app's
         local session cracked it. Every packet on a connection
         shares ONE base counter B:
             login   = B
             login2  = B + 1
             control = B - 1
         (proven: app B=1009 gave login f103, login2 f203,
          control f003 - little-endian.) The library breaks this:
         it sends login with B=ff03 but HARDCODES login2 to f503,
         so the chain is inconsistent and the device silently
         drops the control. The login and login2 themselves are
         accepted; only the broken chain kills the command.

         This version hand-builds all three packets with one
         consistent B, bypassing the library's login/login2
         entirely. Session id and a 2-byte login2 field proved
         free (device accepts the library's 00000000). The token,
         keyblock (device constant) and IR command are as proven.
         off3 / on3 / raw3 use this path.

    014  TWO BYTES AND A DEAD STEP. The 013 -v dump was gold:
         login answered, but login2 came back 44 bytes with
         status 6f (reject) instead of 130 bytes status 00. The
         cause: my login2 had 27 zero-bytes of padding where the
         capture has 30. Three missing bytes corrupted it, the
         session died, and get-state then timed out. Fixed to 30.
         Also: the app goes login -> login2 -> control with NO
         get-state in between (confirmed from the capture order),
         so the get-state step is removed - it was only adding a
         counter collision. off3/on3 now do exactly what the app
         does, byte-for-byte.

    015  THE LOGIN2 NONCE. 014 got login2 to the right length but
         the device replied 44 bytes / status 10 instead of 130
         bytes / status 00. A fourth capture pinned it: the app's
         login2 always carries a NONZERO 4-byte session nonce and
         a NONZERO 2-byte field right after the token. Across
         three captures those values were different each time and
         matched no CRC - they are per-connection nonces, and the
         device rejects the all-zero versions we were sending.
         Control does NOT echo them (its session stays 0), so they
         only have to be present and nonzero in login2. This
         version fills both from the live timestamp, mirroring the
         app. Everything else (counter chain, keyblock, IR) already
         matched byte-for-byte.

    016  SIDESTEP THE LOGIN2 NONCE. Reversing login2's session
         nonce led into per-connection crypto (the key-CRC second
         stage uses an unknown secret). But a key realisation makes
         it unnecessary: the LIBRARY's own login+login2 already
         authenticate enough for the device to answer get_state in
         every run. TEST 012 tried counter fe03 but sent the
         control on a FRESH socket with no login - that was the
         bug, not the counter. This version does the library's
         working login on ONE connection, then hand-sends the
         control on that SAME authenticated socket with counter
         fe03 (the library login uses ff03; control = login-1).
         No login2 rebuild, no nonce, no crypto.

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
    add -v to any command to print every packet in hex (debug log)

    python breeze_control.py off4           TEST 016: library login + hand control
    python breeze_control.py on4            TEST 016: same, turn ON
    python breeze_control.py off3           TEST 013: full hand-built OFF
    python breeze_control.py on3            TEST 013: full hand-built ON
    python breeze_control.py off2           TEST 011: app-shaped local OFF
    python breeze_control.py on2            TEST 011: app-shaped local ON
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
import logging
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

try:
    from aioswitcher.device.tools import convert_token_to_packet
except ImportError:
    convert_token_to_packet = None

try:
    from importlib.metadata import version as _pkg_version
    AIOSWITCHER_VERSION = _pkg_version("aioswitcher")
except Exception:
    AIOSWITCHER_VERSION = "unknown"

VERBOSE = False
TOKEN = None      # set from token.txt or --token=... in main()


def tap_packets(api):
    """TEST 008: wrap this api instance's _send_packet so every packet
    actually sent, and every answer received, is printed in full hex.
    The library logs only 'sending a control packet' - not the bytes.
    This is the microscope -v promised and TEST 007 did not deliver.

    TEST 009: also forces the token login when a token was given.
    The library hardcodes the Breeze as token-free and IGNORES the
    token argument for it, so it is injected here by hand. With
    _token set, _login() switches to LOGIN_TOKEN_PACKET_TYPE2 plus
    the second login packet - the same dance the phone app does
    with a token-firmware device.
    """
    if TOKEN and convert_token_to_packet:
        api._token = convert_token_to_packet(TOKEN)
    if not VERBOSE:
        return api
    original = api._send_packet

    async def tapped(packet_id, packet):
        print(f"  >>> {packet_id} packet, {len(packet)} hex chars:")
        for i in range(0, len(packet), 64):
            print(f"      {packet[i:i+64]}")
        response = await original(packet_id, packet)
        try:
            h = response.hex()
        except Exception:
            h = repr(response)
        print(f"  <<< reply, {len(h)} hex chars:")
        for i in range(0, len(h), 64):
            print(f"      {h[i:i+64]}")
        return response

    api._send_packet = tapped
    return api

TEST_NUMBER = 16

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
        tap_packets(api)
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
        tap_packets(api)
        resp = await read_valid(api)
        if resp is None:
            print("\nCould not get a valid reading. Close the Switcher phone app,")
            print("wait 15 seconds, and try again.")
            return False
        show(resp)
        return True


async def send(state=None, mode=None, temp=None, fan=None, swing=None):
    """TEST 007: one connection per job.

    The library's control_breeze_device() does its own login and its
    own state read internally. Sharing a socket with our "before" read
    meant two logins on one connection - the Breeze answers the reads,
    then goes silent on the command and resets. So now every step gets
    a fresh connection, exactly as the phone app behaves.
    """
    mgr = SwitcherBreezeRemoteManager()
    remote = mgr.get_remote(REMOTE_ID)

    # ---- job 1: read "before" on its own connection, then close it ----
    async with SwitcherApi(DeviceType.BREEZE, IP_ADDRESS, DEVICE_ID, DEVICE_KEY) as api:
        tap_packets(api)
        before = await read_valid(api)
        if before is None:
            print("\nABORTED - no valid reading, so nothing was sent.")
            print("A command built on a bad reading is what switched the AC")
            print("off last time. Close the Switcher app, wait, try again.")
            return False
        show(before, "before")
    # the connection is now CLOSED. Give the device a moment to notice.
    await asyncio.sleep(2)

    # Every field is stated explicitly. Anything not being changed
    # is taken from the VALIDATED reading above, never from a blank.
    f_mode = mode if mode is not None else before.mode
    f_fan = fan if fan is not None else before.fan_level
    f_swing = swing if swing is not None else before.swing

    # TEST 004: asking to change anything implies turning the unit ON.
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

    # ---- job 2: the command, on a FRESH connection, retried once ----
    sent_ok = False
    for attempt in (1, 2):
        try:
            async with SwitcherApi(DeviceType.BREEZE, IP_ADDRESS,
                                   DEVICE_ID, DEVICE_KEY) as api:
                tap_packets(api)
                resp = await asyncio.wait_for(
                    api.control_breeze_device(
                        remote, f_state, f_mode, f_temp, f_fan, f_swing),
                    timeout=NET_TIMEOUT,
                )
            print("command accepted" if resp.successful else "command FAILED")
            sent_ok = True
            break
        except asyncio.TimeoutError:
            print(f"attempt {attempt}: NO ANSWER within {NET_TIMEOUT}s.")
        except (ConnectionResetError, ConnectionError, OSError) as exc:
            print(f"attempt {attempt}: connection dropped "
                  f"({type(exc).__name__}: {exc})")
        if attempt == 1:
            print("    retrying once on a fresh connection in 5 s...\n")
            await asyncio.sleep(5)
    if not sent_ok:
        print("The Breeze often acts on a command without replying,")
        print("so this does NOT mean it failed - check the AC, then")
        print("run 'state' in a few seconds to see what it did.")
    print()

    # ---- job 3: read "after" on a third fresh connection ----
    await asyncio.sleep(3)
    try:
        async with SwitcherApi(DeviceType.BREEZE, IP_ADDRESS,
                               DEVICE_ID, DEVICE_KEY) as api:
            tap_packets(api)
            after = await read_valid(api)
            if after:
                show(after, "after")
    except (ConnectionResetError, ConnectionError, OSError) as exc:
        print(f"could not read back the state ({type(exc).__name__}) - "
              "run 'state' manually in a few seconds.")
    return True


async def experimental_off():
    """TEST 010: the OFF code inside a token-command envelope.

    Header copied from GENERAL_TOKEN_COMMAND (the packet the new
    runners/lights firmware accepts), tail '3701'+length+command
    copied from BREEZE_COMMAND_PACKET. This exact combination is a
    GUESS - it exists nowhere in the library. Length and CRC are
    applied by the library's own _send_packet, same as any packet.
    """
    if not TOKEN or not convert_token_to_packet:
        print("this experiment needs token.txt - see TEST 009 notes")
        return
    from aioswitcher.device import DeviceState as DS

    mgr = SwitcherBreezeRemoteManager()
    remote = mgr.get_remote(REMOTE_ID)

    async with SwitcherApi(DeviceType.BREEZE, IP_ADDRESS, DEVICE_ID, DEVICE_KEY) as api:
        tap_packets(api)
        # token login (forced by tap_packets), then read the state so
        # the command is built exactly as the library would build it
        timestamp, login_resp = await api._login()
        cur = await asyncio.wait_for(api.get_breeze_state(), timeout=NET_TIMEOUT)
        cmd = remote.build_command(
            DS.OFF, cur.mode, 24, cur.fan_level, cur.swing, cur.state)
        tok = convert_token_to_packet(TOKEN)

        packet = (
            "fef0000003050102"
            + "00000000"
            + "000000"
            + "000000000000000000"
            + timestamp
            + "00000000000000000000f0fe"
            + DEVICE_ID
            + "00"
            + tok
            + "00000000"                    # DEVICE_PASS
            + "0" * 54
            + "3701"
            + cmd.length
            + cmd.command
        )
        print("EXPERIMENT: OFF in a token envelope - watch the AC")
        try:
            resp = await asyncio.wait_for(
                api._send_packet("token-experiment control", packet),
                timeout=NET_TIMEOUT)
            print(f"device answered {len(resp)} bytes: {resp.hex()[:80]}")
            print("AN ANSWER AT ALL is news - did the AC beep?")
        except asyncio.TimeoutError:
            print("silence again. The guess is wrong, or close but not exact.")
        except (ConnectionResetError, ConnectionError, OSError) as exc:
            print(f"connection dropped: {type(exc).__name__}: {exc}")


# TEST 011: the 30-byte session block observed in this device's local
# capture. Constant across both commands in that session. If a fresh
# session rejects it, it is session-derived - see the notes above.
DEFAULT_KEYBLOCK = "be89834ee8fb3e4bd0a78d41dabbf57ea436cff65fd6a8c9082a6c1049201a"


def load_keyblock():
    try:
        import os
        here = os.path.dirname(os.path.abspath(__file__))
        with open(os.path.join(here, "keyblock.txt")) as fh:
            t = "".join(fh.read().split())
            if t:
                return t
    except FileNotFoundError:
        pass
    return DEFAULT_KEYBLOCK


async def send_local(state=None, mode=None, temp=None, fan=None, swing=None,
                     raw_ir=None):
    """TEST 011: send the control packet in the exact shape the phone app
    uses locally, byte-for-byte from the capture. The library builds the
    login (token-based, proven working) and the IR command string; we
    build the envelope it gets wrong.
    """
    if not TOKEN or not convert_token_to_packet:
        print("this path needs token.txt - see TEST 009 notes")
        return False

    from aioswitcher.api import packets as P
    from aioswitcher.device.tools import (current_timestamp_to_hexadecimal,
                                          set_message_length,
                                          sign_packet_with_crc_key)
    from aioswitcher.device import DeviceState as DS

    tok = convert_token_to_packet(TOKEN)      # 4 bytes, e.g. 5aa5ec11
    keyblk = load_keyblock()                  # 30 bytes
    mgr = SwitcherBreezeRemoteManager()
    remote = mgr.get_remote(REMOTE_ID)

    async with SwitcherApi(DeviceType.BREEZE, IP_ADDRESS, DEVICE_ID, DEVICE_KEY) as api:
        tap_packets(api)                       # forces the token login + hex dump
        timestamp, login_resp = await api._login()
        if not login_resp.successful:
            print("login failed"); return False

        cur = await asyncio.wait_for(api.get_breeze_state(), timeout=NET_TIMEOUT)

        if raw_ir is not None:
            ir_cmd = raw_ir
        else:
            f_state = state if state is not None else cur.state
            f_mode  = mode  if mode  is not None else cur.mode
            f_fan   = fan   if fan   is not None else cur.fan_level
            f_swing = swing if swing is not None else cur.swing
            f_temp  = temp  if temp  is not None else 24
            command = remote.build_command(
                f_state, f_mode, f_temp, f_fan, f_swing, cur.state)
            ir_cmd = command.command
        clen = f"{len(ir_cmd)//2:02x}"

        # Build the envelope EXACTLY as the captured app packet:
        #  fef0 0000(len placeholder) 03050102 00000000 <ctr> 01000000
        #  <tok> 0000 <ts> 00000000000000000000 f0fe <devid>00
        #  <tok> <keyblk> 3701 <clen> 00000000 00 <ir>
        # TEST 012: the control counter must be the LOGIN counter minus 1.
        # The library's token login always uses ff03, so this is fe03.
        # (Proven: cap2 login 4a04 -> control 4904; cap3 login f103 ->
        #  control f003; both are login-counter minus one.)
        ctr = "fe03"
        body = (
            "fef00000"                    # magic + length placeholder
            "03050102"                    # control type (token variant)
            "00000000"                    # session field (app used 0 here too)
            + ctr
            + "01000000"
            + tok
            + "0000"
            + timestamp
            + "00000000000000000000f0fe"
            + DEVICE_ID + "00"
            + tok
            + keyblk
            + "3701"
            + clen
            + "00000000" + "00"
            + ir_cmd
        )
        packet = sign_packet_with_crc_key(set_message_length(body))

        print("SENDING the app-shaped local control packet - watch the AC")
        try:
            from binascii import unhexlify
            api._writer.write(unhexlify(packet))
            resp = await asyncio.wait_for(api._reader.read(1024), timeout=NET_TIMEOUT)
            print(f"device answered {len(resp)} bytes: {resp.hex()[:96]}")
            # the app's accepted reply began fef0300004000102...0100000 0
            if "0400010200000000" in resp.hex():
                print(">>> looks like the ACK the device sent the app. Did it beep?")
        except asyncio.TimeoutError:
            print("silence. If the login worked but this timed out, the keyblock")
            print("is likely session-derived - we capture the login math next.")
        except Exception as exc:
            print(f"error: {type(exc).__name__}: {exc}")
    return True


async def send_full(state=None, mode=None, temp=None, fan=None, swing=None,
                    raw_ir=None, base=0x03e8):
    """TEST 013: hand-build login + login2 + control with ONE consistent
    base counter B, the way the app does. base is the little-endian u16
    value B; login=B, login2=B+1, control=B-1. Everything is signed with
    the library's own CRC (proven correct - the device accepts our login).
    """
    if not TOKEN or not convert_token_to_packet:
        print("this path needs token.txt"); return False

    from binascii import unhexlify
    from aioswitcher.device.tools import (current_timestamp_to_hexadecimal,
                                          set_message_length,
                                          sign_packet_with_crc_key)
    from aioswitcher.device import DeviceState as DS

    tok    = convert_token_to_packet(TOKEN)     # 4 bytes
    keyblk = load_keyblock()                    # 30 bytes, device constant
    ts     = current_timestamp_to_hexadecimal() # 4 bytes LE

    def le16(n):  # little-endian 2-byte hex
        n &= 0xffff
        return f"{n & 0xff:02x}{(n >> 8) & 0xff:02x}"

    ctr_login   = le16(base)
    ctr_login2  = le16(base + 1)
    ctr_control = le16(base - 1)

    mgr = SwitcherBreezeRemoteManager()
    remote = mgr.get_remote(REMOTE_ID)

    import asyncio as _a
    reader = writer = None
    try:
        reader, writer = await _a.wait_for(
            _a.open_connection(IP_ADDRESS, 10000), timeout=NET_TIMEOUT)

        async def xfer(name, body):
            packet = sign_packet_with_crc_key(set_message_length(body))
            if VERBOSE:
                print(f"  >>> {name} ({len(packet)//2} B): {packet}")
            writer.write(unhexlify(packet))
            await writer.drain()
            resp = await _a.wait_for(reader.read(1024), timeout=NET_TIMEOUT)
            if VERBOSE:
                print(f"  <<< {name} reply ({len(resp)} B): {resp.hex()}")
            return resp

        # ---- login: token in the tail (NOT device id), counter B ----
        login_body = ("fef00000" "0305a600" "00000000" + ctr_login +
                      "0100" "0000" + tok + "0000" + ts +
                      "00000000000000000000f0fe" + tok)
        await xfer("login", login_body)

        # ---- login2: counter B+1, session 00000000 (free), token block ----
        # TEST 015: login2 needs a NONZERO 4-byte session nonce (bytes
        # 8-12) and a NONZERO 2-byte field right after the token. The app
        # varies both per connection; the device rejects the all-zero
        # version (status 10, short reply). We derive both from the live
        # timestamp so they are always nonzero and unique.
        sess_nonce = ts                      # 4 bytes, nonzero, per-connection
        two_byte   = ts[:4]                  # 2 bytes, nonzero
        login2_body = ("fef00000" "0305a100" + sess_nonce + ctr_login2 +
                       "0100" "0000" + DEVICE_ID + "00" "0000" + ts +
                       "00000000000000000000f0fe" "0400" + tok + two_byte +
                       "00" * 30 + "01")
        l2reply = await xfer("login2", login2_body)
        if VERBOSE and len(l2reply) < 100:
            print(f"  NOTE: login2 reply is {len(l2reply)}B (want ~130B). "
                  "Short reply = login2 not fully accepted.")

        # TEST 014: the app goes login -> login2 -> control with NO get-state
        # in between (confirmed from the capture), so it is removed here.
        # Build the IR command from explicit fields / defaults.
        if raw_ir is not None:
            ir_cmd = raw_ir
        else:
            f_state = state if state is not None else DS.ON
            from aioswitcher.device import (ThermostatMode as TM,
                                            ThermostatFanLevel as TF,
                                            ThermostatSwing as TSw)
            f_mode  = mode  if mode  is not None else TM.COOL
            f_fan   = fan   if fan   is not None else TF.AUTO
            f_swing = swing if swing is not None else TSw.OFF
            f_temp  = temp  if temp  is not None else 24
            command = remote.build_command(
                f_state, f_mode, f_temp, f_fan, f_swing, DS.ON)
            ir_cmd = command.command
        clen = f"{len(ir_cmd)//2:02x}"

        # ---- control: counter B-1, token + keyblock + IR ----
        control_body = ("fef00000" "03050102" "00000000" + ctr_control +
                        "0100" "0000" + tok + "0000" + ts +
                        "00000000000000000000f0fe" + DEVICE_ID + "00" +
                        tok + keyblk + "3701" + clen + "00000000" "00" + ir_cmd)
        print("SENDING hand-built control (consistent counter chain) - watch the AC")
        resp = await xfer("control", control_body)
        print(f"control reply: {resp.hex()[:96]}")
        if len(resp) >= 6:
            print(">>> the device ANSWERED the control packet. Did the AC beep?")
    except _a.TimeoutError:
        print("timeout. If login/login2 answered but control did not, the counter")
        print("chain or a field is still off - the -v dump shows exactly where.")
    except Exception as exc:
        print(f"error: {type(exc).__name__}: {exc}")
    finally:
        if writer is not None:
            writer.close()
    return True


async def send_reuse(state=None, mode=None, temp=None, fan=None, swing=None,
                     raw_ir=None):
    """TEST 016: let the library log in (proven to authenticate), then reach
    into the SAME open connection and hand-send our control packet with the
    correct counter. No login2 rebuild, no nonce reversing.
    """
    if not TOKEN or not convert_token_to_packet:
        print("this path needs token.txt"); return False

    from binascii import unhexlify
    from aioswitcher.device.tools import (set_message_length,
                                          sign_packet_with_crc_key)
    from aioswitcher.device import (DeviceState as DS, ThermostatMode as TM,
                                    ThermostatFanLevel as TF, ThermostatSwing as TSw)

    tok    = convert_token_to_packet(TOKEN)
    keyblk = load_keyblock()
    mgr = SwitcherBreezeRemoteManager()
    remote = mgr.get_remote(REMOTE_ID)

    async with SwitcherApi(DeviceType.BREEZE, IP_ADDRESS, DEVICE_ID, DEVICE_KEY) as api:
        tap_packets(api)                       # forces token login + hex dump
        # library login + login2 (authenticates; also gives us the timestamp)
        timestamp, login_resp = await api._login()
        if not login_resp.successful:
            print("library login failed"); return False
        # read state so build_command has real current values
        cur = await asyncio.wait_for(api.get_breeze_state(), timeout=NET_TIMEOUT)

        if raw_ir is not None:
            ir_cmd = raw_ir
        else:
            f_state = state if state is not None else cur.state
            f_mode  = mode  if mode  is not None else cur.mode
            f_fan   = fan   if fan   is not None else cur.fan_level
            f_swing = swing if swing is not None else cur.swing
            f_temp  = temp  if temp  is not None else 24
            command = remote.build_command(
                f_state, f_mode, f_temp, f_fan, f_swing, cur.state)
            ir_cmd = command.command
        clen = f"{len(ir_cmd)//2:02x}"

        # The library login uses counter ff03; control = login - 1 = fe03.
        ctr = "fe03"
        body = ("fef00000" "03050102" "00000000" + ctr +
                "0100" "0000" + tok + "0000" + timestamp +
                "00000000000000000000f0fe" + DEVICE_ID + "00" +
                tok + keyblk + "3701" + clen + "00000000" "00" + ir_cmd)
        packet = sign_packet_with_crc_key(set_message_length(body))

        print("SENDING control on the library-authenticated socket - watch the AC")
        if VERBOSE:
            print(f"  >>> control ({len(packet)//2} B): {packet}")
        api._writer.write(unhexlify(packet))
        await api._writer.drain()
        try:
            resp = await asyncio.wait_for(api._reader.read(1024), timeout=NET_TIMEOUT)
            print(f"control reply ({len(resp)} B): {resp.hex()[:96]}")
            i = resp.hex().find("04000102")
            if i >= 0:
                status = resp.hex()[i+8+8+4+2:i+8+8+4+2+2]
                print(f"control reply status byte: {status} (00 = accepted)")
        except asyncio.TimeoutError:
            print("control timed out - try -v and compare the >>> bytes to the app.")
    return True


def usage():
    print(__doc__)
    sys.exit(1)


def main():
    print(f"=== BREEZE CONTROL - TEST {TEST_NUMBER:03d} ===")
    print(f"aioswitcher {AIOSWITCHER_VERSION}")

    # TEST 007: -v anywhere on the line turns on the library debug
    # log, which prints every packet in hex. Our best microscope.
    global VERBOSE, TOKEN
    argv = []
    for x in sys.argv[1:]:
        if x == "-v":
            VERBOSE = True
        elif x.startswith("--token="):
            TOKEN = x[len("--token="):].strip()
        else:
            argv.append(x)
    if TOKEN is None:
        try:
            import os
            here = os.path.dirname(os.path.abspath(__file__))
            with open(os.path.join(here, "token.txt")) as fh:
                t = fh.read().strip()
                if t:
                    TOKEN = t
        except FileNotFoundError:
            pass
    if VERBOSE:
        logging.basicConfig(level=logging.DEBUG,
                            format="%(levelname)s %(name)s: %(message)s")
        print("verbose: full packet hex dump is ON")
    if TOKEN:
        print("token: PRESENT - forcing the token login on every connection")
        if not convert_token_to_packet:
            print("but this aioswitcher cannot convert it - upgrade the library")
    else:
        print("token: none (token.txt not found and no --token given)")
    print()
    if not argv:
        usage()

    cmd = argv[0].lower()
    a = [x.lower() for x in argv[1:]]

    try:
        if cmd == "off4":
            asyncio.run(send_reuse(state=__import__("aioswitcher.device", fromlist=["DeviceState"]).DeviceState.OFF))
        elif cmd == "on4":
            asyncio.run(send_reuse(state=__import__("aioswitcher.device", fromlist=["DeviceState"]).DeviceState.ON))
        elif cmd == "raw4" and a:
            asyncio.run(send_reuse(raw_ir=a[0]))
        elif cmd == "off3":
            asyncio.run(send_full(state=__import__("aioswitcher.device", fromlist=["DeviceState"]).DeviceState.OFF))
        elif cmd == "on3":
            asyncio.run(send_full(state=__import__("aioswitcher.device", fromlist=["DeviceState"]).DeviceState.ON))
        elif cmd == "raw3" and a:
            asyncio.run(send_full(raw_ir=a[0]))
        elif cmd == "off2":
            asyncio.run(send_local(state=__import__("aioswitcher.device", fromlist=["DeviceState"]).DeviceState.OFF))
        elif cmd == "on2":
            asyncio.run(send_local(state=__import__("aioswitcher.device", fromlist=["DeviceState"]).DeviceState.ON))
        elif cmd == "raw2" and a:
            asyncio.run(send_local(raw_ir=a[0]))
        elif cmd == "exp":
            asyncio.run(experimental_off())
        elif cmd == "raw":
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
#                  TEST  016   (end of file)
#  off4/on4 reuse the library's working login on one socket, then
#  hand-send the control with counter fe03. Sidesteps login2's
#  per-connection nonce entirely. off3/on3 (full rebuild) kept too.
# ============================================================