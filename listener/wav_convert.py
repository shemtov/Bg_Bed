#!/usr/bin/env python3
# ============================================================
#   Bg_Bed  -  listener  -  TEST 010  -  log to wav
# ============================================================
#
#   WHAT CHANGED
#   ------------
#   Prints a LEVEL TABLE, one row per second, showing both
#   channels and the difference between them.
#
#   That is what answers the attribution question.  A single
#   peak across the whole recording tells you nothing if you
#   moved between microphones - this shows each moment.
#
#   The gain now lives in the firmware, so this script only
#   normalises what is left, and usually will not need to.
#
#   USAGE
#   -----
#       python wav_convert.py
#
#   Run from the listener folder after a recording.

import array
import base64
import glob
import math
import os
import re
import sys
import wave

BEGIN = "-----BEGIN AUDIO-----"
END   = "-----END AUDIO-----"

TARGET_PEAK = 30000
MAX_GAIN    = 200.0
GOOD_DIFF   = 6.0          # dB needed for reliable side attribution


def newest_log():
    found = []
    for pattern in ("logs/*.log", "*.log", "logs/*.txt"):
        found.extend(glob.glob(pattern))
    return max(found, key=os.path.getmtime) if found else None


def dbfs(peak):
    return 20.0 * math.log10(peak / 32768.0) if peak > 0 else float("-inf")


def fmt(peak):
    return "  --  " if peak <= 0 else "%6.1f" % dbfs(peak)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else newest_log()

    if not path or not os.path.exists(path):
        print("No log file found.  Run this from the listener folder.")
        return 1

    print("Reading", path)

    with open(path, "r", errors="ignore") as fh:
        text = fh.read()

    if BEGIN not in text:
        print("No audio block in that file.")
        return 1

    body = text.split(BEGIN, 1)[1]
    if END in body:
        body = body.split(END, 1)[0]
    else:
        print("Warning: no END marker - the capture may be cut short.")

    lines = body.splitlines()

    rate, channels = 16000, 2
    head = next((l for l in lines[:5] if "rate=" in l), "")
    m = re.search(r"rate=(\d+)\s+channels=(\d+)", head)
    if m:
        rate, channels = int(m.group(1)), int(m.group(2))

    b64 = "".join(
        l.strip() for l in lines
        if l.strip() and re.fullmatch(r"[A-Za-z0-9+/=]+", l.strip())
    )
    b64 = b64[: len(b64) - (len(b64) % 4)]

    try:
        pcm = base64.b64decode(b64)
    except Exception as exc:
        print("Could not decode the audio:", exc)
        return 1

    if len(pcm) % (2 * channels):
        pcm = pcm[: len(pcm) - (len(pcm) % (2 * channels))]

    s = array.array("h")
    s.frombytes(pcm)

    frames  = len(s) // channels
    seconds = frames / float(rate)
    print("Decoded %.1f seconds, %d Hz, %d channels" % (seconds, rate, channels))

    if seconds < 1:
        print("Far too short - the capture did not complete.")
        return 1

    # ---- per-second table ----
    print()
    if channels == 2:
        print("  sec    LEFT   RIGHT    diff")
        print("  ---   -----   -----   -----")

        best_left = best_right = 0.0

        for sec in range(int(seconds)):
            a = sec * rate * channels
            b = min((sec + 1) * rate * channels, len(s))
            pl = pr = 0
            for i in range(a, b, 2):
                v = s[i];     v = -v if v < 0 else v
                if v > pl: pl = v
                v = s[i + 1]; v = -v if v < 0 else v
                if v > pr: pr = v

            if pl > 0 and pr > 0:
                d = dbfs(pl) - dbfs(pr)
                mark = ""
                if d >=  GOOD_DIFF: mark = "  L"
                if d <= -GOOD_DIFF: mark = "  R"
                if d > best_left:  best_left  = d
                if -d > best_right: best_right = -d
                print("  %3d  %s  %s  %6.1f%s" % (sec + 1, fmt(pl), fmt(pr), d, mark))
            else:
                print("  %3d  %s  %s" % (sec + 1, fmt(pl), fmt(pr)))

        print()
        print("  best separation towards LEFT  : %.1f dB" % best_left)
        print("  best separation towards RIGHT : %.1f dB" % best_right)
        if best_left >= GOOD_DIFF and best_right >= GOOD_DIFF:
            print("  Both sides cleared %.0f dB - attribution works." % GOOD_DIFF)
        else:
            print("  Below %.0f dB on at least one side." % GOOD_DIFF)
            print("  Either the mics are too close together, or the")
            print("  talking was not close enough to each one.")

    peak = max((-v if v < 0 else v) for v in s)
    if peak == 0:
        print("Digital silence.  Nothing reached the buffer.")
        return 1

    print()
    print("  overall peak %d  (%.1f dBFS)" % (peak, dbfs(peak)))

    gain = min(TARGET_PEAK / float(peak), MAX_GAIN)
    if gain > 1.05:
        print("  normalising by x%.1f" % gain)
        out = array.array("h")
        for v in s:
            x = int(v * gain)
            out.append(32767 if x > 32767 else (-32768 if x < -32768 else x))
        s = out
    else:
        print("  already a healthy level, no normalising needed")

    name = "dream.wav"
    with wave.open(name, "wb") as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(2)
        wf.setframerate(rate)
        wf.writeframes(s.tobytes())

    print()
    print("Wrote", os.path.abspath(name))
    return 0


if __name__ == "__main__":
    sys.exit(main())

# ============================================================
#   END  -  Bg_Bed  -  listener  -  TEST 010
# ============================================================
