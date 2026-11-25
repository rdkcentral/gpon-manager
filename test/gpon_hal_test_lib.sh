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
# Shared helpers for the GponManager mock-HAL test tools. Sourced by:
#   gpon_hal_ctl            - interactive CLI (start the mock, raise alarms, ...)
#   run_gpon_hal_test.sh    - full automated test
#
# Both tools share the state below (WORK_DIR), so the environment started by
# "gpon_hal_ctl start" is reused by run_gpon_hal_test.sh and vice versa.
#
# How the test environment works:
#   1. GponManager is stopped and the vendor json_hal_server_gpon is killed.
#   2. A no-op script is bind-mounted over /bin|/usr/bin/json_hal_server_gpon,
#      because GponManager launches the vendor HAL server itself on start-up.
#   3. gpon_hal_mock listens on the same port (from the GponManager HAL conf)
#      and serves Device.X_RDK_ONT.* from gpon_hal_mock_data.txt.
#   4. GponManager is started again and talks to the mock.
#   5. Test tools drive the mock through its control FIFO and read back
#      TR-181 values with dmcli and "[T2]" lines from the GPON log.
#   Teardown reverses 1-4 (stop the mock, unmount the stubs, restart GponManager).
#
# Environment overrides: MOCK_BIN, MOCK_DATA, VEIP_IF, WORK_DIR, GPON_LOG, HAL_CONF

# shellcheck disable=SC2034  # variables are used by the scripts sourcing this file
VEIP_IF=${VEIP_IF:-veip0}
WORK_DIR=${WORK_DIR:-/tmp/gpon_hal_test}
GPON_LOG=${GPON_LOG:-/rdklogs/logs/GPONMANAGERLog.txt.0}
MOCK_BIN=${MOCK_BIN:-}
MOCK_DATA=${MOCK_DATA:-}
HAL_CONF=${HAL_CONF:-}
HAL_SCHEMA=""
SERVICE=RdkGponManager.service
VENDOR_HAL_NAME=json_hal_server_gpon
GPON_BIN=/usr/bin/GponManager
TEST_SHARE=/usr/share/gponmanager-test

FIFO=$WORK_DIR/mock.ctl
MOCK_LOG=$WORK_DIR/mock.log
MOCK_PIDFILE=$WORK_DIR/mock.pid
MOUNTS=$WORK_DIR/mounts.txt
ENV_MARK_FILE=$WORK_DIR/env.mark
CLI_MARK_FILE=$WORK_DIR/cli.mark

X=Device.X_RDK_ONT
XP=Device.XPON.ONU.1
ANI=$XP.ANI.1
TRX=$ANI.Transceiver.1
UNI=$XP.EthernetUNI.1

# PhysicalMedia alarm leaves in the GPON HAL schema (subscribeEventSupportedList).
ALARMS="LOS LOF SF SD LCDG TF SUF MEM DACT DIS MIS PEE RDI ROGUE"

# Subscriptions GponManager makes on start-up: per PhysicalMedia Status + 14
# alarm leaves, Admin/Oper state for each of the 2 VEIPs, PLOAM state.
EXPECTED_SUBS=20

# Marker table: marker:priority:triggerability. Kept in sync with k_table in
# source/telemetry/gponmgr_telemetry.c by test/check_t2_log_coverage.py.
#   auto  = triggered by run_gpon_hal_test.sh
#   cond  = triggered only when the platform allows it (see README)
#   none  = cannot be triggered black-box (see README)
T2_TABLE="
XPON_ONU_LOS:CRITICAL:auto
XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED:INFO:auto
XPON_ONU_REGISTRATION:INFO:auto
XPON_ONU_DEREGISTRATION:WARNING:auto
XPON_ONU_EMERGENCY_STOP:CRITICAL:auto
XPON_INTF_LINK_UP:INFO:cond
XPON_INTF_LINK_DOWN:WARNING:cond
XPON_PHY_STATUS_UP:INFO:cond
XPON_PHY_STATUS_DOWN:CRITICAL:cond
XPON_ALARM_LOS:CRITICAL:auto
XPON_ALARM_LOF:CRITICAL:auto
XPON_ALARM_SF:ERROR:auto
XPON_ALARM_SD:WARNING:auto
XPON_ALARM_LCDG:ERROR:auto
XPON_ALARM_TF:CRITICAL:auto
XPON_ALARM_SUF:ERROR:auto
XPON_ALARM_MEM:WARNING:auto
XPON_ALARM_DACT:ERROR:auto
XPON_ALARM_DIS:CRITICAL:auto
XPON_ALARM_MIS:ERROR:auto
XPON_ALARM_PEE:ERROR:auto
XPON_ALARM_RDI:WARNING:auto
XPON_ALARM_ROGUE:CRITICAL:auto
XPON_SYSTEM_INIT_SUCCESS:INFO:auto
XPON_SYSTEM_INIT_FAILURE:CRITICAL:auto
XPON_SYSTEM_SHUTDOWN:INFO:none
XPON_SYSTEM_HAL_WRONG_PON_MODE:CRITICAL:auto
XPON_ERROR_HAL_CALL_FAILED:ERROR:auto
XPON_ERROR_HAL_EVENT_SUBSCRIBE_FAILED:ERROR:auto
XPON_ERROR_HAL_EVENT_PARSE_FAILED:WARNING:none
XPON_ERROR_STATS_COLLECTION_FAILED:WARNING:auto
XPON_ERROR_RBUS_PUBLISH_FAILED:ERROR:cond
XPON_ERROR_RBUS_REGISTER_FAILED:ERROR:none
XPON_ERROR_STATE_MACHINE_START_FAILED:CRITICAL:none
XPON_ONU_STATE_CHANGE:INFO:auto
XPON_ONU_EMERGENCY_STOP_CLEARED:INFO:auto
XPON_INTF_ADMIN_LOCK:WARNING:auto
XPON_INTF_ADMIN_UNLOCK:INFO:auto
"

log() { echo "$(date '+%H:%M:%S') $*"; }
die() { echo "ERROR: $*" >&2; exit 2; }
nap() { sleep "$1" 2>/dev/null || sleep 1; }

# ---------------------------------------------------------------------------
# GPON log (byte offsets survive appends; a rotation makes the new file "new")
# ---------------------------------------------------------------------------
LOG_MARK=0

log_size() { if [ -f "$GPON_LOG" ]; then wc -c < "$GPON_LOG" | tr -d ' '; else echo 0; fi; }

mark() { LOG_MARK=$(log_size); }

# since <offset>: GPON log content appended after <offset>
since() {
    size=$(log_size)
    if [ "$size" -lt "$1" ]; then
        cat "$GPON_LOG" 2>/dev/null
    else
        tail -c +"$(($1 + 1))" "$GPON_LOG" 2>/dev/null
    fi
}

since_mark() { since "$LOG_MARK"; }

# t2_regex <marker> [value]: matches "[T2] <PRIO> <marker> <value>" at end of line;
# without a value it matches the marker with any (or no) value.
t2_regex() {
    if [ -n "$2" ]; then
        echo "\[T2\] +[A-Z]+ +$1 $2[[:space:]]*\$"
    else
        echo "\[T2\] +[A-Z]+ +$1( |[[:space:]]*\$)"
    fi
}

t2_count() { since_mark | tr -d '\r' | grep -cE "$(t2_regex "$1" "$2")"; }

marker_prio() { echo "$T2_TABLE" | grep "^$1:" | cut -d: -f2; }

# t2_wait <marker> [value] [timeout]: print the matching "[T2]" line logged after
# the mark; return 1 if it does not appear within <timeout> seconds.
t2_wait() {
    waited=0
    while [ "$waited" -le "${3:-10}" ]; do
        line=$(since_mark | tr -d '\r' | grep -E "$(t2_regex "$1" "$2")" | tail -1)
        [ -n "$line" ] && { echo "$line"; return 0; }
        sleep 1
        waited=$((waited + 1))
    done
    return 1
}

t2_line_prio() { echo "$1" | sed -n 's/.*\[T2\] *\([A-Z]*\) .*/\1/p'; }

# ---------------------------------------------------------------------------
# dmcli
# ---------------------------------------------------------------------------
dm_get() {
    dmcli eRT getv "$1" 2>/dev/null | tr -d '\r' | sed -n 's/.*value: *//p' | head -1 | sed 's/[[:space:]]*$//'
}

dm_ok() { dmcli eRT getv "$1" 2>/dev/null | grep -q "Execution succeed"; }

# ---------------------------------------------------------------------------
# Mock HAL
# ---------------------------------------------------------------------------
mock_pid() { cat "$MOCK_PIDFILE" 2>/dev/null; }

mock_running() {
    pid=$(mock_pid)
    [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null && [ -p "$FIFO" ]
}

# hal_name <name>: "PhysicalMedia.1.Status" -> "Device.X_RDK_ONT.PhysicalMedia.1.Status"
hal_name() {
    case $1 in
        Device.*) echo "$1" ;;
        *) echo "$X.$1" ;;
    esac
}

# mock <command...>: send a control command and wait until the mock processed it.
mock() {
    mock_running || { log "mock HAL is not running (gpon_hal_ctl start)"; return 1; }
    token="$$.$(date +%s).$(awk 'BEGIN{srand(); print int(rand()*100000)}')"
    echo "$*" > "$FIFO"
    echo "sync $token" > "$FIFO"
    i=0
    while ! grep -q "SYNC $token\$" "$MOCK_LOG"; do
        i=$((i + 1))
        [ $i -gt 50 ] && { log "mock did not acknowledge '$*'"; return 1; }
        nap 0.2
    done
    return 0
}

# hal_event <name> <value>: update the store and publish the HAL event, as the
# vendor HAL does when a value changes.
hal_event() { mock event "$(hal_name "$1")" "$2" && sleep 1; }

# hal_set <name> <value>: update the store only (GponManager sees it on the next poll).
hal_set() { mock set "$(hal_name "$1")" "$2"; }

# Force a PhysicalMedia HAL poll through the X_RDK DML (GponPhy_IsUpdated).
poll_pm() {
    before=$(grep -c "GET $X.PhysicalMedia\." "$MOCK_LOG")
    dm_get "$X.PhysicalMedia.1.PonMode" > /dev/null
    after=$(grep -c "GET $X.PhysicalMedia\." "$MOCK_LOG")
    [ "$after" -gt "$before" ] || log "note: X_RDK getv did not trigger a PhysicalMedia HAL poll"
}

subs_count() { grep -c "MOCK SUBSCRIBE .* type=" "$MOCK_LOG" 2>/dev/null; }

start_mock() {
    mkdir -p "$WORK_DIR"
    sed "s/@VEIP_IF@/$VEIP_IF/g" "$MOCK_DATA" > "$WORK_DIR/data.txt"
    : > "$MOCK_LOG"
    # nohup: the mock must outlive the gpon_hal_ctl shell / ssh session.
    if command -v nohup > /dev/null 2>&1; then
        nohup "$MOCK_BIN" -c "$HAL_CONF" -s "$HAL_SCHEMA" -d "$WORK_DIR/data.txt" -f "$FIFO" >> "$MOCK_LOG" 2>&1 &
    else
        "$MOCK_BIN" -c "$HAL_CONF" -s "$HAL_SCHEMA" -d "$WORK_DIR/data.txt" -f "$FIFO" >> "$MOCK_LOG" 2>&1 &
    fi
    echo $! > "$MOCK_PIDFILE"
    i=0
    while ! grep -q "MOCK READY" "$MOCK_LOG"; do
        i=$((i + 1))
        if [ $i -gt 20 ] || ! kill -0 "$(mock_pid)" 2>/dev/null; then
            cat "$MOCK_LOG"
            rm -f "$MOCK_PIDFILE"
            die "mock HAL did not start"
        fi
        sleep 1
    done
    log "mock HAL running (pid $(mock_pid), log $MOCK_LOG)"
}

stop_mock() {
    pid=$(mock_pid)
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
        kill "$pid" 2>/dev/null
        i=0
        while kill -0 "$pid" 2>/dev/null && [ $i -lt 10 ]; do sleep 1; i=$((i + 1)); done
        kill -9 "$pid" 2>/dev/null
    fi
    rm -f "$MOCK_PIDFILE"
}

# ---------------------------------------------------------------------------
# GponManager service and vendor HAL
# ---------------------------------------------------------------------------
gpon_stop() {
    systemctl stop "$SERVICE" 2>/dev/null
    i=0
    while pidof GponManager > /dev/null 2>&1 && [ $i -lt 15 ]; do sleep 1; i=$((i + 1)); done
    for pid in $(pidof GponManager 2>/dev/null); do kill -9 "$pid" 2>/dev/null; done
}

gpon_start() { systemctl start "$SERVICE" 2>/dev/null || systemctl restart "$SERVICE"; }

# gpon_restart_on_mock [timeout]: restart GponManager, wait for INIT_SUCCESS and
# for its HAL event subscriptions. Leaves the log mark at the restart.
gpon_restart_on_mock() {
    gpon_stop
    mark
    subs_before=$(subs_count)
    gpon_start
    t2_wait XPON_SYSTEM_INIT_SUCCESS "" "${1:-90}" > /dev/null || return 1
    i=0
    while [ $(($(subs_count) - subs_before)) -lt $EXPECTED_SUBS ] && [ $i -lt 30 ]; do
        sleep 1; i=$((i + 1))
    done
    sleep 2
    return 0
}

kill_vendor_hal() {
    for sig in TERM KILL; do
        for pid in $(pidof "$VENDOR_HAL_NAME" 2>/dev/null); do
            [ "$pid" = "$(mock_pid)" ] && continue
            kill -s $sig "$pid" 2>/dev/null
        done
        sleep 1
    done
}

# GponManager starts the vendor HAL itself; bind-mount a no-op over it.
stub_vendor_hal() {
    printf '#!/bin/sh\nexit 0\n' > "$WORK_DIR/hal_stub.sh"
    chmod 755 "$WORK_DIR/hal_stub.sh"
    : > "$MOUNTS"
    for bin in /bin/$VENDOR_HAL_NAME /usr/bin/$VENDOR_HAL_NAME; do
        [ -e "$bin" ] || continue
        if mount --bind "$WORK_DIR/hal_stub.sh" "$bin"; then
            echo "$bin" >> "$MOUNTS"
            log "stubbed $bin"
        else
            die "cannot bind-mount stub over $bin"
        fi
    done
}

unstub_vendor_hal() {
    [ -f "$MOUNTS" ] || return 0
    while read -r bin; do
        [ -n "$bin" ] && umount "$bin" 2>/dev/null && log "restored $bin"
    done < "$MOUNTS"
    rm -f "$MOUNTS"
}

# ---------------------------------------------------------------------------
# Environment
# ---------------------------------------------------------------------------
# resolve_paths <dir of the calling script>: find the HAL conf/schema, the mock
# binary and its data file (installed by the rdkgponmanager-test package).
resolve_paths() {
    if [ -z "$HAL_CONF" ]; then
        # The recipe installs one of the two depending on WanManagerUnificationEnable.
        for c in /etc/rdk/conf/gpon_manager_wan_unify_conf.json /etc/rdk/conf/gpon_manager_conf.json; do
            [ -f "$c" ] && { HAL_CONF=$c; break; }
        done
    fi
    [ -f "$HAL_CONF" ] || die "GponManager HAL conf not found (set HAL_CONF)"
    HAL_SCHEMA=$(sed -n 's/.*"hal_schema_path" *: *"\([^"]*\)".*/\1/p' "$HAL_CONF" | head -1)
    [ -f "$HAL_SCHEMA" ] || die "HAL schema '$HAL_SCHEMA' from $HAL_CONF not found"
    if [ -z "$MOCK_BIN" ]; then
        for c in "$1/gpon_hal_mock" "$1/hal_mock/gpon_hal_mock" /usr/bin/gpon_hal_mock; do
            [ -x "$c" ] && { MOCK_BIN=$c; break; }
        done
    fi
    if [ -z "$MOCK_DATA" ]; then
        for c in "$1/gpon_hal_mock_data.txt" "$1/hal_mock/gpon_hal_mock_data.txt" $TEST_SHARE/gpon_hal_mock_data.txt; do
            [ -f "$c" ] && { MOCK_DATA=$c; break; }
        done
    fi
    [ -x "$MOCK_BIN" ] || die "gpon_hal_mock binary not found (set MOCK_BIN)"
    [ -f "$MOCK_DATA" ] || die "mock data file not found (set MOCK_DATA)"
}

env_active() { mock_running; }

# env_setup: put GponManager on the mock HAL (steps 1-4 above).
env_setup() {
    mkdir -p "$WORK_DIR"
    log_size > "$ENV_MARK_FILE"
    log_size > "$CLI_MARK_FILE"
    gpon_stop
    kill_vendor_hal
    stub_vendor_hal
    start_mock
}

# env_teardown: back to the vendor HAL.
env_teardown() {
    log "restoring the vendor HAL"
    gpon_stop
    stop_mock
    unstub_vendor_hal
    kill_vendor_hal
    gpon_start
    log "GponManager restarted with the vendor HAL"
}
