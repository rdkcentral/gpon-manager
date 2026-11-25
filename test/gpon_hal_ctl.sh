#!/bin/sh
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
#
# gpon_hal_ctl - drive GponManager from the command line through the mock
# GPON JSON HAL (gpon_hal_mock), then check TR-181 (dmcli/rbus) values and the
# "[T2]" telemetry lines in the GPON log.
#
# LAB USE ONLY: "start" replaces the vendor GPON HAL, so the PON WAN is down
# until "stop" is run.
#
# Typical session (as root on the device):
#   gpon_hal_ctl start                      # GponManager now runs on the mock HAL
#   gpon_hal_ctl mark                       # remember the GPON log position
#   gpon_hal_ctl alarm LOS raise            # HAL publishes ...PhysicalMedia.1.Alarm.LOS=Active
#   gpon_hal_ctl t2                         # [T2] CRITICAL XPON_ALARM_LOS RAISED,PM=1
#   gpon_hal_ctl get Device.XPON.ONU.1.ANI.1.TC.Alarms.LOS        # true
#   gpon_hal_ctl expect-t2 XPON_ALARM_LOS RAISED,PM=1             # exit 0 = PASS
#   gpon_hal_ctl alarm LOS clear
#   gpon_hal_ctl ploam O5                   # ONU registered
#   gpon_hal_ctl veip admin unlock          # EthernetUNI.1.Enable -> true
#   gpon_hal_ctl stop                       # back to the vendor HAL
#
# The full automated test is "gpon_hal_ctl test" (run_gpon_hal_test.sh). It
# reuses an environment started with "gpon_hal_ctl start" and leaves it running.
#
# HAL names may be given without the "Device.X_RDK_ONT." prefix, e.g.
# "PhysicalMedia.1.Temperature.CurrentTemp".

SELF_DIR=$(cd "$(dirname "$0")" && pwd)
LIB=""
for c in "$SELF_DIR/gpon_hal_test_lib.sh" /usr/share/gponmanager-test/gpon_hal_test_lib.sh; do
    [ -f "$c" ] && { LIB=$c; break; }
done
[ -n "$LIB" ] || { echo "ERROR: gpon_hal_test_lib.sh not found" >&2; exit 2; }
# shellcheck source=gpon_hal_test_lib.sh
. "$LIB"
LIB_DIR=$(dirname "$LIB")

usage() {
    cat <<EOF
Usage: gpon_hal_ctl <command> [args]

Environment (LAB ONLY - the PON WAN is down while the mock is in use):
  start [-y]                     Stop GponManager and the vendor HAL, start the mock
                                 HAL and restart GponManager on it (waits for
                                 XPON_SYSTEM_INIT_SUCCESS). -y: no confirmation.
  stop                           Stop the mock, restore the vendor HAL, restart GponManager.
  status                         Mock / GponManager / stub state and HAL subscriptions.
  restart-gpon                   Restart GponManager on the mock (start-up events).

HAL events (store + publish, as the vendor HAL does on a change):
  alarm <NAME|all> raise|clear [pm]
                                 NAME: $ALARMS
                                 -> XPON_ALARM_<NAME> RAISED|CLEARED,PM=<pm>,
                                    Device.XPON.ONU.1.ANI.1.TC.Alarms.<NAME>
  ploam <O1..O7>                 Ploam.RegistrationState -> XPON_ONU_STATE_CHANGE (+ LOS,
                                 REGISTRATION, DEREGISTRATION, EMERGENCY_STOP, ...)
  veip admin lock|unlock [n]     Veip.<n>.AdministrativeState -> XPON_INTF_ADMIN_LOCK|UNLOCK
  veip oper up|down [n]          Veip.<n>.OperationalState -> link state machine
                                 (XPON_INTF_LINK_UP|DOWN, XPON_PHY_STATUS_UP|DOWN)
  pm-status <Up|Down|...> [pm]   PhysicalMedia.<pm>.Status event
  event <name> <value>           Any HAL event (store + publish).
  publish <name> <value>         Publish only, the store keeps its value.

HAL values (store only; GponManager reads them on its next poll):
  set <name> <value>             e.g. set PhysicalMedia.1.Temperature.CurrentTemp 612
  ponmode <mode> [pm]            PhysicalMedia.<pm>.PonMode (EPON -> XPON_SYSTEM_HAL_WRONG_PON_MODE)
  poll                           Make GponManager poll PhysicalMedia now (dmcli getv X_RDK).

Fault injection (mock replies):
  fail get|set|subscribe on|off  Reply with a failure (set -> XPON_ERROR_HAL_CALL_FAILED).
  hang get|set|subscribe <n>     Delay the next <n> requests past the 10 s client timeout.
  hangtime <seconds>             Delay used by "hang" (default 12).

Inspect:
  get <param...>                 dmcli getv (TR-181 values: Device.XPON.*, Device.X_RDK_ONT.*).
  dump [prefix]                  Values in the mock HAL store.
  subs                           Event subscriptions GponManager made.
  mocklog [lines]                Tail of the mock log ($MOCK_LOG).
  mark                           Remember the GPON log position for t2/log/expect-*.
  t2 [-a]                        "[T2]" lines since the mark (-a: since "start").
  log [regex]                    GPON log lines since the mark (optionally filtered).

Verify (exit status 0 = PASS, 1 = FAIL; use after "mark" + a trigger):
  expect-dm <param> <value>      TR-181 value equals <value>.
  expect-t2 <marker> [value] [timeout]
                                 "[T2] <PRIO> <marker> [value]" logged since the mark with
                                 the priority from the telemetry table (default 10 s).
  expect-no-t2 <marker> [seconds]
                                 Marker NOT logged since the mark (waits, default 3 s).

Full test:
  test [run_gpon_hal_test.sh options]
                                 Run the automated test (-l lists scenarios, -s picks some).

Files: $WORK_DIR (mock log, control FIFO, marks). Overrides: VEIP_IF=$VEIP_IF,
GPON_LOG=$GPON_LOG, HAL_CONF, MOCK_BIN, MOCK_DATA, WORK_DIR.
EOF
}

need_root() { [ "$(id -u)" = 0 ] || die "run as root"; }
need_env()  { mock_running || die "mock HAL is not running; run 'gpon_hal_ctl start' first"; }
load_mark() { LOG_MARK=$(cat "$CLI_MARK_FILE" 2>/dev/null || echo 0); }

check_alarm() {
    for a in $ALARMS; do [ "$a" = "$1" ] && return 0; done
    die "unknown alarm '$1' (one of: $ALARMS)"
}

cmd_start() {
    need_root
    resolve_paths "$LIB_DIR"
    if env_active; then
        log "mock HAL already running (pid $(mock_pid))"
        return 0
    fi
    if [ "$1" != "-y" ]; then
        echo "This stops GponManager and the vendor GPON HAL (PON WAN goes down). Continue? [y/N]"
        read -r answer
        [ "$answer" = y ] || [ "$answer" = Y ] || exit 0
    fi
    env_setup
    log "starting GponManager on the mock HAL (up to 90 s)"
    if gpon_restart_on_mock 90; then
        log "GponManager is up: $(subs_count) HAL subscriptions"
        log_size > "$CLI_MARK_FILE"
        log "ready - try: gpon_hal_ctl alarm LOS raise; gpon_hal_ctl t2"
    else
        log "XPON_SYSTEM_INIT_SUCCESS not seen; check '$GPON_LOG' and 'gpon_hal_ctl mocklog'"
        return 1
    fi
}

cmd_stop() {
    need_root
    if ! env_active && [ ! -f "$MOUNTS" ]; then
        log "mock environment is not active"
        return 0
    fi
    env_teardown
}

cmd_status() {
    if env_active; then
        echo "mock HAL      : running (pid $(mock_pid)), log $MOCK_LOG"
    else
        echo "mock HAL      : not running"
    fi
    echo "GponManager   : $(pidof GponManager 2>/dev/null || echo 'not running')"
    echo "vendor HAL    : $(pidof $VENDOR_HAL_NAME 2>/dev/null || echo 'not running')"
    echo "stubbed paths : $(cat "$MOUNTS" 2>/dev/null | tr '\n' ' ')"
    if [ -f "$MOCK_LOG" ]; then
        echo "subscriptions : $(subs_count) (expected $EXPECTED_SUBS)"
        sed -n 's/.*MOCK SUBSCRIBE \(.*\) type=.*/  \1/p' "$MOCK_LOG" | sort -u
    fi
}

cmd_alarm() {
    [ $# -ge 2 ] || die "usage: alarm <NAME|all> raise|clear [pm]"
    case $2 in
        raise) v=Active;   t2=RAISED ;;
        clear) v=Inactive; t2=CLEARED ;;
        *) die "usage: alarm <NAME|all> raise|clear [pm]" ;;
    esac
    pm=${3:-1}
    if [ "$1" = all ]; then
        list=$ALARMS
    else
        check_alarm "$1"
        list=$1
    fi
    for a in $list; do
        hal_event "PhysicalMedia.$pm.Alarm.$a" "$v" && log "PhysicalMedia.$pm.Alarm.$a=$v published (expect XPON_ALARM_$a $t2,PM=$pm)"
    done
}

cmd_veip() {
    [ $# -ge 2 ] || die "usage: veip admin lock|unlock [n] | veip oper up|down [n]"
    n=${3:-1}
    case $1:$2 in
        admin:lock)   hal_event "Veip.$n.AdministrativeState" Lock ;;
        admin:unlock) hal_event "Veip.$n.AdministrativeState" Unlock ;;
        oper:up)      hal_event "Veip.$n.OperationalState" Up ;;
        oper:down)    hal_event "Veip.$n.OperationalState" Down ;;
        *) die "usage: veip admin lock|unlock [n] | veip oper up|down [n]" ;;
    esac && log "Veip.$n $1=$2 published"
}

cmd_t2() {
    if [ "$1" = "-a" ]; then
        since "$(cat "$ENV_MARK_FILE" 2>/dev/null || echo 0)"
    else
        load_mark
        since_mark
    fi | tr -d '\r' | grep -F "[T2]"
    return 0
}

cmd_log() {
    load_mark
    if [ -n "$1" ]; then
        since_mark | tr -d '\r' | grep -E "$1"
    else
        since_mark | tr -d '\r'
    fi
    return 0
}

cmd_get() {
    [ $# -ge 1 ] || die "usage: get <param...>"
    for p in "$@"; do
        if dm_ok "$p"; then
            echo "$p = '$(dm_get "$p")'"
        else
            echo "$p : get FAILED"
        fi
    done
}

cmd_expect_dm() {
    [ $# -ge 2 ] || die "usage: expect-dm <param> <value>"
    got=$(dm_get "$1")
    if [ "$got" = "$2" ]; then
        echo "PASS $1 = '$got'"
    elif ! dm_ok "$1"; then
        echo "FAIL $1 get failed (expected '$2')"; return 1
    else
        echo "FAIL $1 = '$got', expected '$2'"; return 1
    fi
}

cmd_expect_t2() {
    [ $# -ge 1 ] || die "usage: expect-t2 <marker> [value] [timeout]"
    load_mark
    want=$(marker_prio "$1")
    [ -n "$want" ] || echo "note: $1 is not in the telemetry table"
    if line=$(t2_wait "$1" "$2" "${3:-10}"); then
        prio=$(t2_line_prio "$line")
        if [ -z "$want" ] || [ "$prio" = "$want" ]; then
            echo "PASS $line"
        else
            echo "FAIL $1 logged as $prio, expected $want: $line"; return 1
        fi
    else
        echo "FAIL [T2] $1${2:+ $2} not logged within ${3:-10}s since the mark"; return 1
    fi
}

cmd_expect_no_t2() {
    [ $# -ge 1 ] || die "usage: expect-no-t2 <marker> [seconds]"
    load_mark
    sleep "${2:-3}"
    n=$(t2_count "$1")
    if [ "$n" -eq 0 ]; then
        echo "PASS no $1 since the mark"
    else
        echo "FAIL $1 logged $n time(s) since the mark"; return 1
    fi
}

cmd_test() {
    for c in "$LIB_DIR/run_gpon_hal_test.sh" "$SELF_DIR/run_gpon_hal_test.sh"; do
        [ -f "$c" ] && exec sh "$c" "$@"
    done
    die "run_gpon_hal_test.sh not found"
}

[ $# -ge 1 ] || { usage; exit 2; }
cmd=$1
shift
case $cmd in
    start)        cmd_start "$@" ;;
    stop)         cmd_stop ;;
    status)       cmd_status ;;
    restart-gpon) need_root; need_env; gpon_restart_on_mock 90 && log "GponManager restarted on the mock ($(subs_count) subscriptions so far)" ;;
    alarm)        need_env; cmd_alarm "$@" ;;
    ploam)        need_env; [ -n "$1" ] || die "usage: ploam <O1..O7>"; hal_event Ploam.RegistrationState "$1" && log "Ploam.RegistrationState=$1 published" ;;
    veip)         need_env; cmd_veip "$@" ;;
    pm-status)    need_env; [ -n "$1" ] || die "usage: pm-status <Up|Down|...> [pm]"; hal_event "PhysicalMedia.${2:-1}.Status" "$1" && log "PhysicalMedia.${2:-1}.Status=$1 published" ;;
    event)        need_env; [ $# -ge 2 ] || die "usage: event <name> <value>"; hal_event "$1" "$2" && log "$(hal_name "$1")=$2 published" ;;
    publish)      need_env; [ $# -ge 2 ] || die "usage: publish <name> <value>"; mock publish "$(hal_name "$1")" "$2" && log "$(hal_name "$1")=$2 published (store unchanged)" ;;
    set)          need_env; [ $# -ge 2 ] || die "usage: set <name> <value>"; hal_set "$1" "$2" && log "$(hal_name "$1")=$2 stored (seen on the next poll; 'gpon_hal_ctl poll')" ;;
    ponmode)      need_env; [ -n "$1" ] || die "usage: ponmode <mode> [pm]"; hal_set "PhysicalMedia.${2:-1}.PonMode" "$1" && log "PonMode=$1 stored (seen on the next poll)" ;;
    poll)         need_env; poll_pm; log "PhysicalMedia polled" ;;
    fail)         need_env; [ $# -ge 2 ] || die "usage: fail get|set|subscribe on|off"; mock fail "$1" "$2" && log "fail $1 $2" ;;
    hang)         need_env; [ $# -ge 2 ] || die "usage: hang get|set|subscribe <n>"; mock hang "$1" "$2" && log "hang $1 $2" ;;
    hangtime)     need_env; [ -n "$1" ] || die "usage: hangtime <seconds>"; mock hangtime "$1" && log "hangtime $1" ;;
    dump)         need_env; n=$(wc -l < "$MOCK_LOG")
                  if [ -n "$1" ]; then mock dump "$(hal_name "$1")"; else mock dump; fi
                  tail -n +"$((n + 1))" "$MOCK_LOG" | sed -n 's/.*MOCK DUMP //p' ;;
    subs)         [ -f "$MOCK_LOG" ] && sed -n 's/.*MOCK SUBSCRIBE \(.*\) type=.*/\1/p' "$MOCK_LOG" | sort -u ;;
    mocklog)      tail -n "${1:-40}" "$MOCK_LOG" ;;
    get)          cmd_get "$@" ;;
    mark)         mkdir -p "$WORK_DIR"; log_size > "$CLI_MARK_FILE"; log "GPON log mark set at byte $(cat "$CLI_MARK_FILE")" ;;
    t2)           cmd_t2 "$@" ;;
    log)          cmd_log "$@" ;;
    expect-dm)    cmd_expect_dm "$@" ;;
    expect-t2)    cmd_expect_t2 "$@" ;;
    expect-no-t2) cmd_expect_no_t2 "$@" ;;
    test)         cmd_test "$@" ;;
    help|-h|--help) usage ;;
    *) echo "unknown command '$cmd'" >&2; usage; exit 2 ;;
esac
