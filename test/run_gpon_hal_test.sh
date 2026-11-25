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
# GponManager on-target integration test driven by a mock JSON HAL server.
#
# Replaces the vendor json_hal_server_gpon with gpon_hal_mock, triggers HAL
# events / faults and verifies:
#   - T2 telemetry markers, values and priorities ("[T2]" lines in the GPON log),
#   - that every T2 event is printed in the GPON log (and nothing else is),
#   - Device.XPON (BBF) and Device.X_RDK_ONT TR-181 values via dmcli (rbus).
#
# LAB USE ONLY: stops GponManager and the vendor GPON HAL server, so the PON
# WAN is down while the test runs.
#
# How to run (as root on the device, with the rdkgponmanager-test package):
#   gpon_hal_ctl test -y                       # complete test, everything restored after
#   gpon_hal_ctl test -y -s init,alarm,checks  # selected scenarios
#   gpon_hal_ctl test -l                       # list scenarios
# or directly: sh /usr/share/gponmanager-test/run_gpon_hal_test.sh -y
#
# If the mock environment is already up ("gpon_hal_ctl start"), the test reuses
# it and leaves it running, so you can continue with gpon_hal_ctl commands
# afterwards; otherwise it sets it up and restores the vendor HAL on exit.
#
# Results: PASS/FAIL per check, a T2 marker coverage matrix and a summary line;
# exit status 0 when there is no FAIL. Files are kept in $WORK_DIR
# (results.txt, t2_lines.txt, coverage.txt, mock.log).
#
# Usage: run_gpon_hal_test.sh [-y] [-k] [-s <scenario list>] [-l]
#   -y  do not ask for confirmation
#   -k  keep the mock running and GponManager attached to it on exit
#       (continue with gpon_hal_ctl; "gpon_hal_ctl stop" restores)
#   -s  comma separated scenarios to run (default: all), see -l
#   -l  list scenarios
#
# Environment overrides: MOCK_BIN, MOCK_DATA, VEIP_IF, WORK_DIR, GPON_LOG, HAL_CONF

TEST_DIR=$(cd "$(dirname "$0")" && pwd)
LIB=""
for c in "$TEST_DIR/gpon_hal_test_lib.sh" /usr/share/gponmanager-test/gpon_hal_test_lib.sh; do
    [ -f "$c" ] && { LIB=$c; break; }
done
[ -n "$LIB" ] || { echo "ERROR: gpon_hal_test_lib.sh not found" >&2; exit 2; }
# shellcheck source=gpon_hal_test_lib.sh
. "$LIB"

RESULTS=$WORK_DIR/results.txt
T2_ALL=$WORK_DIR/t2_lines.txt

ALL_SCENARIOS="init,static,poll,ploam,veip,link,alarm,set,epon,restart,checks"
SCENARIOS=$ALL_SCENARIOS
ASSUME_YES=0
KEEP=0
REUSED=0

# ---------------------------------------------------------------------------
# Result bookkeeping
# ---------------------------------------------------------------------------
PASS=0
FAIL=0
SKIP=0
WARN=0
CURRENT=""
TEST_MARK=0

section() { CURRENT=$1; echo; echo "=== [$1] $2 ==="; }

record() {
    # record <PASS|FAIL|SKIP|WARN> <id> <text>
    status=$1; id=$2; shift 2
    case $status in
        PASS) PASS=$((PASS + 1)) ;;
        FAIL) FAIL=$((FAIL + 1)) ;;
        SKIP) SKIP=$((SKIP + 1)) ;;
        WARN) WARN=$((WARN + 1)) ;;
    esac
    printf '%-4s %-22s %s\n' "$status" "$CURRENT/$id" "$*" | tee -a "$RESULTS"
}

# expect_t2 <id> <marker> [value] [timeout]: wait for the marker after the mark
# and check that the logged priority matches the telemetry table.
expect_t2() {
    id=$1; marker=$2; value=$3
    if line=$(t2_wait "$marker" "$value" "${4:-10}"); then
        prio=$(t2_line_prio "$line")
        want=$(marker_prio "$marker")
        if [ "$prio" = "$want" ]; then
            record PASS "$id" "T2 $marker${value:+ $value} ($prio)"
        else
            record FAIL "$id" "T2 $marker priority '$prio', expected '$want'"
        fi
        return 0
    fi
    record FAIL "$id" "T2 $marker${value:+ $value} not logged within ${4:-10}s"
    return 1
}

# expect_no_t2 <id> <marker> [seconds]
expect_no_t2() {
    sleep "${3:-3}"
    n=$(t2_count "$2")
    if [ "$n" -eq 0 ]; then
        record PASS "$1" "no $2 (as expected)"
    else
        record FAIL "$1" "unexpected $2 x$n"
    fi
}

# expect_dm <id> <param> <expected value>
expect_dm() {
    got=$(dm_get "$2")
    if [ "$got" = "$3" ]; then
        record PASS "$1" "$2 = '$got'"
    elif ! dm_ok "$2"; then
        record FAIL "$1" "$2 get failed (expected '$3')"
    else
        record FAIL "$1" "$2 = '$got', expected '$3'"
    fi
}

# expect_dm_le <id> <param> <max>: numeric value <= max
expect_dm_le() {
    got=$(dm_get "$2")
    case $got in
        ''|*[!0-9]*) record FAIL "$1" "$2 = '$got', expected a number <= $3" ;;
        *) if [ "$got" -le "$3" ]; then record PASS "$1" "$2 = $got (<= $3)"; else record FAIL "$1" "$2 = $got, expected <= $3"; fi ;;
    esac
}

# expect_dm_rejected <id> <param>: the get must fail
expect_dm_rejected() {
    if dm_ok "$2"; then
        record FAIL "$1" "$2 = '$(dm_get "$2")', expected the get to be rejected"
    else
        record PASS "$1" "$2 get rejected (as expected)"
    fi
}

# gpon_restart_wait <id>: restart GponManager on the mock and record INIT_SUCCESS.
gpon_restart_wait() {
    if gpon_restart_on_mock 90; then
        record PASS "$1" "GponManager restarted on the mock ($(subs_count) subscriptions so far)"
        return 0
    fi
    record FAIL "$1" "XPON_SYSTEM_INIT_SUCCESS not logged within 90s after restart"
    return 1
}

RESTORED=0
restore() {
    [ "$RESTORED" = 1 ] && return
    RESTORED=1
    # Keep every [T2] line the test produced for the post-run review.
    since "$TEST_MARK" | tr -d '\r' | grep -F "[T2]" > "$T2_ALL" 2>/dev/null
    if [ "$KEEP" = 1 ] || [ "$REUSED" = 1 ]; then
        log "mock environment left running; use gpon_hal_ctl (status, alarm, t2, ...) and 'gpon_hal_ctl stop' to restore"
        return
    fi
    echo
    env_teardown
}

# ---------------------------------------------------------------------------
# Scenarios
# ---------------------------------------------------------------------------
sc_preconditions() {
    section pre "Preconditions"
    ini=/etc/debug.ini
    [ -f /nvram/debug.ini ] && ini=/nvram/debug.ini
    level=$(grep -E "^LOG\.RDK\.GPONMANAGER *=" "$ini" 2>/dev/null | tail -1 | sed 's/.*= *//')
    [ -z "$level" ] && level=$(grep -E "^LOG\.RDK\.DEFAULT *=" "$ini" 2>/dev/null | tail -1 | sed 's/.*= *//')
    case $level in
        *INFO*|*DEBUG*|*TRACE*) record PASS loglevel "GPON log level '$level' ($ini) prints [T2] INFO lines" ;;
        *) record WARN loglevel "GPON log level '$level' ($ini); [T2] lines are INFO and may be filtered" ;;
    esac
    if grep -q "t2_event_s" "$GPON_BIN" 2>/dev/null; then
        record PASS t2build "GponManager is linked with T2 (t2_event_s present)"
        T2_BUILD=1
    else
        record WARN t2build "GponManager built without --enable-telemetry2-0: events are only logged, not sent to T2"
        T2_BUILD=0
    fi
}

sc_init() {
    section init "Start-up on the mock HAL"
    gpon_stop
    mark
    gpon_start
    expect_t2 success XPON_SYSTEM_INIT_SUCCESS "" 90
    sleep 3
    subs="$X.PhysicalMedia.1.Status"
    for a in $ALARMS; do subs="$subs $X.PhysicalMedia.1.Alarm.$a"; done
    for sub in $subs \
               "$X.Veip.1.AdministrativeState" "$X.Veip.1.OperationalState" \
               "$X.Veip.2.AdministrativeState" "$X.Veip.2.OperationalState" \
               "$X.Ploam.RegistrationState"; do
        if grep -q "MOCK SUBSCRIBE $sub type=" "$MOCK_LOG"; then
            record PASS subscribe "$sub"
        else
            record FAIL subscribe "$sub not subscribed"
        fi
    done
    [ "$(t2_count XPON_SYSTEM_INIT_FAILURE)" -eq 0 ] && record PASS no-failure "no INIT_FAILURE" || record FAIL no-failure "INIT_FAILURE logged"
    [ "$(t2_count XPON_ERROR_STATS_COLLECTION_FAILED)" -eq 0 ] && record PASS no-stats-err "no STATS_COLLECTION_FAILED" || record FAIL no-stats-err "STATS_COLLECTION_FAILED logged"
}

sc_static() {
    section static "TR-181 values from the baseline HAL data"
    UP=$(echo "$VEIP_IF" | tr 'a-z' 'A-Z')
    expect_dm onu-count   Device.XPON.ONUNumberOfEntries 1
    expect_dm onu-enable  $XP.Enable true
    expect_dm onu-name    $XP.Name ONU1
    expect_dm ani-count   $XP.ANINumberOfEntries 1
    expect_dm ani-name    $ANI.Name ANI1
    expect_dm ani-mode    $ANI.PONMode XGS-PON
    expect_dm ani-status  $ANI.Status Up
    expect_dm ani-sfp     $ANI.SFPReferenceList ""
    expect_dm ani-enable  $ANI.Enable true
    expect_dm ani-alias   $ANI.Alias cpe-PON1
    expect_dm trx-count   $ANI.TransceiverNumberOfEntries 1
    expect_dm trx-vendor  $TRX.VendorName ACME
    expect_dm trx-pn      $TRX.VendorPartNumber XF10-MOCK
    expect_dm trx-rev     $TRX.VendorRevision V1.0
    expect_dm trx-mode    $TRX.PONMode XGS-PON
    expect_dm trx-temp    $TRX.Temperature 45
    expect_dm trx-volt    $TRX.Voltage 3300
    expect_dm trx-rx      $TRX.RxPower -1850
    expect_dm trx-tx      $TRX.TxPower 250
    expect_dm act-onuid   $ANI.TC.ONUActivation.ONUID 5
    expect_dm act-vendor  $ANI.TC.ONUActivation.VendorID ALCL
    expect_dm act-serial  $ANI.TC.ONUActivation.SerialNumber ALCL1A2B3C4D
    expect_dm act-state   $ANI.TC.ONUActivation.ONUState O1
    expect_dm pm-fecbytes $ANI.TC.PM.PHY.CorrectedFECBytes 1000
    expect_dm pm-fectotal $ANI.TC.PM.PHY.TotalFECCodewords 500000
    expect_dm gem-count   $ANI.TC.GEM.PortNumberOfEntries 1
    expect_dm gem-portid  $ANI.TC.GEM.Port.1.PortID 1024
    expect_dm alarm-los   $ANI.TC.Alarms.LOS false
    expect_dm uni-count   $XP.EthernetUNINumberOfEntries 1
    expect_dm uni-name    $UNI.Name "$VEIP_IF"
    expect_dm uni-alias   $UNI.Alias "cpe-$UP"
    expect_dm uni-descr   $UNI.Description "GPON VEIP WAN interface cpe-$UP"
    expect_dm uni-upstr   $UNI.Upstream true
    expect_dm uni-lower   $UNI.LowerLayers ""
    expect_dm uni-enable  $UNI.Enable false
    expect_dm uni-status  $UNI.Status Down
    expect_dm uni-idname  $UNI.InterdomainName vlan_wan
    expect_dm uni-idid    $UNI.InterdomainID "(VEIP,1025)"
    expect_dm uni-anis    $UNI.ANIs Device.XPON.ONU.1.ANI.1
    if [ -d "/sys/class/net/$VEIP_IF" ]; then
        # Live counter: the TR-181 value must be at least the sysfs value read just before.
        sysfs=$(cat "/sys/class/net/$VEIP_IF/statistics/tx_bytes")
        got=$(dm_get "$UNI.Stats.BytesSent")
        case $got in
            ''|*[!0-9]*) record FAIL uni-stats "$UNI.Stats.BytesSent = '$got' (sysfs $sysfs)" ;;
            *) if [ "$got" -ge "$sysfs" ]; then record PASS uni-stats "$UNI.Stats.BytesSent = $got (sysfs $sysfs)";
               else record FAIL uni-stats "$UNI.Stats.BytesSent = $got < sysfs $sysfs"; fi ;;
        esac
    else
        expect_dm_rejected uni-stats "$UNI.Stats.BytesSent"
    fi
    expect_dm xrdk-name   $X.PhysicalMedia.1.ModuleName XF10-MOCK
    expect_dm xrdk-ver    $X.PhysicalMedia.1.ModuleVersion V1.0
    expect_dm xrdk-state  $X.Ploam.RegistrationState O1
    expect_dm xrdk-veip   $X.Veip.1.InterfaceName "$VEIP_IF"
    expect_dm ro-alias    $UNI.Alias "cpe-$UP"
    if dmcli eRT setv $UNI.Alias string changed 2>/dev/null | grep -q "Execution succeed"; then
        record FAIL ro-alias "EthernetUNI.Alias accepted a set (must be read-only)"
    else
        record PASS ro-alias "EthernetUNI.Alias set rejected"
    fi
}

sc_poll() {
    section poll "Polled values (no HAL event)"
    hal_set PhysicalMedia.1.Temperature.CurrentTemp 612
    poll_pm
    expect_dm xrdk-temp $X.PhysicalMedia.1.Temperature.CurrentTemp 612
    expect_dm temp-round $TRX.Temperature 61
    hal_set PhysicalMedia.1.Temperature.CurrentTemp -55
    poll_pm
    expect_dm temp-neg $TRX.Temperature -6
    hal_set PhysicalMedia.1.Temperature.CurrentTemp 452
    poll_pm
    mark
    expect_no_t2 no-event XPON_ONU_STATE_CHANGE 1
}

# ploam_step <id> <from> <to> <expected markers...>
ploam_step() {
    id=$1; from=$2; to=$3; shift 3
    mark
    hal_event Ploam.RegistrationState "$to"
    expect_t2 "$id" XPON_ONU_STATE_CHANGE "From=$from,To=$to" 5
    for m in "$@"; do
        expect_t2 "$id" "$m" "" 5
    done
    expect_dm "$id" $ANI.TC.ONUActivation.ONUState "$to"
}

sc_ploam() {
    section ploam "PLOAM state transitions"
    ploam_step o1-o2 O1 O2 XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED
    ploam_step o2-o3 O2 O3
    ploam_step o3-o4 O3 O4
    ploam_step o4-o5 O4 O5 XPON_ONU_REGISTRATION
    mark
    hal_event Ploam.RegistrationState O5
    expect_no_t2 o5-o5 XPON_ONU_STATE_CHANGE 2
    ploam_step o5-o6 O5 O6 XPON_ONU_LOS XPON_ONU_DEREGISTRATION
    ploam_step o6-o5 O6 O5 XPON_ONU_REGISTRATION
    ploam_step o5-o7 O5 O7 XPON_ONU_DEREGISTRATION XPON_ONU_EMERGENCY_STOP
    ploam_step o7-o1 O7 O1 XPON_ONU_EMERGENCY_STOP_CLEARED
    ploam_step o1-o2b O1 O2 XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED
    ploam_step o2-o1 O2 O1 XPON_ONU_LOS
    ploam_step o1-o5 O1 O5 XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED XPON_ONU_REGISTRATION
}

sc_veip() {
    section veip "VEIP administrative state"
    mark
    hal_event Veip.1.AdministrativeState Unlock
    expect_t2 unlock XPON_INTF_ADMIN_UNLOCK "Interface=$VEIP_IF,Veip=1" 5
    expect_dm unlock-enable $UNI.Enable true
    mark
    hal_event Veip.1.AdministrativeState Unlock
    expect_no_t2 unlock-same XPON_INTF_ADMIN_UNLOCK 2
}

sc_link() {
    section link "Link state machine (needs WAN Manager)"
    [ "$(dm_get $X.Veip.1.AdministrativeState)" = "Unlock" ] || hal_event Veip.1.AdministrativeState Unlock
    mark
    hal_event Veip.1.OperationalState Up
    sleep 3
    if [ "$(t2_count XPON_ERROR_RBUS_PUBLISH_FAILED)" -gt 0 ]; then
        expect_t2 publish-fail XPON_ERROR_RBUS_PUBLISH_FAILED "" 1
        record SKIP link "WAN Manager rejected the PHY status publish; link events not checked"
        return
    fi
    expect_t2 up XPON_INTF_LINK_UP "Interface=$VEIP_IF" 10
    expect_t2 up XPON_PHY_STATUS_UP "" 5
    expect_dm up-status $UNI.Status Up
    expect_dm_le up-lastchange $UNI.LastChange 15

    mark
    hal_event Veip.1.OperationalState Down
    expect_t2 down XPON_INTF_LINK_DOWN "Interface=$VEIP_IF" 10
    expect_t2 down XPON_PHY_STATUS_DOWN "" 5
    expect_dm down-status $UNI.Status Down

    # LOS on the active PhysicalMedia must take the link down even when the VEIP is Up.
    mark
    hal_event Veip.1.OperationalState Up
    expect_t2 up2 XPON_INTF_LINK_UP "Interface=$VEIP_IF" 10
    mark
    hal_set PhysicalMedia.1.Alarm.LOS Active
    poll_pm
    expect_t2 los-down XPON_INTF_LINK_DOWN "Interface=$VEIP_IF" 10
    mark
    hal_set PhysicalMedia.1.Alarm.LOS Inactive
    poll_pm
    expect_t2 los-clear-up XPON_INTF_LINK_UP "Interface=$VEIP_IF" 10

    mark
    hal_event Veip.1.AdministrativeState Lock
    expect_t2 lock XPON_INTF_ADMIN_LOCK "Interface=$VEIP_IF,Veip=1" 5
    expect_t2 lock-down XPON_INTF_LINK_DOWN "Interface=$VEIP_IF" 10
    expect_t2 lock-down XPON_PHY_STATUS_DOWN "" 5
    expect_dm lock-enable $UNI.Enable false
    hal_event Veip.1.OperationalState Down
}

sc_alarm() {
    section alarm "PhysicalMedia alarms (HAL leaf events + TR-181)"
    for a in $ALARMS; do
        if grep -q "SUBSCRIBE $X.PhysicalMedia.1.Alarm.$a type=" "$MOCK_LOG"; then
            record PASS "$a-sub" "subscribed to $X.PhysicalMedia.1.Alarm.$a"
        else
            record FAIL "$a-sub" "no subscription for $X.PhysicalMedia.1.Alarm.$a (json-hal only delivers exact leaf names)"
        fi
        mark
        hal_event "PhysicalMedia.1.Alarm.$a" Active
        expect_t2 "$a-raise" "XPON_ALARM_$a" "RAISED,PM=1" 3
        poll_pm
        expect_dm "$a-tr181" "$ANI.TC.Alarms.$a" true
        mark
        hal_event "PhysicalMedia.1.Alarm.$a" Inactive
        expect_t2 "$a-clear" "XPON_ALARM_$a" "CLEARED,PM=1" 3
        poll_pm
        expect_dm "$a-tr181" "$ANI.TC.Alarms.$a" false
    done
}

sc_set() {
    section set "HAL set requests"
    p=$X.PhysicalMedia.1.RxPower.SignalLevelLowerThreshold
    mark
    if dmcli eRT setv $p int -2700 2>/dev/null | grep -q "Execution succeed"; then
        record PASS set-ok "setv $p -2700 accepted"
    else
        record FAIL set-ok "setv $p -2700 failed"
    fi
    sleep 1
    if grep -q "MOCK SET $p=-2700 -> ok" "$MOCK_LOG"; then
        record PASS set-hal "HAL received $p=-2700"
    else
        record FAIL set-hal "HAL did not receive $p=-2700"
    fi
    expect_no_t2 set-ok XPON_ERROR_HAL_CALL_FAILED 1

    mock fail set on
    mark
    if dmcli eRT setv $p int -2600 2>/dev/null | grep -q "Execution succeed"; then
        record FAIL set-fail "setv accepted although the HAL failed it"
    else
        record PASS set-fail "setv rejected when the HAL fails"
    fi
    expect_t2 set-fail XPON_ERROR_HAL_CALL_FAILED "" 5
    mock fail set off
    dmcli eRT setv $p int -2800 > /dev/null 2>&1
}

sc_epon() {
    section epon "Wrong PON mode (EPON module, latched)"
    mark
    hal_set PhysicalMedia.1.PonMode EPON
    poll_pm
    expect_t2 epon XPON_SYSTEM_HAL_WRONG_PON_MODE "" 5
    expect_dm epon-mode $ANI.PONMode Unknown
    mark
    poll_pm
    poll_pm
    expect_no_t2 epon-latched XPON_SYSTEM_HAL_WRONG_PON_MODE 1
    hal_set PhysicalMedia.1.PonMode XGS-PON
    poll_pm
    expect_dm epon-restore $ANI.PONMode XGS-PON
    mark
    hal_set PhysicalMedia.1.PonMode EPON
    poll_pm
    expect_t2 epon-again XPON_SYSTEM_HAL_WRONG_PON_MODE "" 5
    hal_set PhysicalMedia.1.PonMode XGS-PON
    poll_pm
}

sc_restart() {
    section restart "Start-up failures (GponManager restarts)"

    # 1. First HAL read times out -> STATS_COLLECTION_FAILED, init still succeeds.
    gpon_stop
    mock hang get 1
    mark
    gpon_start
    expect_t2 stats XPON_ERROR_STATS_COLLECTION_FAILED "" 60
    expect_t2 stats-init XPON_SYSTEM_INIT_SUCCESS "" 90
    mock hang get 0

    # 2. First event subscription times out -> HAL_EVENT_SUBSCRIBE_FAILED, init still succeeds.
    gpon_stop
    mock hang subscribe 1
    mark
    gpon_start
    expect_t2 subscribe XPON_ERROR_HAL_EVENT_SUBSCRIBE_FAILED "" 90
    expect_t2 subscribe-init XPON_SYSTEM_INIT_SUCCESS "" 30
    mock hang subscribe 0

    # 3. EPON module at start-up -> WRONG_PON_MODE + INIT_FAILURE.
    gpon_stop
    hal_set PhysicalMedia.1.PonMode EPON
    mark
    gpon_start
    expect_t2 epon-start XPON_SYSTEM_HAL_WRONG_PON_MODE "" 60
    expect_t2 epon-start XPON_SYSTEM_INIT_FAILURE "" 30
    gpon_stop
    hal_set PhysicalMedia.1.PonMode XGS-PON

    # 4. No HAL server -> HAL_CALL_FAILED + INIT_FAILURE (connect timeout is 10 s).
    stop_mock
    mark
    gpon_start
    expect_t2 no-hal XPON_ERROR_HAL_CALL_FAILED "" 60
    expect_t2 no-hal XPON_SYSTEM_INIT_FAILURE "" 30
    gpon_stop
    start_mock

    # 5. Clean start-up again; a VEIP already Unlocked at start-up is the seed, so no UNLOCK event.
    hal_set Veip.1.AdministrativeState Unlock
    gpon_restart_wait clean
    mark
    hal_event Veip.1.AdministrativeState Unlock
    expect_no_t2 seed XPON_INTF_ADMIN_UNLOCK 2
    hal_event Veip.1.AdministrativeState Lock
    hal_event Veip.1.OperationalState Down
}

sc_checks() {
    section checks "GPON log / T2 consistency"
    all=$WORK_DIR/t2_check.txt
    since "$TEST_MARK" | tr -d '\r' | grep -F "[T2]" > "$all"
    total=$(wc -l < "$all" | tr -d ' ')
    bad=0
    while read -r line; do
        prio=$(echo "$line" | sed -n 's/.*\[T2\] *\([A-Z]*\) *\([A-Z0-9_]*\).*/\1/p')
        marker=$(echo "$line" | sed -n 's/.*\[T2\] *\([A-Z]*\) *\([A-Z0-9_]*\).*/\2/p')
        want=$(marker_prio "$marker")
        if [ -z "$want" ]; then
            record FAIL t2-known "unknown marker in log: $line"; bad=$((bad + 1))
        elif [ "$want" != "$prio" ]; then
            record FAIL t2-prio "$marker logged as $prio, table says $want"; bad=$((bad + 1))
        fi
    done < "$all"
    [ "$bad" -eq 0 ] && record PASS t2-lines "$total [T2] lines, all known markers with the table priority"

    nfail=$(since "$TEST_MARK" | grep -c "t2 send failed")
    if [ "$nfail" -eq 0 ]; then
        record PASS t2-send "no 't2 send failed' warnings"
    else
        record FAIL t2-send "$nfail 't2 send failed' warnings"
    fi

    if [ "${T2_BUILD:-0}" = 1 ] && [ -f /rdklogs/logs/telemetry2_0.txt.0 ]; then
        log "T2 daemon log mentions: $(grep -oE 'XPON_[A-Z_]+' /rdklogs/logs/telemetry2_0.txt.0 2>/dev/null | sort -u | tr '\n' ' ')"
    fi
}

coverage_matrix() {
    echo
    echo "=== T2 marker coverage (GPON log, this run) ==="
    since "$TEST_MARK" | tr -d '\r' | grep -F "[T2]" > "$WORK_DIR/t2_cov.txt"
    printf '%-40s %-9s %-6s %s\n' MARKER PRIORITY COUNT STATUS
    echo "$T2_TABLE" | while IFS=: read -r marker prio how; do
        [ -z "$marker" ] && continue
        n=$(grep -cE "\[T2\] +[A-Z]+ +$marker( |[[:space:]]*\$)" "$WORK_DIR/t2_cov.txt")
        if [ "$n" -gt 0 ]; then
            status=COVERED
        else
            case $how in
                none) status="NOT TRIGGERABLE (static check only)" ;;
                cond) status="NOT SEEN (platform dependent)" ;;
                *)    status="NOT SEEN" ;;
            esac
        fi
        printf '%-40s %-9s %-6s %s\n' "$marker" "$prio" "$n" "$status"
    done | tee "$WORK_DIR/coverage.txt"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
while getopts "yks:lh" opt; do
    case $opt in
        y) ASSUME_YES=1 ;;
        k) KEEP=1 ;;
        s) SCENARIOS=$OPTARG ;;
        l) echo "$ALL_SCENARIOS" | tr ',' '\n'; exit 0 ;;
        *) sed -n '/^# GponManager on-target/,/^# Environment overrides/s/^#//p' "$0"; exit 2 ;;
    esac
done

[ "$(id -u)" = 0 ] || die "run as root"
command -v dmcli > /dev/null || die "dmcli not found"
resolve_paths "$TEST_DIR"

if [ "$ASSUME_YES" != 1 ]; then
    echo "This test stops GponManager and the vendor GPON HAL (PON WAN goes down). Continue? [y/N]"
    read -r answer
    [ "$answer" = y ] || [ "$answer" = Y ] || exit 0
fi

mkdir -p "$WORK_DIR"
: > "$RESULTS"
trap restore EXIT
trap 'exit 130' INT TERM

log "mock=$MOCK_BIN data=$MOCK_DATA veip=$VEIP_IF log=$GPON_LOG work=$WORK_DIR"
TEST_MARK=$(log_size)
sc_preconditions

if env_active; then
    # Started by "gpon_hal_ctl start": reload the baseline data, keep it running on exit.
    REUSED=1
    log "reusing the mock environment (pid $(mock_pid)); reloading the baseline HAL data"
    gpon_stop
    stop_mock
    start_mock
else
    env_setup
fi

# Without the init scenario GponManager still has to be running on the mock.
case ",$SCENARIOS," in
    *,init,*) ;;
    *) section setup "GponManager on the mock HAL"; gpon_restart_wait start ;;
esac

for s in $(echo "$SCENARIOS" | tr ',' ' '); do
    case $s in
        init)    sc_init ;;
        static)  sc_static ;;
        poll)    sc_poll ;;
        ploam)   sc_ploam ;;
        veip)    sc_veip ;;
        link)    sc_link ;;
        alarm)   sc_alarm ;;
        set)     sc_set ;;
        epon)    sc_epon ;;
        restart) sc_restart ;;
        checks)  sc_checks ;;
        *) log "unknown scenario '$s'" ;;
    esac
done

coverage_matrix
echo
echo "=== Summary: PASS=$PASS FAIL=$FAIL WARN=$WARN SKIP=$SKIP ==="
echo "Results: $RESULTS  Mock log: $MOCK_LOG  T2 lines: $T2_ALL"
[ "$FAIL" -eq 0 ]
