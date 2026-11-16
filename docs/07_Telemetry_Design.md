# 07 — GPON Manager Telemetry Design

**Document:** 07 | **Version:** 1.1 | **Component:** RDK GPON Manager (`rdkgponmanager`)
**Spec:** [08_TR181_Telemetry_Reference.md](08_TR181_Telemetry_Reference.md) §2 (Telemetry Events) — HTML: [08_TR181_Telemetry_Reference.html](08_TR181_Telemetry_Reference.html)
**Code:** [`source/telemetry/`](../source/telemetry/)

This document describes the T2 telemetry module of GPON Manager. It deliberately mirrors the EPON Manager design (`rdk-eponmanager/design_docs/07_Telemetry_Design.md`): same API shape, marker naming, priority model, value formats and T2 encoding. Only the GPON-specific producers (PLOAM O-states, ITU-T alarms, VEIP link state machine) differ, plus 4 GPON-only events with no EPON counterpart (ONU state change, emergency-stop cleared, OMCI VEIP admin lock/unlock).

---

## 1. Design Principles

| # | Principle | Description |
|---|-----------|-------------|
| 1 | Single producer surface | Callers only know event ids. Marker names, severity, value formatting, alarm/O-state mapping and T2 dispatch are hidden inside `source/telemetry/`. |
| 2 | Replaceable backend | The real `libtelemetry_msgsender` is selected at configure time only (`--enable-telemetry2-0`). Caller code never changes, and builds without T2 keep a log-only backend. |
| 3 | Independent error reporting | A T2 backend failure doesn't cascade to the producer. Telemetry hiccups are logged and suppressed. |
| 4 | Edge-triggered | Events describe *changes* (state transitions, alarm edges, first failure of a streak), never steady state. |
| 5 | Event-driven | State events are raised only from HAL push events, state-machine transitions and lifecycle/error paths. TR-181 polling (CCSP/RBUS gets, `GponHal_get_data()`) never raises telemetry, except for poll failures (`XPON_ERROR_STATS_COLLECTION_FAILED`, `XPON_SYSTEM_HAL_WRONG_PON_MODE`). |
| 6 | Parity with EPON | Same API pattern (`xponMgr_*` ↔ `eponMgr_*`), same marker pattern (`XPON_*` ↔ `EPON_*`), same priority set, same value formats. |

---

## 2. Key Decisions

| # | Topic | Decision |
|---|-------|----------|
| 1 | Marker prefix | `XPON_` (the module covers GPON, XG-PON, XGS-PON and NG-PON2). |
| 2 | Producer API | Five entry points backed by a single `dispatch(id, ctx)`: `_raise_simple()`, `_raise_intf()` (as in EPON), plus `_raise_alarms()`, `_raise_onu_state()` and `_raise_veip_admin()`, which take *prev/curr* values and do the edge detection inside the module. |
| 3 | Alarm raised/cleared | One event id per ITU-T alarm (14). `RAISED`/`CLEARED` is encoded in the value together with the PhysicalMedia instance: `RAISED,PM=<n>`. |
| 4 | ONU status events | The 4 EPON ONU events (LOS, DOWNSTREAM_SIGNAL_DETECTED, REGISTRATION, DEREGISTRATION), derived from PLOAM O-state transitions, plus GPON-only `XPON_ONU_EMERGENCY_STOP` / `_EMERGENCY_STOP_CLEARED` (entering / leaving O7). These are milestones only, so GPON-only `XPON_ONU_STATE_CHANGE` (`From=O<a>,To=O<b>`) reports *every* O-state change, including O2/O3/O4 and O8/O9 moves and returns to O1 from O6–O9. |
| 5 | Wrong PON mode | `XPON_SYSTEM_HAL_WRONG_PON_MODE` only when the module reports an **EPON** mode. All ITU modes (GPON / XG-PON / XGS-PON / NG-PON2) are valid. |
| 6 | Shutdown | `XPON_SYSTEM_SHUTDOWN` only on the graceful interactive `q` path in `ssp_main.c`. In daemon mode no signal handler raises it. |
| 7 | Simple-marker encoding | Events without a value use `t2_event_d(marker, 1)`, because `t2_event_s()` drops an empty string. Events with a value use `t2_event_s(marker, value)`. Identical to EPON. |
| 8 | Retry loops | Failures on repeating paths (stats polling, WAN Manager publish, PON mode) are latched: one event per failure streak. |
| 9 | Build flag | `--enable-telemetry2-0` (default **off**), giving `ENABLE_FEATURE_TELEMETRY2_0` and `-ltelemetry_msgsender`. Same flag name as EPON. |
| 10 | T2 component name | `GponManager` (EPON: `EponManager`). |
| 11 | OMCI coverage | The HAL exposes no OMCI state machine, only `Omci.{RxBaseLineMessages, RxExtendedMessages, MicErrors}` and the OMCI-provisioned VEIP `AdministrativeState`. Telemetry covers the admin state (`XPON_INTF_ADMIN_LOCK/UNLOCK`). `MicErrors` has no HAL event, so it isn't reported (decision 12). |
| 12 | Event-only detection | ONU state, alarm and VEIP admin events are detected in the HAL push callbacks (`eventcb_*` in `gponmgr_dml_hal.c`), never in the shared `Map_hal_dml_*` mappers that also serve TR-181 polling. The callbacks compare the pushed value with an event-fed shadow (`g_evt_telem`), so a poll that sees a change first can't swallow the event. Data that is only polled (e.g. `Ploam.MicErrors` / `Omci.MicErrors`) has no telemetry. The only telemetry from the polling path is poll failures: `ERROR_STATS_COLLECTION_FAILED` and `SYSTEM_HAL_WRONG_PON_MODE` (polled `PonMode` rejected). |

---

## 3. Public API

Header: [`source/telemetry/gponmgr_telemetry.h`](../source/telemetry/gponmgr_telemetry.h)

```c
typedef enum {
    /* §2.1 ONU activation (PLOAM) */
    XPON_TELEM_ONU_LOS, XPON_TELEM_ONU_DOWNSTREAM_SIGNAL_DETECTED,
    XPON_TELEM_ONU_REGISTRATION, XPON_TELEM_ONU_DEREGISTRATION,
    XPON_TELEM_ONU_EMERGENCY_STOP,
    /* §2.2 Interface link */
    XPON_TELEM_INTF_LINK_UP, XPON_TELEM_INTF_LINK_DOWN,
    XPON_TELEM_PHY_STATUS_UP, XPON_TELEM_PHY_STATUS_DOWN,
    /* §2.3 PhysicalMedia alarms */
    XPON_TELEM_ALARM_LOS, XPON_TELEM_ALARM_LOF, XPON_TELEM_ALARM_SF,
    XPON_TELEM_ALARM_SD, XPON_TELEM_ALARM_LCDG, XPON_TELEM_ALARM_TF,
    XPON_TELEM_ALARM_SUF, XPON_TELEM_ALARM_MEM, XPON_TELEM_ALARM_DACT,
    XPON_TELEM_ALARM_DIS, XPON_TELEM_ALARM_MIS, XPON_TELEM_ALARM_PEE,
    XPON_TELEM_ALARM_RDI, XPON_TELEM_ALARM_ROGUE,
    /* §2.4 System lifecycle */
    XPON_TELEM_SYSTEM_INIT_SUCCESS, XPON_TELEM_SYSTEM_INIT_FAILURE,
    XPON_TELEM_SYSTEM_SHUTDOWN, XPON_TELEM_SYSTEM_HAL_WRONG_PON_MODE,
    /* §2.5 Errors */
    XPON_TELEM_ERROR_HAL_CALL_FAILED, XPON_TELEM_ERROR_HAL_EVENT_SUBSCRIBE_FAILED,
    XPON_TELEM_ERROR_HAL_EVENT_PARSE_FAILED, XPON_TELEM_ERROR_STATS_COLLECTION_FAILED,
    XPON_TELEM_ERROR_RBUS_PUBLISH_FAILED, XPON_TELEM_ERROR_RBUS_REGISTER_FAILED,
    XPON_TELEM_ERROR_STATE_MACHINE_START_FAILED,
    /* GPON-specific additions (no EPON equivalent) */
    XPON_TELEM_ONU_STATE_CHANGE, XPON_TELEM_ONU_EMERGENCY_STOP_CLEARED,   /* §2.1 */
    XPON_TELEM_INTF_ADMIN_LOCK, XPON_TELEM_INTF_ADMIN_UNLOCK,             /* §2.2 */

    XPON_TELEM_EVENT_ID_MAX
} xponMgr_telemetry_event_id_t;

int xponMgr_telemetry_init(const char *component_name);   /* once, at start-up */
int xponMgr_telemetry_cleanup(void);                     /* safe if not initialised */

int xponMgr_telemetry_raise_simple(xponMgr_telemetry_event_id_t id);
int xponMgr_telemetry_raise_intf(xponMgr_telemetry_event_id_t id, const char *ifname);

/* Edge-detecting helpers — raise only what changed between prev and curr. */
int xponMgr_telemetry_raise_alarms(const DML_ALARM *prev, const DML_ALARM *curr,
                                   ULONG pm_instance);            /* returns #events */
int xponMgr_telemetry_raise_onu_state(REG_STATE_ENUM prev,
                                      REG_STATE_ENUM curr);       /* returns #events */
int xponMgr_telemetry_raise_veip_admin(DML_LOCK_ENUM prev, DML_LOCK_ENUM curr,
                                       const char *ifname,
                                       ULONG veip_instance);      /* returns 0/1 */
```

The GPON-only ids are appended after the EPON-parity ids so existing id values don't change.

| Function | EPON equivalent | Notes |
|----------|-----------------|-------|
| `xponMgr_telemetry_init` / `_cleanup` | `eponMgr_telemetry_init` / `_cleanup` | Idempotent. Calls `t2_init()` / `t2_uninit()` when T2 is enabled. |
| `xponMgr_telemetry_raise_simple` | `eponMgr_telemetry_raise_simple` | Value-less events. |
| `xponMgr_telemetry_raise_intf` | `eponMgr_telemetry_raise_intf` | `Interface=<ifname>`; NULL/empty reported as `unknown`. |
| `xponMgr_telemetry_raise_alarms` | `eponMgr_telemetry_raise_alarm` | Takes the whole `DML_ALARM` before/after instead of one HAL alarm record, because the GPON HAL reports alarm *state* rather than alarm *events*. |
| `xponMgr_telemetry_raise_onu_state` | (inside EPON `controller.c`) | GPON maps PLOAM O-states inside the module, so the mapping is unit-testable. |
| `xponMgr_telemetry_raise_veip_admin` | — (GPON-only) | OMCI VEIP `Lock`/`Unlock` edge → `XPON_INTF_ADMIN_LOCK/UNLOCK` with `Interface=<ifname>,Veip=<n>`. |

---

## 4. Architecture

```mermaid
flowchart LR
    subgraph producers [GPON Manager producers]
        SSP[ssp_main.c<br/>init / cleanup / SHUTDOWN / INIT_FAILURE]
        OBJ[gponmgr_dml_obj.c<br/>GponMgrDml_Initialize]
        HAL[gponmgr_dml_hal.c<br/>JSON-HAL client, eventcb_* + event shadow,<br/>GponHal_get_data poll failure]
        MAP[gponmgr_dml_hal_param.c<br/>Map_hal_dml_pm PonMode poll failure]
        CTL[gponmgr_controller.c<br/>HAL event subscription]
        LSM[gponmgr_link_state_machine.c<br/>VEIP link SM]
    end

    subgraph telem [source/telemetry]
        API([gponmgr_telemetry.h])
        TBL[(Descriptor table<br/>38 events)]
        FMT[Value formatter]
        MAPR[Alarm / O-state mapper]
        DSP[Dispatcher<br/>mutex-protected state]
        BE[T2 backend]
    end

    SSP --> API
    OBJ --> API
    HAL --> API
    MAP --> API
    CTL --> API
    LSM --> API
    API --> MAPR --> DSP
    API --> DSP
    DSP --> TBL
    DSP --> FMT
    DSP --> BE
    BE -- "t2_event_s / t2_event_d" --> T2[(T2 daemon<br/>libtelemetry_msgsender)]
    BE -- "[T2] log line" --> LOG[(GPON Manager log)]
```

Build structure: `source/telemetry/` builds `libRdkGponManager_telemetry.la` (noinst), which is linked into the `GponManager` binary together with `$(T2_LIBS)`. It is listed first in `source/Makefile.am` `SUBDIRS`, so it is built before `TR-181` and `GponManager`.

---

## 5. Internal Components

Source: [`source/telemetry/gponmgr_telemetry.c`](../source/telemetry/gponmgr_telemetry.c). The section numbers match the comment banners in the file.

### 5.1 Event Descriptor Table

`k_table[XPON_TELEM_EVENT_ID_MAX]` is a designated-initialiser array that maps each id to `{ marker, priority, value-format }`, for example:

```c
[XPON_TELEM_ONU_LOS]        = { "XPON_ONU_LOS",        PRIO_CRITICAL, FMT_NONE  },
[XPON_TELEM_INTF_LINK_UP]   = { "XPON_INTF_LINK_UP",   PRIO_INFO,     FMT_INTF  },
[XPON_TELEM_ALARM_ROGUE]    = { "XPON_ALARM_ROGUE",    PRIO_CRITICAL, FMT_ALARM },
```

`lookup(id)` rejects ids that are out of range or have no entry.

### 5.2 Value Formatter

| Format | Output | Events |
|--------|--------|--------|
| `FMT_NONE` | empty | ONU milestones, PHY status, lifecycle, errors |
| `FMT_INTF` | `Interface=<ifname>` (`unknown` if NULL/empty) | `XPON_INTF_LINK_UP/DOWN` |
| `FMT_ALARM` | `RAISED,PM=<n>` / `CLEARED,PM=<n>` | `XPON_ALARM_*` |
| `FMT_ONU_STATE` | `From=O<a>,To=O<b>` | `XPON_ONU_STATE_CHANGE` (needs ctx; `raise_simple` returns `-1`) |
| `FMT_VEIP` | `Interface=<ifname>,Veip=<n>` (`unknown` if NULL/empty) | `XPON_INTF_ADMIN_LOCK/UNLOCK` |

For alarms `<n>` is the 1-based `Device.X_RDK_ONT.PhysicalMedia.{i}` instance; for VEIP it is the `Veip.{i}` instance (the interface name may still be empty if the admin state event arrives before it). EPON uses `LLID=<n>` in the same place. Formatting goes into a 256-byte stack buffer; truncation is reported as a failure.

### 5.3 DML Alarm / PLOAM State Mapper

* **Alarms:** `k_alarm_map[]` holds `{ offsetof(DML_ALARM, <field>), event id }` for the 14 alarms. `raise_alarms()` compares each field in `prev` and `curr` and dispatches only the fields that changed (`ACTIVE` → `RAISED`, `INACTIVE` → `CLEARED`).
* **ONU state:** `map_onu_state(prev, curr)` turns one O-state transition into 0–3 ONU events (§6.1): always `STATE_CHANGE`, plus up to two milestones. If `prev == curr`, nothing is sent or logged.
* **VEIP admin:** `raise_veip_admin(prev, curr, …)` sends `ADMIN_LOCK` or `ADMIN_UNLOCK` when the value changes.

### 5.4 T2 Backend

`t2_send(marker, value, prio)`:

1. Always logs `[T2] <PRIO> <marker> [value]` through `CcspTraceInfo`. This is the only output when T2 is compiled out.
2. With `ENABLE_FEATURE_TELEMETRY2_0`: empty value → `t2_event_d(marker, 1)`; otherwise `t2_event_s(marker, value)`.
3. A non-success `T2ERROR` is logged at warning level and returns `-1` to the dispatcher. It is never propagated as a failure of the producer's own operation.

The backend matches EPON `t2_send()` line for line apart from the logging macro.

### 5.5 Dispatcher

`dispatch(id, ctx)`: `lookup` → check `initialized` under `g_state.mtx` → `format_value` → `t2_send`. The mutex only guards the `initialized` flag and the component name. Formatting and the T2 call run outside the lock.

---

## 6. GPON Producers

### 6.1 ONU State → Event Mapping

Rules in `map_onu_state()` (G.984.3 / G.987.3 / G.9807.1). They are evaluated in this order, and every rule that matches emits its event:

| Order | Condition | Event |
|-------|-----------|-------|
| 0 | `prev != curr` (any change) | `XPON_ONU_STATE_CHANGE` `From=O<prev>,To=O<curr>` |
| 1 | `prev == O7` (leaving Emergency Stop) | `XPON_ONU_EMERGENCY_STOP_CLEARED` |
| 2 | `prev == O1` (leaving Initial) | `XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED` |
| 3 | `curr == O6`, or `curr == O1` with `prev` in O2–O5 | `XPON_ONU_LOS` |
| 4 | `prev == O5` (leaving Operation) | `XPON_ONU_DEREGISTRATION` |
| 5 | `curr == O5` (entering Operation) | `XPON_ONU_REGISTRATION` |
| 6 | `curr == O7` (Emergency Stop) | `XPON_ONU_EMERGENCY_STOP` |

Rules 1–6 are the milestones (rules 2–6 are the EPON-parity set). Moves among O2/O3/O4/O8/O9 and returns to O1 from O6/O8/O9 only raise `STATE_CHANGE`. O8/O9 (Downstream/Upstream Tuning) are G.989.3 NG-PON2 states and never occur on single-wavelength GPON/XG-PON/XGS-PON. The full 72-transition table is in [08 §2.6](08_TR181_Telemetry_Reference.md).

Examples (all covered by the unit test; SC = STATE_CHANGE): O1→O5 = SC + DSD + REG; O5→O1 = SC + LOS + DEREG; O5→O6 = SC + LOS + DEREG; O1→O7 = SC + DSD + EMERG; O3→O1 = SC + LOS; O7→O1 = SC + EMERG_CLEARED; O7→O5 = SC + EMERG_CLEARED + REG; O2→O3 = SC; O8→O9 = SC; O*n*→O*n* = none. A 9×9 matrix check verifies that SC is sent first and exactly once per change.

### 6.2 Hook Locations

| File / function | Event(s) | Trigger |
|-----------------|----------|---------|
| `GponManager/ssp_main.c` `main()` | `init("GponManager")` | Before `cmd_dispatch('e')` (engage), so DML init is covered. |
| | `SYSTEM_INIT_FAILURE` + cleanup | `Cdm_Init()` failure, before `exit(1)`. |
| | `SYSTEM_SHUTDOWN` | After the interactive `q` loop, before `Cdm_Term()`. |
| | cleanup | After `ssp_cancel()`. |
| `TR-181/middle_layer_src/gponmgr_dml_obj.c` `GponMgrDml_Initialize()` | `SYSTEM_INIT_FAILURE` | Any of: DataInit, `GponHal_Init`, `GponHal_get_init_data`, `GponMgr_InternalControlInit` failing. |
| | `ERROR_RBUS_REGISTER_FAILED` | `GponMgrXponRbus_Init() != 0` (non-fatal). |
| | `SYSTEM_INIT_SUCCESS` | End of function. |
| `gponmgr_dml_hal.c` `GponHal_Init()` | `ERROR_HAL_CALL_FAILED` | JSON-HAL client init / connect / ready failure (3 sites). |
| `gponmgr_dml_hal.c` `GponHal_setParam()` | `ERROR_HAL_CALL_FAILED` | Request couldn't be sent, or the HAL returned failure. |
| `gponmgr_dml_hal.c` `eventcb_*()` (5 callbacks) | `ERROR_HAL_EVENT_PARSE_FAILED` | HAL event message couldn't be parsed or mapped. |
| `gponmgr_dml_hal.c` `GponHal_get_data()` | `ERROR_STATS_COLLECTION_FAILED` | Bulk read failure. Latched by a `static bool`, re-armed on success. |
| `gponmgr_dml_hal_param.c` `Map_hal_dml_pm()` (`PonMode`) | `SYSTEM_HAL_WRONG_PON_MODE` | Poll failure: `PonMode` has no HAL event; the polled value contains `EPON` and no ITU mode maps. Static latch, reset when a valid mode maps. |
| `gponmgr_dml_hal.c` `eventcb_PhysicalMediaAlarmsAll()` | `ALARM_*` | HAL push event only. After `Map_hal_dml_pm()` updates the DML, the event is applied to a copy of the event shadow `g_evt_telem.pm_alarm[n]` with `Map_hal_dml_alarm()` → `raise_alarms(&shadow, &next, hal_index)`, then the shadow is updated. |
| `gponmgr_dml_hal.c` `eventcb_PloamRegistrationState()` | `ONU_*` | HAL push event only. `raise_onu_state(g_evt_telem.ploam_state, new)` after a successful `RegistrationState` mapping, then the shadow is updated. |
| `gponmgr_dml_hal.c` `eventcb_VeipAdministrativeState()` | `INTF_ADMIN_LOCK/UNLOCK` | HAL push event only. `raise_veip_admin(g_evt_telem.veip_admin[n], new, InterfaceName, hal_index)` after a successful mapping, before the VEIP SM is started on Unlock. |
| `GponManager/gponmgr_controller.c` `GponMgr_subscribe_hal_events()` | `ERROR_HAL_EVENT_SUBSCRIBE_FAILED` | Any subscription failed (after the request data is released). |
| `GponManager/gponmgr_link_state_machine.c` `gpon_sm_transition_LinkDown_to_LinkUp()` / `_LinkUp_to_LinkDown()` | `INTF_LINK_UP/DOWN`, `PHY_STATUS_UP/DOWN` | Successful transition. PHY events on the 0→1 / 1→0 edge of the global `g_veip_links_up` counter (mutex-protected). |
| `gpon_config_veip_interface()` / `gpon_disable_veip_interface()` | `ERROR_RBUS_PUBLISH_FAILED` | WAN Manager PHY status update failed. Latched per state machine (`publish_failure_reported`), because the SM retries every 500 ms. |
| `GponMgr_Link_StateMachine_Start()` | `ERROR_STATE_MACHINE_START_FAILED` | Control-block allocation or `pthread_create()` failure. |

### 6.3 Duplicate Suppression

| Mechanism | Where | Why |
|-----------|-------|-----|
| Event-only edge detection | Alarms, O-states, VEIP admin state | Only the HAL push callbacks compare the pushed value with the event shadow `g_evt_telem` (one entry per PLOAM / PhysicalMedia / VEIP instance). Each entry is seeded on its first event from the DML value held just before it. Polled reads (CCSP getters, `Device.XPON` getters, `GponHal_get_data`) update the DML only and never touch the shadow, so they can neither raise nor swallow a transition. |
| Safe defaults | `gponmgr_dml_data.c` `SetDefault*` | Alarms default to `INACTIVE` and `RegistrationState` to `O1`, also for newly added PhysicalMedia rows, so a shadow seeded from defaults raises no false CLEARED/LOS. VEIP `AdministrativeState` defaults to `Lock`, so the first OMCI *Unlock* raises `ADMIN_UNLOCK` (wanted, like REGISTRATION at boot) and no false `ADMIN_LOCK` is sent. |
| Latches | STATS, PUBLISH, WRONG_PON_MODE | Bounded retry and polling loops (init busy-loop, 500 ms SM loop, TR-181 gets) would otherwise flood T2. |
| Transition-only link events | Link SM | Raised only in the transition functions, never in the steady-state loop. |

---

## 7. Sequence Diagrams

### 7.1 Normal Event (VEIP link up)

```mermaid
sequenceDiagram
    autonumber
    participant SM as gponmgr_link_state_machine.c
    participant API as gponmgr_telemetry.h
    participant Telem as gponmgr_telemetry.c
    participant T2D as T2 daemon

    SM->>SM: gpon_sm_transition_LinkDown_to_LinkUp() OK
    SM->>API: raise_intf(XPON_TELEM_INTF_LINK_UP, "veip0")
    API->>Telem: dispatch(id, ctx{ifname="veip0"})
    Note over Telem: lookup(id) → { "XPON_INTF_LINK_UP", INFO, FMT_INTF }<br/>format → "Interface=veip0"
    Telem->>T2D: t2_event_s("XPON_INTF_LINK_UP", "Interface=veip0")
    SM->>SM: g_veip_links_up 0 → 1
    SM->>API: raise_simple(XPON_TELEM_PHY_STATUS_UP)
    Telem->>T2D: t2_event_d("XPON_PHY_STATUS_UP", 1)
```

### 7.2 Alarm (RAISED / CLEARED)

```mermaid
sequenceDiagram
    autonumber
    participant HAL as GPON HAL (JSON-HAL)
    participant CB as gponmgr_dml_hal.c
    participant Map as gponmgr_dml_hal_param.c
    participant Telem as gponmgr_telemetry.c
    participant T2 as T2 daemon

    HAL->>CB: event PhysicalMedia.1.Alarm (LOS active)
    CB->>Map: Map_hal_dml_pm() [DML data lock held] → pm->Alarm.LOS = ACTIVE
    CB->>Map: next = shadow; Map_hal_dml_alarm(&next) → next.LOS = ACTIVE
    CB->>Telem: raise_alarms(&shadow, &next, 1)
    Note over Telem: LOS: INACTIVE → ACTIVE (others unchanged)<br/>format → "RAISED,PM=1"
    Telem->>T2: t2_event_s("XPON_ALARM_LOS", "RAISED,PM=1")
    CB->>CB: shadow = next
```

### 7.3 ONU State Change

```mermaid
sequenceDiagram
    autonumber
    participant HAL as GPON HAL
    participant CB as eventcb_PloamRegistrationState()
    participant Telem as gponmgr_telemetry.c
    participant T2 as T2 daemon

    HAL->>CB: event Ploam.RegistrationState = O1 (event shadow O5)
    CB->>CB: Map_hal_dml_ploam() → DML O1
    CB->>Telem: raise_onu_state(O5, O1)
    Note over Telem: map_onu_state → [STATE_CHANGE, LOS, DEREGISTRATION]
    Telem->>T2: t2_event_s("XPON_ONU_STATE_CHANGE", "From=O5,To=O1")
    Telem->>T2: t2_event_d("XPON_ONU_LOS", 1)
    Telem->>T2: t2_event_d("XPON_ONU_DEREGISTRATION", 1)
```

### 7.4 Error Event (latched)

```mermaid
sequenceDiagram
    autonumber
    participant SM as Link SM (500 ms loop)
    participant Telem as gponmgr_telemetry.c
    participant T2 as T2 daemon

    SM->>SM: WAN Manager publish fails (1st)
    SM->>Telem: raise_simple(XPON_TELEM_ERROR_RBUS_PUBLISH_FAILED)
    Telem->>T2: t2_event_d("XPON_ERROR_RBUS_PUBLISH_FAILED", 1)
    SM->>SM: publish fails again (latched — no event)
    SM->>SM: publish succeeds → latch re-armed
```

---

## 8. Threading Model

```mermaid
flowchart TB
    subgraph proc [GponManager process]
        MT(main thread<br/>ssp_main.c, DML init, controller init)
        HEV(JSON-HAL event thread<br/>eventcb_* → Map_hal_dml_* + event shadow)
        GET(CCSP / RBUS getter threads<br/>X_RDK_ONT + Device.XPON gets → GponHal_get_*)
        SMT(VEIP link SM thread per VEIP)
    end

    MT  -- raise_simple --> TELE([gponmgr_telemetry.c<br/>mutex-protected state])
    HEV -- raise_alarms / raise_onu_state / raise_veip_admin / raise_simple --> TELE
    GET -- poll failures only: raise_simple STATS_COLLECTION_FAILED / HAL_WRONG_PON_MODE --> TELE
    SMT -- raise_intf / raise_simple --> TELE

    TELE -- t2_event_s / t2_event_d --> T2(T2 daemon)
```

* Thread safety comes from the internal `pthread_mutex_t` in `g_state`. The T2 call is made **outside** this lock.
* The event shadow `g_evt_telem` is only read and written by the `eventcb_*` callbacks while they hold the DML data lock (`GponMgrDml_GetData_locked`). Getter threads never touch it, so polling can't race with or alter event detection.
* The PHY aggregate counter `g_veip_links_up` has its own mutex. Each per-SM latch is only touched by its own SM thread.

---

## 9. State Machines

### 9.1 Alarm Raised / Cleared

```mermaid
stateDiagram-v2
    [*] --> Inactive: shadow seeded (no event)
    Inactive --> Active: HAL Alarm.X = Active<br/>XPON_ALARM_X "RAISED,PM=n"
    Active --> Inactive: HAL Alarm.X = Inactive<br/>XPON_ALARM_X "CLEARED,PM=n"
    Active --> Active: unchanged (no event)
    Inactive --> Inactive: unchanged (no event)
```

The HAL owns the alarm state. The DML keeps the last value from any source (event or poll); the event shadow keeps the last *pushed* value; the telemetry module is **stateless** and forwards each edge between shadow and pushed value as one T2 event. Transitions come only from HAL alarm events.

### 9.2 Failure Latch (STATS / PUBLISH / WRONG_PON_MODE)

```mermaid
stateDiagram-v2
    [*] --> Armed
    Armed --> Reported: failure → raise event
    Reported --> Reported: failure (suppressed)
    Reported --> Armed: success / valid PON mode
    Armed --> Armed: success
```

---

## 10. Error Handling

| Failure | Behaviour |
|---------|-----------|
| `_raise*` called before `_init` (or after `_cleanup`) | Return `0`; event silently dropped. |
| `_raise*` with unknown id | Return `-1`; `WARN` logged. |
| `_raise_alarms` with NULL snapshot | Return `-1`; nothing sent. |
| `_raise_simple(XPON_TELEM_ONU_STATE_CHANGE)` (no ctx) | Return `-1`; `WARN` logged; nothing sent. |
| Value formatting overflow | Return `-1`; `WARN` logged; nothing sent. |
| T2 backend send error | Logged at `WARN`; return `-1`. Producers ignore the return value, so callers never cascade-fail. |
| T2 compiled out | Log line only; return `0`. |

---

## 11. Build & Enablement

`configure.ac`:

```sh
./configure --enable-telemetry2-0     # default: --disable-telemetry2-0
```

* Checks `libtelemetry_msgsender` for `t2_event_s` and fails configure if it is missing.
* Defines `ENABLE_FEATURE_TELEMETRY2_0` (in `config.h` and in `T2_CFLAGS`, because the sources don't include `config.h`) and sets `T2_LIBS=-ltelemetry_msgsender`.

Yocto recipe (`meta-rdk-wan/recipes-ccsp/ccsp/rdkgponmanager.bb`). Add the same pattern used by the EPON Manager recipe:

```bitbake
DEPENDS_append  = " ${@bb.utils.contains('DISTRO_FEATURES', 'telemetry2_0', 'telemetry', '', d)}"
RDEPENDS_${PN}_append = " ${@bb.utils.contains('DISTRO_FEATURES', 'telemetry2_0', 'telemetry', '', d)}"
EXTRA_OECONF_append = " ${@bb.utils.contains('DISTRO_FEATURES', 'telemetry2_0', '--enable-telemetry2-0', '--disable-telemetry2-0', d)}"
```

Runtime check on the device:

```sh
grep -r "\[T2\]" /rdklogs/logs/ | grep XPON_     # GPON Manager log: every event, T2 on or off
grep XPON_ /opt/logs/telemetry2_0.txt.0            # T2 daemon log, with T2 enabled and a profile listing XPON_* markers
```

T2 report profiles must list the `XPON_*` markers (component `GponManager`, `"use": "count"` for value-less events, `"use": "absolute"` / `"accumulate"` for valued ones).

---

## 12. Verification

| Check | Result |
|-------|--------|
| Cross-compile every source file with the recipe's flags (`-Werror -Wall`), with and without `ENABLE_FEATURE_TELEMETRY2_0` | Clean; the only warnings are existing `-Wswitch` warnings in `gponmgr_dml_hal.c`. |
| `autoreconf` + `configure` + `make` with `--enable-telemetry2-0` | `GponManager` has NEEDED `libtelemetry_msgsender.so.0`, and `t2_init`, `t2_event_s`, `t2_event_d`, `t2_uninit` are resolved from it. |
| Same without the flag | No telemetry library dependency. |
| Unit test (aarch64 under qemu, T2 API stubbed) | Pass: O-state mapping table incl. `STATE_CHANGE` values and a 9×9 matrix check, `EMERGENCY_STOP_CLEARED`, alarm RAISED/CLEARED edges + `PM=<n>`, unchanged alarms are silent, `Interface=`/`unknown`, VEIP `ADMIN_LOCK/UNLOCK` values, pre-init drop, unknown id → `-1`, idempotent init, cleanup. |

---

## 13. Known Limitations

* `XPON_SYSTEM_SHUTDOWN` isn't raised in daemon mode (SIGTERM). GPON Manager has no shutdown hook on that path; by design this event is limited to the graceful `q` path.
* Telemetry is event-driven, so data the HAL never pushes (e.g. `Ploam.MicErrors` / `Omci.MicErrors`, PM counters) raises no events. T2 profiles can still report those values by polling the TR-181 parameters directly.
* The state held before the first HAL event of an instance (for example, an ONU already in O5 when GPON Manager starts) is the baseline and isn't reported.
* The HAL exposes no OMCI state machine or OMCI link-state event, so OMCI session up/down can't be reported; only VEIP admin state is covered.
* `XPON_ONU_STATE_CHANGE` is valued (`t2_event_s`), so T2 profiles should use `"use": "absolute"` (last transition) or `"accumulate"` (transition history), not `"count"`.
* EPON's `FACTORY_RESET`, `ONU_RESET`, `EVENT_QUEUE_FULL` and `PSM_ACCESS_FAILED` have no GPON equivalent (no such operations, queue or PSM use).
* Telemetry for `Device.XPON` parameters that are registered but unsupported isn't applicable; see [08 Appendix A](08_TR181_Telemetry_Reference.md#appendix-a-unsupported-devicexpon-parameters).

---

## References

* [08_TR181_Telemetry_Reference.md](08_TR181_Telemetry_Reference.md) / [.html](08_TR181_Telemetry_Reference.html) — TR-181 parameters and telemetry event spec.
* [README.md](README.md) — documentation index.
* EPON Manager `design_docs/07_Telemetry_Design.md` — reference design this module mirrors.
* ITU-T G.984.3, G.987.3, G.9807.1, G.989.3 — ONU activation states and alarm taxonomy.
* RDK T2 Telemetry Framework — `telemetry_busmessage_sender.h` (`t2_init`, `t2_event_s`, `t2_event_d`, `t2_uninit`).
