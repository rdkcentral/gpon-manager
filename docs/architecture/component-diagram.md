# GPON Manager Architecture

## Component Overview

GPON Manager is the RDK-B data model provider for GPON ONT devices. It is structured as a layered system: a process management layer (SSP), a hardware abstraction layer (HAL), a data model layer (DML), and a runtime control layer comprising the GPON Controller and per-VEIP link state machines.

---

## System Context

This diagram shows GPON Manager as a black box and its relationships with external systems.

```mermaid
graph TB
    subgraph Platform["RDK-B Platform"]
        GPON["GPON Manager"]
    end

    subgraph HW["Hardware Layer"]
        HAL_SRV["JSON HAL Server\njson_hal_server_gpon"]
        ONT["GPON ONT\n(vendor firmware)"]
    end

    subgraph Network["Network Management"]
        WAN_MGR["WAN Manager"]
        ETH_MGR["ETH Manager\n(standard mode only)"]
    end

    subgraph Infra["Platform Infrastructure"]
        CCSP_BUS["Message Bus\n(D-Bus)"]
        SYSTEMD["systemd"]
    end

    GPON <-->|"JSON RPC over TCP:40100"| HAL_SRV
    HAL_SRV --- ONT
    GPON <-->|"D-Bus: BaseInterfaceStatus\n(WAN unification mode)"| WAN_MGR
    GPON <-->|"D-Bus: X_RDK_Interface\n(standard mode)"| ETH_MGR
    GPON <-->|"D-Bus: data model access,\ncomponent registration"| CCSP_BUS
    GPON -->|"sd_notify READY=1"| SYSTEMD
```

### External Dependencies

| System                                   | Interface                  | Notes                                                            |
| ---------------------------------------- | -------------------------- | ---------------------------------------------------------------- |
| JSON HAL Server (`json_hal_server_gpon`) | TCP port 40100 (localhost) | Vendor-supplied; port is configurable                            |
| WAN Manager                              | D-Bus                      | Used in WAN unification mode (`WAN_MANAGER_UNIFICATION_ENABLED`) |
| ETH Manager                              | D-Bus                      | Used in standard mode only                                       |
| Message Bus                              | D-Bus (local socket)       | Required for data model access by other components               |
| systemd                                  | `sd_notify`                | Optional; requires `ENABLE_SD_NOTIFY` build flag                 |

---

## High-Level Architecture

```mermaid
graph TB
    subgraph ExternalBus["External: Message Bus"]
        CR["Component Registry"]
        CCSP_BUS["Message Bus (D-Bus)"]
    end

    subgraph ExternalHW["External: Hardware Layer"]
        HAL_SERVER["json_hal_server_gpon\nTCP Port 40100"]
        ONT["GPON ONT Hardware"]
    end

    subgraph ExternalWAN["External: Network Management"]
        WAN_MGR["WAN Manager\n(D-Bus)"]
        ETH_MGR["ETH Manager\n(D-Bus)"]
    end

    subgraph DML["TR-181 DML Layer"]
        PLUGIN["DML Entry Point\ngponmgr_dml_plugin_main.c"]
        BEMGR["Backend Manager\ngponmgr_dml_backendmgr.c"]
        OBJ["DML Object\ngponmgr_dml_obj.c"]
        FUNC["DML Functions\ngponmgr_dml_func.c"]
        DATA["Data Store\ngponmgr_dml_data.c\n(mutex-protected)"]
    end

    subgraph CTRL_LAYER["Control Layer"]
        CTRL["GPON Controller\ngponmgr_controller.c"]
        LSM1["Link SM Thread\nVEIP 1"]
        LSM2["Link SM Thread\nVEIP 2"]
        HAL_ABS["HAL Abstraction\ngponmgr_dml_hal.c"]
        HAL_PARAM["HAL Param Mapper\ngponmgr_dml_hal_param.c"]
        ETH_IFACE["ETH/WAN Iface\ngponmgr_dml_eth_iface.c"]
    end

    CCSP_BUS <--> PLUGIN
    CR <--> PLUGIN
    PLUGIN --> BEMGR
    BEMGR --> OBJ
    OBJ --> DATA
    OBJ --> CTRL
    FUNC <--> DATA
    CTRL --> HAL_ABS
    CTRL --> LSM1
    CTRL --> LSM2
    LSM1 <--> DATA
    LSM2 <--> DATA
    LSM1 --> ETH_IFACE
    LSM2 --> ETH_IFACE
    HAL_ABS <--> HAL_PARAM
    HAL_PARAM --> DATA
    HAL_ABS <-->|"JSON RPC"| HAL_SERVER
    HAL_SERVER --- ONT
    ETH_IFACE <-->|"D-Bus"| WAN_MGR
    ETH_IFACE <-->|"D-Bus"| ETH_MGR
```

---

## Component Responsibilities

### DML Layer (`source/TR-181/middle_layer_src/`)

- Mutex-protected in-memory GPON data store.
- All `Device.X_RDK_ONT.*` parameter get/set implementations.
- Data model object lifecycle.

### Control Layer (`source/GponManager/`)

- HAL event subscription and initial configuration dispatch.
- Per-VEIP link state machine management.
- WAN/ETH interface provisioning.

### HAL Abstraction (`source/TR-181/middle_layer_src/gponmgr_dml_hal.c`)

- `json_hal_client` initialisation and connection management.
- Typed query functions for each ONT subsystem.
- Event callback registration and dispatch.
- HAL parameter mapping from JSON to data store structures.

---

## Data Flow Architecture

```mermaid
sequenceDiagram
    participant PLATFORM as Platform
    participant DML as DML Layer
    participant DATA as Data Store
    participant HAL as HAL Abstraction
    participant HALSRV as JSON HAL Server

    Note over DML,HALSRV: Initialisation Phase
    DML->>HAL: GponHal_Init()
    HAL->>HALSRV: json_hal_client_init() + run()
    HAL-->>DML: Connected

    DML->>HAL: GponHal_get_init_data()
    HAL->>HALSRV: getParameters (PhysicalMedia, GTC, PLOAM, OMCI, GEM, VEIP, TR69)
    HALSRV-->>HAL: getParametersResponse (JSON)
    HAL->>DATA: Map_hal_dml_* (populate data store)
    HAL-->>DML: Init data populated

    Note over HAL,HALSRV: Runtime Event Subscription
    HAL->>HALSRV: subscribeEvent (PM.Status, PM.Alarm, VEIP.AdminState, VEIP.OperState, Ploam.RegistrationState)

    Note over HALSRV,DATA: Runtime Event Processing
    HALSRV-->>HAL: publishEvent (onChange)
    HAL->>DATA: eventcb_* (update data store under mutex)

    Note over PLATFORM,DATA: Parameter Access
    PLATFORM->>DML: GetParamValue (Device.X_RDK_ONT.*)
    DML->>DATA: GponMgrDml_GetData_locked()
    DATA-->>DML: Locked data pointer
    DML-->>PLATFORM: Parameter value
    DML->>DATA: GponMgrDml_GetData_release()
```

---

## Threading Model

```mermaid
graph TB
    MAIN["Main Thread\n(daemon sleep loop, 30s interval)\nssp_main.c"]
    SM1["Link SM Thread - VEIP 1\nGponMgr_Link_SM_Thread()\n500ms poll loop"]
    SM2["Link SM Thread - VEIP 2\nGponMgr_Link_SM_Thread()\n500ms poll loop"]

    MAIN -->|"pthread_create per VEIP"| SM1
    MAIN -->|"pthread_create per VEIP"| SM2
```

### Thread Responsibilities

#### Main Thread

Runs the daemon sleep loop (`while(1) { sleep(30); }` in daemon mode). All initialisation — HAL startup, DML registration, controller setup — completes synchronously during startup before the loop is entered. Signal handlers are installed on the main thread.

#### Link State Machine Threads

One thread is created per VEIP interface by `GponMgr_Link_StateMachine_Start()`. Each thread is detached (`pthread_detach`). The thread runs a `select()`-based timer loop with a 500 ms timeout. On each wake, it acquires the global data store mutex, evaluates the current state, performs any required transitions, and releases the mutex. The thread exits when `sm_running` is set to `FALSE` (on transition to `GSM_EXIT`).

---

## Message Flow Patterns

### Link State Machine Lifecycle

```mermaid
flowchart TD
    INIT["GponMgr_Link_StateMachine_Start()\nInitialise GPON_LINK_SM_CTRL_T"]
    START["gpon_sm_transition_Start()\nsm_running = true\n→ GSM_LINK_DOWN"]
    POLL["500ms select() wait"]
    LOCK["GponMgrDml_GetData_locked()"]
    STATE_DOWN["Gpon_Link_Down_State()"]
    STATE_UP["Gpon_Link_Up_State()"]
    CHK_ENABLED["check_gpon_veip_interface_enabled()"]
    CHK_UP["check_gpon_veip_interface_up()"]
    T_UP["gpon_sm_transition_LinkDown_to_LinkUp()\ngpon_config_veip_interface()"]
    T_DOWN["gpon_sm_transition_LinkUp_to_LinkDown()\ngpon_disable_veip_interface()"]
    EXIT["gpon_sm_transition_Exit()\nsm_running = false"]
    RELEASE["GponMgrDml_GetData_release()"]
    CLEANUP["GponMgr_Link_SM_Cleanup()"]

    INIT --> START
    START --> POLL
    POLL --> LOCK
    LOCK --> STATE_DOWN
    LOCK --> STATE_UP
    STATE_DOWN --> CHK_ENABLED
    CHK_ENABLED -->|"Unlocked"| CHK_UP
    CHK_ENABLED -->|"Locked"| EXIT
    CHK_UP -->|"Up + no LOS"| T_UP
    CHK_UP -->|"Down or LOS"| RELEASE
    T_UP --> RELEASE
    STATE_UP --> CHK_ENABLED
    CHK_ENABLED -->|"Locked"| T_DOWN
    T_DOWN --> RELEASE
    RELEASE --> POLL
    EXIT --> CLEANUP
```

---

## Error Recovery Flow

```mermaid
flowchart TD
    ERR["Error Detected"]
    TYPE{Error Type}

    ERR --> TYPE

    TYPE -->|"HAL connection failure\n(init)"| RETRY["Retry up to 10x\n(1s intervals)"]
    RETRY -->|"Max retries exceeded"| FAIL1["Return ANSC_STATUS_FAILURE\nLog error"]

    TYPE -->|"HAL data not initialised"| LOOP["Retry in loop\n(no limit)\nuntil valid data"]
    LOOP -->|"Valid data received"| OK["Continue initialisation"]

    TYPE -->|"HAL event subscribe failure"| ABORT["Abort subscription loop\nReturn failure\nLog error"]

    TYPE -->|"State machine thread failure"| LOG["Log error\nReturn ANSC_STATUS_FAILURE\nNo SM for that VEIP"]

    TYPE -->|"Signal (SIGSEGV/SIGBUS etc.)"| BT["Print stack backtrace\nExit process"]
```

---

## Interface Architecture

```mermaid
graph TB
    subgraph DML_API["TR-181 DML API"]
        GetStr["GponPhy_GetParamStringValue()"]
        GetUlong["GponPhy_GetParamUlongValue()"]
        GetInt["GponPhyRxpwr_GetParamIntValue()"]
        SetInt["GponPhyRxpwr_SetParamIntValue()"]
        GetEntry["GponPhy_GetEntry()"]
        Sync["GponPhy_Synchronize()"]
    end

    subgraph HAL_API["HAL API (Internal)"]
        HalInit["GponHal_Init()"]
        HalGetPm["GponHal_get_pm()"]
        HalGetVeip["GponHal_get_veip()"]
        HalGetPloam["GponHal_get_ploam()"]
        HalGetGem["GponHal_get_gem()"]
        HalSub["GponHal_Event_Subscribe()"]
        HalSet["GponHal_setParam()"]
    end

    subgraph DATA_API["Data Store API"]
        Lock["GponMgrDml_GetData_locked()"]
        Release["GponMgrDml_GetData_release()"]
        Init["GponMgrDml_DataInit()"]
    end

    DML_API --> DATA_API
    DML_API --> HAL_API
    HAL_API --> DATA_API
```

---

## Security Architecture

```mermaid
graph TB
    BUS["Message Bus\n(D-Bus, local socket)"]
    SSP["GPON Manager Process"]
    HAL_SRV["JSON HAL Server\n(localhost TCP 40100)"]
    SYS["System Commands\n(v_secure_system)"]

    BUS -->|"Local IPC only"| SSP
    SSP -->|"localhost TCP only"| HAL_SRV
    SSP -->|"secure_wrapper"| SYS
```

- All CCSP communication is over the local D-Bus socket; no external network exposure.
- HAL server communication is over localhost TCP only.
- System command execution uses the `v_secure_system()` wrapper (`secure_wrapper` library) to prevent command injection.

---

## Build Variants

Two significant build-time variants change runtime behaviour:

| Macro                             | Effect                                                                                                                                                                      |
| --------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `WAN_MANAGER_UNIFICATION_ENABLED` | Uses WAN Manager (`Device.X_RDK_WanManager.Interface.*`) instead of ETH Manager for link status; adds `PhysicalMedia.Enable`, `Alias`, `LowerLayers`, `Upstream` parameters |
| `ENABLE_SD_NOTIFY`                | Sends `sd_notify(READY=1)` to systemd after successful initialisation                                                                                                       |
| `INCLUDE_BREAKPAD`                | Delegates crash signal handling to Breakpad exception handler                                                                                                               |
| `_COSA_SIM_`                      | Enables PC simulation mode (empty subsystem prefix)                                                                                                                         |

For steps to add new data model parameters or integrate on a new platform, see [configuration-guide.md](../configuration-guide.md).
