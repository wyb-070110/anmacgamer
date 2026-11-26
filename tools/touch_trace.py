#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
"""Summarises a GS_TOUCH_TRACE file (the touch frames GameScrcpy sends), one line per gesture: how evenly the
moves are spaced, the speed over the last 100 ms before the lift, and the gap between the last move and the lift.
Android treats a lift more than 40 ms after the last move as a stopped finger (no fling; docs/handoff/swipe-design.md).

    GS_TOUCH_TRACE=/tmp/touch.log open build/mac-app/bin/anmacgamer.app   # then mirror and swipe
    tools/touch_trace.py /tmp/touch.log
    tools/touch_trace.py --size 1080x2400 /tmp/touch.log
    tools/touch_trace.py --self-test
"""
import math
import sys

DOWN, UP, MOVE, CANCEL = 0, 1, 2, 3
COORD_MAX = 65535.0
STOPPED_MS = 40.0
WINDOW_MS = 100.0


def parse(lines):
    """Trace lines are `<steady µs> <s|x> <pointer> <action> <x> <y>`; x means the frame was not sent."""
    events = []
    for line in lines:
        f = line.split()
        if len(f) != 6 or f[1] not in ("s", "x"):
            continue
        events.append((int(f[0]), f[1] == "s", int(f[2]), int(f[3]), int(f[4]), int(f[5])))
    return events


def gestures(events):
    """Splits the events into gestures, each one pointer from its down to its up or cancel."""
    open_, done = {}, []
    for t, sent, pid, action, x, y in events:
        if action == DOWN:
            open_[pid] = {"pointer": pid, "points": [(t, action, x, y)], "unsent": 0 if sent else 1}
            continue
        g = open_.get(pid)
        if g is None:
            continue
        g["points"].append((t, action, x, y))
        g["unsent"] += 0 if sent else 1
        if action in (UP, CANCEL):
            done.append(open_.pop(pid))
    return sorted(done, key=lambda g: g["points"][0][0])


def quantile(values, q):
    v = sorted(values)
    return v[int(q * (len(v) - 1))] if v else 0.0


def lift_speed(samples, t_end, scale):
    """Speed over the last 100 ms before the lift, following Android's VelocityTracker: ACTION_UP is not a sample,
    any > 40 ms gap resets history, and lifting > 40 ms after the last move zeroes the velocity."""
    if len(samples) < 2 or (t_end - samples[-1][0]) / 1000.0 > STOPPED_MS:
        return 0.0
    cutoff = t_end - WINDOW_MS * 1000.0
    first = len(samples) - 1
    while first > 0:
        prev, cur = samples[first - 1], samples[first]
        if prev[0] < cutoff or (cur[0] - prev[0]) / 1000.0 > STOPPED_MS:
            break
        first -= 1
    a, b = samples[first], samples[-1]
    span_s = (b[0] - a[0]) / 1e6
    if span_s <= 0:
        return 0.0
    dx = (b[2] - a[2]) / COORD_MAX * scale[0]
    dy = (b[3] - a[3]) / COORD_MAX * scale[1]
    return math.hypot(dx, dy) / span_s


def analyse(g, scale=(1.0, 1.0)):
    pts = g["points"]
    t0, t_end = pts[0][0], pts[-1][0]
    samples = [p for p in pts[:-1] if p[1] in (DOWN, MOVE)]
    moves = [p for p in samples if p[1] == MOVE]
    steps = [(b[0] - a[0]) / 1000.0 for a, b in zip(samples[:-1], samples[1:])]
    last = moves[-1] if moves else pts[0]
    lift_gap = (t_end - last[0]) / 1000.0
    cancelled = pts[-1][1] == CANCEL
    is_tap = not cancelled and len(moves) == 0
    stopped = not cancelled and len(moves) > 0 and lift_gap > STOPPED_MS
    return {
        "pointer": g["pointer"],
        "start_ms": t0 / 1000.0,
        "duration_ms": (t_end - t0) / 1000.0,
        "moves": len(moves),
        "step_p50": quantile(steps, 0.5),
        "step_p90": quantile(steps, 0.9),
        "step_max": max(steps, default=0.0),
        "speed": lift_speed(samples, t_end, scale) if not cancelled else 0.0,
        "lift_gap_ms": lift_gap,
        "tap": is_tap,
        "stopped": stopped,
        "cancelled": cancelled,
        "unsent": g["unsent"],
    }


def report(events, scale=(1.0, 1.0), unit="scr/s", out=sys.stdout):
    rows = [analyse(g, scale) for g in gestures(events)]
    if not rows:
        print("no complete gestures", file=out)
        return rows
    first = rows[0]["start_ms"]
    speed_col = f"{unit} last 100ms"
    print(f"{'at s':>7} {'ptr':>3} {'ms':>6} {'moves':>5} {'step p50/p90/max ms':>20} {speed_col:>16} "
          f"{'lift gap':>8}", file=out)
    for r in rows:
        steps = f"{r['step_p50']:.1f}/{r['step_p90']:.1f}/{r['step_max']:.1f}" if r["moves"] else "-"
        kind = "cancel" if r["cancelled"] else ("tap" if r["tap"] else ("stopped" if r["stopped"] else "fling"))
        speed_txt = f"{r['speed']:16.0f}" if unit == "px/s" else f"{r['speed']:16.2f}"
        extra = f" unsent {r['unsent']}" if r["unsent"] else ""
        print(f"{(r['start_ms'] - first) / 1000:7.2f} {r['pointer']:3d} {r['duration_ms']:6.0f} {r['moves']:5d} "
              f"{steps:>20} {speed_txt} {r['lift_gap_ms']:7.1f} {kind}{extra}", file=out)
    swipes = [r for r in rows if r["moves"] > 0 and not r["cancelled"]]
    stopped = sum(r["stopped"] for r in swipes)
    flings = len(swipes) - stopped
    taps = sum(r["tap"] for r in rows)
    tail = f", cancelled {sum(r['cancelled'] for r in rows)}" if any(r["cancelled"] for r in rows) else ""
    print(f"gestures {len(rows)}: swipes {len(swipes)} (fling {flings}, stopped > {STOPPED_MS:.0f} ms {stopped}), "
          f"taps {taps}{tail}", file=out)
    return rows


def parse_size(spec):
    parts = spec.lower().split("x")
    if len(parts) != 2 or not all(p.isdigit() and int(p) > 0 for p in parts):
        raise ValueError(f"expected WxH in pixels, got {spec!r}")
    return (float(parts[0]), float(parts[1]))


def self_test():
    import io
    # 1. A fling across 0.40 of a 1000x2000 screen in 20 moves x 8 ms = 160 ms (2.5 scr/s = 2500 px/s), lift +8 ms.
    step_x = round(COORD_MAX * 0.40 / 20)
    lines = [f"{1_000_000} s 0 {DOWN} 10000 32768"]
    for i in range(1, 21):
        lines.append(f"{1_000_000 + i * 8000} s 0 {MOVE} {10000 + i * step_x} 32768")
    lines.append(f"{1_000_000 + 21 * 8000} s 0 {UP} {10000 + 20 * step_x} 32768")
    # 2. A drag that pauses 80 ms before lifting in place (> 40 ms -> stopped, speed 0).
    base = 3_000_000
    lines += [f"{base} s 1 {DOWN} 0 0", f"{base + 16000} s 1 {MOVE} 0 10000", f"{base + 32000} s 1 {MOVE} 0 20000",
              f"{base + 112000} s 1 {UP} 0 20000"]
    # 3. A pure tap (0 moves, 25 ms) and an unsent cancelled pointer, plus malformed/orphan lines.
    lines += [f"{base + 200000} s 2 {DOWN} 32768 32768", f"{base + 225000} s 2 {UP} 32768 32768",
              f"{base + 1000} x 3 {DOWN} 5 5", f"{base + 2000} x 3 {CANCEL} 5 5",
              "garbage line", f"{base + 3000} s 7 {MOVE} 1 1"]

    rows = report(parse(lines), out=io.StringIO())
    assert len(rows) == 4, rows
    fling, drag, cancel, tap = rows
    assert fling["moves"] == 20 and fling["step_p50"] == 8.0 and fling["step_max"] == 8.0, fling
    assert abs(fling["speed"] - 2.5) < 0.02 and fling["lift_gap_ms"] == 8.0 and not fling["stopped"], fling
    assert drag["stopped"] and drag["lift_gap_ms"] == 80.0 and drag["speed"] == 0.0 and drag["step_p50"] == 16.0, drag
    assert cancel["cancelled"] and cancel["unsent"] == 2 and cancel["pointer"] == 3, cancel
    assert tap["tap"] and tap["moves"] == 0 and not tap["stopped"] and tap["speed"] == 0.0, tap

    px_rows = report(parse(lines), scale=parse_size("1000x2000"), unit="px/s", out=io.StringIO())
    assert abs(px_rows[0]["speed"] - 2500.0) < 10.0, px_rows[0]
    print("touch_trace self-test passed")


def main(argv):
    if argv == ["--self-test"]:
        self_test()
        return
    scale, unit = (1.0, 1.0), "scr/s"
    if len(argv) == 3 and argv[0] == "--size":
        scale, unit, argv = parse_size(argv[1]), "px/s", argv[2:]
    if len(argv) != 1:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    with open(argv[0]) as f:
        report(parse(f), scale=scale, unit=unit)


if __name__ == "__main__":
    main(sys.argv[1:])
