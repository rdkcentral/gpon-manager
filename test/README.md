# GponManager HAL / TR-181 / T2 integration test

On-target test that replaces the vendor GPON JSON HAL server
(`json_hal_server_gpon`) with a mock, drives HAL events and faults into
GponManager, and verifies:

- **T2 telemetry**: every `XPON_*` marker raised, with its value format and priority.
- **GPON log**: every T2 event is printed as a `[T2] <PRIORITY> <MARKER> [value]` line
  in `/rdklogs/logs/GPONMANAGERLog.txt.0`, and no unknown marker or wrong priority is logged.
- **TR-181**: `Device.XPON.*` (BBF) and `Device.X_RDK_ONT.*` values read with `dmcli`.

> **Lab use only.** The test stops GponManager and the vendor HAL server, so the
> PON WAN is down while it runs. Everything is restored on exit (including Ctrl-C).

Everything that runs on the device is POSIX/busybox `sh` plus the C mock; no
Python is needed on the target. The Python checker is an optional build-host tool.

## Layout

| File | Installed as | Purpose |
|------|--------------|---------|
| [hal_mock/gpon_hal_mock.c](hal_mock/gpon_hal_mock.c) | `/usr/bin/gpon_hal_mock` | Mock JSON HAL server (json-hal-lib), driven through a control FIFO. |
| [hal_mock/gpon_hal_mock_data.txt](hal_mock/gpon_hal_mock_data.txt) | `/usr/share/gponmanager-test/` | Baseline HAL data model (`Device.X_RDK_ONT.*`) served by the mock. |
| [gpon_hal_ctl.sh](gpon_hal_ctl.sh) | `/usr/bin/gpon_hal_ctl` | CLI: start/stop the mock environment, raise alarms, PLOAM/VEIP events, change HAL values, inject faults, check `dmcli` values and `[T2]` lines. |
| [run_gpon_hal_test.sh](run_gpon_hal_test.sh) | `/usr/share/gponmanager-test/` | Complete automated test (POSIX / busybox `sh`). |
| [gpon_hal_test_lib.sh](gpon_hal_test_lib.sh) | `/usr/share/gponmanager-test/` | Helpers and the T2 marker table shared by the CLI and the test. |
| [hal_mock/Makefile](hal_mock/Makefile) | - | Manual build of the mock (`-Wall -Wextra -Werror`). |
| [check_t2_log_coverage.py](check_t2_log_coverage.py) | not installed (build host only) | Optional static T2 check of the sources, plus offline review of a captured GPON log. |

## Build

The `rdkgponmanager.bbappend` builds the mock in `do_compile` and installs all
target files in the separate `rdkgponmanager-test` package. Add it to the image
(`IMAGE_INSTALL_append = " rdkgponmanager-test"`) or install the ipk on the device.

Manually, with the recipe toolchain:

```sh
W=<build>/tmp/work/cortexa53-rdk-linux/rdkgponmanager/<version>
export PATH=$W/recipe-sysroot-native/usr/bin/aarch64-rdk-linux:$PATH
make -C test/hal_mock \
     CC="aarch64-rdk-linux-gcc -mcpu=cortex-a53 -march=armv8-a+crc --sysroot=$W/recipe-sysroot" \
     CFLAGS="-Os -g -I$W/recipe-sysroot/usr/include"
scp test/hal_mock/gpon_hal_mock test/hal_mock/gpon_hal_mock_data.txt \
    test/gpon_hal_ctl.sh test/gpon_hal_test_lib.sh test/run_gpon_hal_test.sh root@<device>:/tmp/gpontest/
```

The scripts look for the library and mock next to themselves first, then in the
installed locations, so a copied directory works the same way
(`sh /tmp/gpontest/gpon_hal_ctl.sh ...`).

## Run the complete test

```sh
gpon_hal_ctl test -y                       # all scenarios, vendor HAL restored afterwards
gpon_hal_ctl test -y -s init,alarm,checks  # selected scenarios
gpon_hal_ctl test -l                       # list scenarios
# same as: sh /usr/share/gponmanager-test/run_gpon_hal_test.sh [options]
```

Options: `-y` no confirmation, `-k` keep the mock environment running on exit,
`-s` scenarios, `-l` list. If the environment was started with
`gpon_hal_ctl start`, the test reloads the baseline HAL data, reuses it and
leaves it running afterwards.

Environment overrides: `MOCK_BIN`, `MOCK_DATA`, `VEIP_IF` (default `veip0`),
`WORK_DIR` (default `/tmp/gpon_hal_test`), `GPON_LOG`, `HAL_CONF`.

Output: `PASS|FAIL|WARN|SKIP` lines, a T2 marker coverage matrix and a summary.
The exit status is 0 only when there is no `FAIL`. Artifacts in `WORK_DIR`:
`results.txt`, `mock.log`, `t2_lines.txt`, `coverage.txt`.

The device run already checks marker names, priorities and coverage in shell
(`checks` scenario, `coverage.txt`). Optionally, copy `t2_lines.txt` to the
build host and review it there (not on the device):

```sh
python3 test/check_t2_log_coverage.py --log t2_lines.txt
```

## Manual testing with gpon_hal_ctl

`gpon_hal_ctl help` lists every command. A typical session (root, lab only):

```sh
gpon_hal_ctl start -y                    # GponManager now runs on the mock HAL
gpon_hal_ctl status                      # mock / GponManager / 20 HAL subscriptions

gpon_hal_ctl mark                        # remember the GPON log position
gpon_hal_ctl alarm LOS raise             # HAL event ...PhysicalMedia.1.Alarm.LOS=Active
gpon_hal_ctl t2                          # [T2] CRITICAL XPON_ALARM_LOS RAISED,PM=1
gpon_hal_ctl expect-t2 XPON_ALARM_LOS RAISED,PM=1          # PASS / FAIL (exit 0 / 1)
gpon_hal_ctl expect-dm Device.XPON.ONU.1.ANI.1.TC.Alarms.LOS true
gpon_hal_ctl alarm LOS clear

gpon_hal_ctl mark; gpon_hal_ctl ploam O5 # registration
gpon_hal_ctl expect-t2 XPON_ONU_STATE_CHANGE From=O1,To=O5
gpon_hal_ctl veip admin unlock           # XPON_INTF_ADMIN_UNLOCK, EthernetUNI.1.Enable=true
gpon_hal_ctl veip oper up                # link state machine (needs WAN Manager)

gpon_hal_ctl set PhysicalMedia.1.Temperature.CurrentTemp 612   # value only, no event
gpon_hal_ctl poll
gpon_hal_ctl get Device.XPON.ONU.1.ANI.1.Transceiver.1.Temperature   # 61

gpon_hal_ctl mark; gpon_hal_ctl fail set on
dmcli eRT setv Device.X_RDK_ONT.PhysicalMedia.1.RxPower.SignalLevelLowerThreshold int -2600
gpon_hal_ctl expect-t2 XPON_ERROR_HAL_CALL_FAILED
gpon_hal_ctl fail set off

gpon_hal_ctl stop                        # back to the vendor HAL
```

| Group | Commands |
|-------|----------|
| Environment | `start [-y]`, `stop`, `status`, `restart-gpon` |
| HAL events (store + publish) | `alarm <NAME\|all> raise\|clear [pm]`, `ploam <O1..O7>`, `veip admin lock\|unlock [n]`, `veip oper up\|down [n]`, `pm-status <value> [pm]`, `event <name> <value>`, `publish <name> <value>` |
| HAL values (next poll) | `set <name> <value>`, `ponmode <mode> [pm]`, `poll` |
| Faults | `fail get\|set\|subscribe on\|off`, `hang get\|set\|subscribe <n>`, `hangtime <s>` |
| Inspect | `get <param...>`, `dump [prefix]`, `subs`, `mocklog [n]`, `mark`, `t2 [-a]`, `log [regex]` |
| Verify | `expect-dm <param> <value>`, `expect-t2 <marker> [value] [timeout]`, `expect-no-t2 <marker> [s]` |
| Full test | `test [options]` |

HAL names may omit the `Device.X_RDK_ONT.` prefix. `expect-t2` without a value
matches the marker with any value. If the shell or ssh session ends, the mock
keeps running; `gpon_hal_ctl stop` always restores the vendor HAL.

### How it works

1. Stops `RdkGponManager.service`, kills `json_hal_server_gpon` and bind-mounts a
   no-op script over it (GponManager starts the HAL server itself at start-up).
2. Starts `gpon_hal_mock` on the port from the GPON HAL conf (`/etc/rdk/conf/gpon_manager*_conf.json`).
3. Starts GponManager, which connects to the mock and subscribes to its events.
4. Each scenario writes commands to the mock FIFO, then checks the GPON log after a
   byte-offset mark (`[T2]` lines) and `dmcli eRT getv` values.
5. On exit (or `gpon_hal_ctl stop`): stops GponManager, stops the mock, removes
   the bind mounts and restarts GponManager with the vendor HAL.

### Scenarios

| Scenario | What it does | Markers / values checked |
|----------|--------------|--------------------------|
| `pre` (always) | Log level in `debug.ini`, T2 linked into GponManager | WARN only |
| `init` | Start on the mock | `SYSTEM_INIT_SUCCESS`, 20 HAL event subscriptions (PM Status, 14 alarm leaves, 2×VEIP admin/oper, PLOAM) |
| `static` | Baseline data | `Device.XPON.ONU.1.*`, ANI, Transceiver, ONUActivation, GEM, Alarms, EthernetUNI, X_RDK values; EthernetUNI.Alias read-only |
| `poll` | HAL value changed without an event | Transceiver.Temperature rounding (612 → 61, -55 → -6) |
| `ploam` | `Ploam.RegistrationState` events O1…O7 | `ONU_STATE_CHANGE`, `ONU_DOWNSTREAM_SIGNAL_DETECTED`, `ONU_REGISTRATION`, `ONU_DEREGISTRATION`, `ONU_LOS`, `ONU_EMERGENCY_STOP`, `ONU_EMERGENCY_STOP_CLEARED`; `ONUActivation.ONUState`; no event on the same state |
| `veip` | `Veip.1.AdministrativeState` Unlock | `INTF_ADMIN_UNLOCK`, `EthernetUNI.Enable` |
| `link` | `Veip.1.OperationalState` Up/Down, LOS, Lock | `INTF_LINK_UP/DOWN`, `PHY_STATUS_UP/DOWN`, `INTF_ADMIN_LOCK`, `EthernetUNI.Status/LastChange`. Skipped when WAN Manager rejects the PHY status (`RBUS_PUBLISH_FAILED`) |
| `alarm` | All 14 `PhysicalMedia.1.Alarm.<X>` leaf events Active/Inactive | `ALARM_<X>` RAISED/CLEARED, `ANI.1.TC.Alarms.<X>` |
| `set` | `dmcli setv` of a HAL-backed parameter; HAL set failure | HAL receives the value; `ERROR_HAL_CALL_FAILED` |
| `epon` | Module reports EPON at run time | `SYSTEM_HAL_WRONG_PON_MODE` (latched), `ANI.PONMode` = `Unknown` |
| `restart` | GponManager restarts with HAL faults | `ERROR_STATS_COLLECTION_FAILED` (get timeout), `ERROR_HAL_EVENT_SUBSCRIBE_FAILED` (subscribe timeout), `SYSTEM_HAL_WRONG_PON_MODE` + `SYSTEM_INIT_FAILURE` (EPON at start), `ERROR_HAL_CALL_FAILED` + `SYSTEM_INIT_FAILURE` (no HAL), no `ADMIN_UNLOCK` for the start-up state |
| `checks` | Whole run | Every `[T2]` line has a known marker and the table priority; no `t2 send failed` |

Markers that cannot be triggered black-box. They are reported as not covered on
the device and are checked only statically on the build host by
`check_t2_log_coverage.py`:

| Marker | Reason |
|--------|--------|
| `XPON_SYSTEM_SHUTDOWN` | Raised only when GponManager runs in the foreground (non-daemon mode). |
| `XPON_ERROR_HAL_EVENT_PARSE_FAILED` | json-hal-lib always delivers well-formed events; a NULL value crashes the client library. |
| `XPON_ERROR_RBUS_REGISTER_FAILED` | Needs the rbus registration of `Device.XPON.` to fail. |
| `XPON_ERROR_STATE_MACHINE_START_FAILED` | Needs the link state machine allocation or `pthread_create` to fail. |

`XPON_ERROR_RBUS_PUBLISH_FAILED` and the link markers depend on WAN Manager
accepting the PHY status update for the VEIP interface.

## Mock HAL control commands

`gpon_hal_ctl` wraps these. To drive the mock directly, write one command per
line to the FIFO (`$WORK_DIR/mock.ctl` when started by the scripts, `-f`):

| Command | Effect |
|---------|--------|
| `set <name> <value>` | Update the store (seen on the next HAL poll). |
| `event <name> <value>` | Update the store and publish the HAL event. |
| `publish <name> <value>` | Publish the event only. |
| `fail get\|set\|subscribe on\|off` | Reply with a failure. A failed `set` raises a HAL error in GponManager; json-hal-lib treats failed `get`/`subscribe` replies as success. |
| `hang get\|set\|subscribe <n>` | Delay the next `n` requests past the 10 s client timeout (a real client-side failure). |
| `hangtime <s>` | Delay used by `hang` (default 12 s). |
| `dump [prefix]`, `subs` | Log the store / the subscriptions. |
| `sync <token>` | Log `SYNC <token>` (the scripts use it to wait for a command). |
| `quit` | Stop the mock. |

Data file format: `<name> <type> <value...>`, with type `string`, `boolean`,
`int`, `unsignedInt`, `long`, `unsignedLong`, `hexBinary` or `base64`.
A `getParameters` request ending in `.` returns every parameter under that prefix.

## Known findings exposed by the test

- **Alarm subscriptions must be per leaf.** The HAL schema
  (`subscribeEventSupportedList`) and the Broadcom HAL publish
  `Device.X_RDK_ONT.PhysicalMedia.<n>.Alarm.<X>`, and json-hal-lib does not
  support prefix subscriptions. GponManager therefore subscribes to each of the
  14 alarm leaves. The `alarm` scenario checks each `<X>-sub` subscription; a
  `FAIL` there means alarm events (and `XPON_ALARM_*` telemetry) are lost and
  `TC.Alarms.*` only updates on polling.
- **T2 is not compiled in** unless the recipe passes `--enable-telemetry2-0`.
  Without it the `[T2]` lines are logged but nothing is sent to the T2 daemon;
  the `pre` scenario reports this as `WARN`.
