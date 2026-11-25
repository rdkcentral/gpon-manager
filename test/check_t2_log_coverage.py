#!/usr/bin/env python3
##########################################################################
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2025 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
"""Host-side static check of GponManager T2 telemetry.

Verifies, without a device:
  1. every event id in gponmgr_telemetry.h has a k_table entry
     (marker "XPON_<id suffix>", priority, value format);
  2. T2 is only called from t2_send(), and t2_send() writes the "[T2]" GPON
     log line before calling T2, so every T2 event is also printed in the log;
  3. every event id has a producer (a raise call or a mapper table entry) and
     every public raise API has a caller outside the telemetry module;
  4. the marker/priority table in test/gpon_hal_test_lib.sh matches k_table.

Optionally (--log FILE) checks a captured GPON log: every "[T2]" line uses a
known marker, the k_table priority and a value of the right format.

Exit status: 0 when every check passes, 1 otherwise.
"""

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HEADER = os.path.join(ROOT, "source/telemetry/gponmgr_telemetry.h")
MODULE = os.path.join(ROOT, "source/telemetry/gponmgr_telemetry.c")
RUNNER = os.path.join(ROOT, "test/gpon_hal_test_lib.sh")
SOURCE = os.path.join(ROOT, "source")

# Indirect producers: the id is raised by a telemetry API that maps HAL data.
INDIRECT = {
    "ALARM_": "xponMgr_telemetry_raise_alarms",
    "ONU_LOS": "xponMgr_telemetry_raise_onu_state",
    "ONU_DOWNSTREAM_SIGNAL_DETECTED": "xponMgr_telemetry_raise_onu_state",
    "ONU_REGISTRATION": "xponMgr_telemetry_raise_onu_state",
    "ONU_DEREGISTRATION": "xponMgr_telemetry_raise_onu_state",
    "ONU_EMERGENCY_STOP": "xponMgr_telemetry_raise_onu_state",
    "ONU_STATE_CHANGE": "xponMgr_telemetry_raise_onu_state",
    "ONU_EMERGENCY_STOP_CLEARED": "xponMgr_telemetry_raise_onu_state",
    "INTF_ADMIN_LOCK": "xponMgr_telemetry_raise_veip_admin",
    "INTF_ADMIN_UNLOCK": "xponMgr_telemetry_raise_veip_admin",
}

VALUE_FORMATS = {
    "FMT_NONE": r"",
    "FMT_INTF": r"Interface=\S+",
    "FMT_ALARM": r"(RAISED|CLEARED),PM=\d+",
    "FMT_ONU_STATE": r"From=O[1-9],To=O[1-9]",
    "FMT_VEIP": r"Interface=\S+,Veip=\d+",
}

failures = []


def check(ok, text):
    print(("PASS " if ok else "FAIL ") + text)
    if not ok:
        failures.append(text)
    return ok


def read(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        return handle.read()


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def function_body(text, name):
    """Return the body of C function @name (first definition), or None."""
    match = re.search(r"^[^\n;]*\b%s\s*\([^;{]*\)\s*\{" % re.escape(name), text, flags=re.M)
    if not match:
        return None
    depth, start = 0, match.end() - 1
    for pos in range(start, len(text)):
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
            if depth == 0:
                return text[start:pos + 1]
    return None


def c_sources():
    for base, _dirs, files in os.walk(SOURCE):
        for name in files:
            if name.endswith(".c"):
                yield os.path.join(base, name)


def parse_enum(header):
    body = re.search(r"typedef\s+enum\s*\{(.*?)\}\s*xponMgr_telemetry_event_id_t", header, flags=re.S)
    ids = re.findall(r"\b(XPON_TELEM_[A-Z0-9_]+)", body.group(1)) if body else []
    return [i for i in ids if i != "XPON_TELEM_EVENT_ID_MAX"]


def parse_k_table(module):
    body = re.search(r"k_table\s*\[[^\]]*\]\s*=\s*\{(.*?)\n\};", module, flags=re.S)
    table = {}
    if body:
        for ident, marker, prio, fmt in re.findall(
                r"\[(XPON_TELEM_[A-Z0-9_]+)\]\s*=\s*\{\s*\"([A-Z0-9_]+)\"\s*,\s*PRIO_([A-Z]+)\s*,\s*(FMT_[A-Z_]+)\s*\}",
                body.group(1)):
            table[ident] = (marker, prio, fmt)
    return table


def parse_runner_table(runner):
    body = re.search(r'T2_TABLE="(.*?)"', runner, flags=re.S)
    table = {}
    if body:
        for line in body.group(1).split():
            parts = line.split(":")
            if len(parts) == 3:
                table[parts[0]] = (parts[1], parts[2])
    return table


def check_table(ids, table):
    print("\n== 1. Event table ==")
    check(len(ids) > 0, "%d event ids in gponmgr_telemetry.h" % len(ids))
    missing = [i for i in ids if i not in table]
    check(not missing, "every event id has a k_table entry" + (": missing %s" % missing if missing else ""))
    extra = [i for i in table if i not in ids]
    check(not extra, "k_table has no unknown ids" + (": %s" % extra if extra else ""))
    bad = [i for i, (marker, _p, _f) in table.items() if marker != "XPON_" + i[len("XPON_TELEM_"):]]
    check(not bad, "marker names are XPON_<event id suffix>" + (": %s" % bad if bad else ""))
    markers = [m for m, _p, _f in table.values()]
    check(len(markers) == len(set(markers)), "marker names are unique")


def check_log_before_send(module):
    print("\n== 2. Every T2 event is printed in the GPON log ==")
    code = strip_comments(module)
    calls = [m.start() for m in re.finditer(r"\bt2_event_[sdf]\s*\(", code)]
    body = function_body(code, "t2_send")
    check(body is not None, "t2_send() found")
    if body is None:
        return
    start = code.find(body)
    outside = [pos for pos in calls if not start <= pos < start + len(body)]
    check(calls and not outside, "t2_event_*() is only called inside t2_send() (%d call(s))" % len(calls))
    log_pos = body.find("CcspTraceInfo")
    send_pos = min([p for p in (body.find("t2_event_s"), body.find("t2_event_d")) if p >= 0] or [-1])
    check(log_pos >= 0 and '"[T2] ' in body[log_pos:log_pos + 80], "t2_send() logs \"[T2] <prio> <marker> [value]\" with CcspTraceInfo")
    check(0 <= log_pos < send_pos, "the log line is written before the T2 call (logged even if T2 is off or fails)")
    early = re.search(r"\breturn\b", body[:log_pos]) if log_pos >= 0 else None
    guard = early and "marker == NULL" in body[:log_pos]
    check(early is None or guard, "no early return before the log line (except the NULL marker guard)")
    check("t2 send failed" in body, "a failed T2 send is logged (\"t2 send failed\")")

    dispatch = function_body(code, "dispatch")
    check(dispatch is not None and "t2_send" in dispatch, "dispatch() routes every event through t2_send()")
    others = [n for n in re.findall(r"\b(\w+)\s*\([^;]*\bt2_send\s*\(", code) if n not in ("dispatch", "t2_send")]
    check(not others, "only dispatch() calls t2_send()" + (": %s" % others if others else ""))


def check_producers(ids):
    print("\n== 3. Producers ==")
    external = {}
    for path in c_sources():
        if os.path.basename(path) == "gponmgr_telemetry.c":
            continue
        text = strip_comments(read(path))
        for ident in set(re.findall(r"\bXPON_TELEM_[A-Z0-9_]+", text)):
            external.setdefault(ident, []).append(os.path.relpath(path, ROOT))
        for api in set(INDIRECT.values()):
            if re.search(r"\b%s\s*\(" % api, text):
                external.setdefault(api, []).append(os.path.relpath(path, ROOT))
    module = strip_comments(read(MODULE))
    for ident in ids:
        suffix = ident[len("XPON_TELEM_"):]
        api = INDIRECT.get(suffix) or next((v for k, v in INDIRECT.items() if k.endswith("_") and suffix.startswith(k)), None)
        if ident in external:
            check(True, "%-45s raised from %s" % (ident, ", ".join(sorted(set(external[ident])))))
        elif api and api in external and len(re.findall(r"\b%s\b" % ident, module)) >= 2:
            check(True, "%-45s mapped by %s() (called from %s)" % (ident, api, ", ".join(sorted(set(external[api])))))
        else:
            check(False, "%-45s has no producer" % ident)


def check_runner(table, runner_table):
    print("\n== 4. test/gpon_hal_test_lib.sh marker table ==")
    expected = {marker: prio for marker, prio, _fmt in table.values()}
    got = {marker: prio for marker, (prio, _how) in runner_table.items()}
    check(set(expected) == set(got), "runner lists the %d k_table markers" % len(expected)
          + ("" if set(expected) == set(got) else ": missing %s extra %s"
             % (sorted(set(expected) - set(got)), sorted(set(got) - set(expected)))))
    wrong = sorted(m for m in expected if m in got and got[m] != expected[m])
    check(not wrong, "runner priorities match k_table" + (": %s" % wrong if wrong else ""))
    print("\nNot triggerable on target (static coverage only):")
    for marker, (_prio, how) in sorted(runner_table.items()):
        if how == "none":
            print("  - %s" % marker)
    print("Platform dependent (need WAN Manager to accept the PHY status publish):")
    for marker, (_prio, how) in sorted(runner_table.items()):
        if how == "cond":
            print("  - %s" % marker)


def check_log(path, table):
    print("\n== 5. GPON log %s ==" % path)
    by_marker = {marker: (prio, fmt) for marker, prio, fmt in table.values()}
    seen, bad, lines = {}, 0, 0
    pattern = re.compile(r"\[T2\] +([A-Z]+) +([A-Z0-9_]+)(?: (.*?))?\s*$")
    for line in read(path).splitlines():
        if "[T2]" not in line:
            continue
        lines += 1
        match = pattern.search(line)
        if not match or match.group(2) not in by_marker:
            bad += 1
            check(False, "unknown [T2] line: %s" % line.strip())
            continue
        prio, marker, value = match.group(1), match.group(2), match.group(3) or ""
        want_prio, fmt = by_marker[marker]
        if prio != want_prio or not re.fullmatch(VALUE_FORMATS[fmt], value):
            bad += 1
            check(False, "%s: priority %s (want %s), value '%s' (want %s)" % (marker, prio, want_prio, value, fmt))
        seen[marker] = seen.get(marker, 0) + 1
    check(bad == 0, "%d [T2] lines, all with a known marker, the k_table priority and a valid value" % lines)
    print("Markers seen: %d of %d" % (len(seen), len(by_marker)))
    for marker in sorted(by_marker):
        print("  %-40s %s" % (marker, seen.get(marker, 0)))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--log", help="GPON log (or t2_lines.txt from the runner) to validate")
    args = parser.parse_args()

    ids = parse_enum(read(HEADER))
    table = parse_k_table(read(MODULE))
    check_table(ids, table)
    check_log_before_send(read(MODULE))
    check_producers(ids)
    check_runner(table, parse_runner_table(read(RUNNER)))
    if args.log:
        check_log(args.log, table)

    print("\n%s: %d failure(s)" % ("FAILED" if failures else "PASSED", len(failures)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
