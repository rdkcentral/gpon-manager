# GPON Manager

## Overview

GPON Manager (`gpon-manager`) is an RDK-B software component that manages the lifecycle, monitoring, and configuration of a GPON (Gigabit Passive Optical Network) Optical Network Terminal (ONT) device.

It exposes the TR-181 data model subtree `Device.X_RDK_ONT.*` to the platform and communicates with the underlying ONT vendor firmware through a hardware abstraction layer (HAL) using a JSON-over-TCP RPC protocol.

GPON Manager runs as a background daemon process and is responsible for:

- Initialising and maintaining connection to the JSON HAL server.
- Fetching and caching ONT device state (physical media, GEM ports, VEIP, GTC, PLOAM, OMCI, TR-69).
- Subscribing to hardware event notifications and propagating state changes to the data model.
- Running a per-VEIP link state machine that controls WAN interface lifecycle.
- Integrating with WAN Manager and ETH Manager for upstream interface management.

---

## Features

- TR-181 data model exposure (`Device.X_RDK_ONT.*`).
- JSON-HAL-based hardware abstraction with schema-validated RPC communication.
- Per-VEIP link state machine running in dedicated POSIX threads.
- Event-driven updates for physical media status, alarms, VEIP state, and PLOAM registration.
- Support for both standard and WAN Manager Unification (`WAN_MANAGER_UNIFICATION_ENABLED`) deployment modes.
- Health and memory usage reporting.
- Systemd `sd_notify` support for supervised startup (when built with `ENABLE_SD_NOTIFY`).
- Breakpad crash reporting support (when built with `INCLUDE_BREAKPAD`).

---

## Build Instructions

### Prerequisites

- RDK-B build environment (Yocto-based).
- JSON-C library (`json-c`).
- `json_hal_client` library.
- `secure_wrapper` library (for `v_secure_system`).

### Build Process

The GPON Manager is built as part of the RDK-B build system.

```bash
bitbake gpon-manager
```

---

## Documentation

### Architecture

- [architecture/component-diagram.md](architecture/component-diagram.md)

### Components

- [components/controller.md](components/controller.md)
- [components/link-state-machine.md](components/link-state-machine.md)
- [components/data-model-layer.md](components/data-model-layer.md)
- [components/hal-abstraction.md](components/hal-abstraction.md)
- [components/eth-iface.md](components/eth-iface.md)

### Configuration

- [configuration-guide.md](configuration-guide.md)

---

## Directory Structure

```text
gpon-manager/
├── cfg/                                    # Autotools auxiliary files
│   └── Makefile.am
│
├── config/                                 # Runtime configuration files
│   ├── gpon_manager_conf.json
│   ├── gpon_manager_wan_unify_conf.json
│   └── RdkGponManager.xml
│
├── hal_schema/                             # HAL schema definitions and examples
│   ├── example_getParametersResponse_msg.json
│   ├── example_getParameters_msg.json
│   ├── example_getSchemaResponse_msg.json
│   ├── example_getSchema_msg.json
│   ├── example_publishEvent_msg.json
│   ├── example_result_msg.json
│   ├── example_setParameters_msg.json
│   ├── example_subscribeEvent_msg.json
│   ├── gpon_hal_schema.json
│   └── gpon_wan_unify_hal_schema.json
│
├── source/
│   ├── Makefile.am
│   │
│   ├── GponManager/                        # Core GPON Manager component
│   │   ├── gponmgr_controller.c
│   │   ├── gponmgr_controller.h
│   │   ├── gponmgr_link_state_machine.c
│   │   ├── gponmgr_link_state_machine.h
│   │   ├── Makefile.am
│   │   ├── ssp_action.c
│   │   ├── ssp_global.h
│   │   ├── ssp_internal.h
│   │   ├── ssp_main.c
│   │   ├── ssp_messagebus_interface.c
│   │   └── ssp_messagebus_interface.h
│   │
│   └── TR-181/                             # TR-181 Data Model implementation
│       ├── Makefile.am
│       │
│       ├── include/
│       │   └── gpon_apis.h
│       │
│       └── middle_layer_src/
│           ├── gponmgr_dml_backendmgr.c
│           ├── gponmgr_dml_backendmgr.h
│           ├── gponmgr_dml_data.c
│           ├── gponmgr_dml_data.h
│           ├── gponmgr_dml_eth_iface.c
│           ├── gponmgr_dml_eth_iface.h
│           ├── gponmgr_dml_func.c
│           ├── gponmgr_dml_func.h
│           ├── gponmgr_dml_hal.c
│           ├── gponmgr_dml_hal.h
│           ├── gponmgr_dml_hal_param.c
│           ├── gponmgr_dml_hal_param.h
│           ├── gponmgr_dml_internal.c
│           ├── gponmgr_dml_internal.h
│           ├── gponmgr_dml_obj.c
│           ├── gponmgr_dml_obj.h
│           ├── gponmgr_dml_plugin_main.c
│           ├── gponmgr_dml_plugin_main.h
│           └── Makefile.am
│
├── configure.ac
├── CONTRIBUTING.md
├── COPYING
├── LICENSE
├── Makefile.am
├── NOTICE
└── README.md
```

---

## Quick Start

### Building the Component

```bash
bitbake gpon-manager
```

### Configuration

Place the configuration file at the target path:

- Standard mode: `/etc/rdk/conf/gpon_manager_conf.json`
- WAN Unification mode: `/etc/rdk/conf/gpon_manager_wan_unify_conf.json`

Place the HAL schema at:

- Standard mode: `/etc/rdk/schemas/gpon_hal_schema.json`
- WAN Unification mode: `/etc/rdk/schemas/gpon_wan_unify_hal_schema.json`

### Key Interfaces

| Interface                | Purpose                                        | Notes                                  |
| ------------------------ | ---------------------------------------------- | -------------------------------------- |
| Message Bus (D-Bus)      | Component registration, data model access      | Uses `msg_daemon.cfg`                  |
| JSON HAL RPC (TCP 40100) | ONT hardware control and monitoring            | Port configurable via JSON config file |
| WAN Manager D-Bus        | WAN link status propagation                    | `eRT.com.cisco.spvtg.ccsp.wanmanager`  |
| ETH Manager D-Bus        | Ethernet interface creation (non-unified mode) | `eRT.com.cisco.spvtg.ccsp.ethagent`    |

---

For system architecture, component internals, and data flow details, see [architecture/component-diagram.md](architecture/component-diagram.md).
