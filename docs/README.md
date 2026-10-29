# RdkGponManager — Design Documentation

**Version:** 1.0
**Project:** RDK GPON Manager (`rdkgponmanager`)

## Overview

RdkGponManager is the RDK-B component that controls and monitors the xPON (GPON / XG-PON / XGS-PON / NG-PON2) ONU lower layers. It sits between the vendor GPON HAL (accessed over JSON-HAL) and the rest of the RDK stack. It exposes the ONU through TR-181 and reports VEIP link state to WAN Manager.

## Purpose

- GPON HAL initialisation, configuration and event subscription (JSON-HAL client)
- TR-181 data models: `Device.X_RDK_ONT` (CCSP/DML) and `Device.XPON` (BBF, RBUS provider)
- VEIP link state machine and WAN Manager integration
- **T2 telemetry** of ONU activation, link, alarm, lifecycle and error events (this documentation set)

## Key Features

- **Event-driven HAL integration**: PhysicalMedia, alarm, VEIP and PLOAM state are pushed by the HAL and mapped into the DML
- **Per-VEIP link state machine** driving WAN Manager PHY status
- **Two TR-181 views** of the same data (`Device.X_RDK_ONT` and `Device.XPON`)
- **T2 telemetry** with edge-triggered events, matching the EPON Manager telemetry design

## Documentation Structure

| # | Document | Description |
|---|----------|-------------|
| 07 | [Telemetry Design](07_Telemetry_Design.md) | Telemetry module architecture, API, producers (hooks), threading, build/enablement, verification |
| 08 | [TR-181 & Telemetry Reference](08_TR181_Telemetry_Reference.md) | `Device.X_RDK_ONT` and `Device.XPON` parameters, telemetry event spec, EPON parity, Appendix A (unsupported `Device.XPON`) |
| 08 | [TR-181 & Telemetry Reference (HTML)](08_TR181_Telemetry_Reference.html) | Same content as a styled, printable HTML page (open in a browser; *Print / Save as PDF*) |
| — | [XPON TR-181 Mapping](../XPON-TR181-MAPPING.md) | Field-by-field RDK `X_RDK_ONT` → BBF `Device.XPON` mapping notes |

Numbering follows the EPON Manager `design_docs/` set (07 = telemetry design, 08 = TR-181 & telemetry reference) so that the two PON managers can be read side by side.

## Quick Start

### Enabling Telemetry

```sh
./configure --enable-telemetry2-0     # requires libtelemetry_msgsender (RDK telemetry)
make
```

Yocto (same pattern as `rdk-eponmanager.bb`):

```bitbake
DEPENDS_append  = " ${@bb.utils.contains('DISTRO_FEATURES', 'telemetry2_0', 'telemetry', '', d)}"
RDEPENDS_${PN}_append = " ${@bb.utils.contains('DISTRO_FEATURES', 'telemetry2_0', 'telemetry', '', d)}"
EXTRA_OECONF_append = " ${@bb.utils.contains('DISTRO_FEATURES', 'telemetry2_0', '--enable-telemetry2-0', '--disable-telemetry2-0', d)}"
```

Without the flag, the telemetry API is still compiled in and logs each event as `[T2] <PRIORITY> <marker> [value]`, but doesn't link to or call T2.

### Raising an Event

```c
#include "gponmgr_telemetry.h"

xponMgr_telemetry_raise_simple(XPON_TELEM_ERROR_HAL_CALL_FAILED);
xponMgr_telemetry_raise_intf(XPON_TELEM_INTF_LINK_UP, "veip0");
xponMgr_telemetry_raise_alarms(&prevAlarm, &pm->Alarm, pmInstance);   /* edge-detected */
xponMgr_telemetry_raise_onu_state(prevState, newState);                /* O-state mapping */
```

To add an event, append an id before `XPON_TELEM_EVENT_ID_MAX` in `gponmgr_telemetry.h`, add its `k_table[]` entry in `gponmgr_telemetry.c`, and document it in 08 §2 (MD and HTML).

### System Components

| Path | Role |
|------|------|
| `source/GponManager/` | Process entry (`ssp_main.c`), controller, VEIP link state machine |
| `source/TR-181/middle_layer_src/` | `Device.X_RDK_ONT` DML, JSON-HAL client, HAL → DML mapping |
| `source/TR-181/rbus/tr181/` | `Device.XPON` RBUS provider |
| `source/telemetry/` | T2 telemetry module (`libRdkGponManager_telemetry.la`) |
| `config/RdkGponManager.xml` | `Device.X_RDK_ONT` data-model definition |

### Key Technologies

- CCSP message bus / DML plug-in, RBUS
- JSON-HAL (`json_hal_client`) for the vendor GPON HAL
- RDK Telemetry 2.0 (`libtelemetry_msgsender`)

## Glossary

| Term | Meaning |
|------|---------|
| ONU | Optical Network Unit (the CPE side of the PON) |
| OLT | Optical Line Terminal (the operator side of the PON) |
| ANI | Access Network Interface (the PON-facing side of the ONU) |
| VEIP | Virtual Ethernet Interface Point: OMCI ME that bridges the ONU to the RDK router stack |
| PLOAM | Physical Layer OAM: messages that drive ONU activation (O-states O1–O9) |
| OMCI | ONU Management and Control Interface |
| GEM / XGEM | G-PON / XG-PON Encapsulation Method ports |
| GTC | GPON Transmission Convergence layer |
| O1 … O9 | ONU activation states (Initial, Standby, Serial-Number, Ranging, Operation, Intermittent LODS, Emergency Stop, …) |
| LOS / LOF / SF / SD … | ITU-T G.984.3 / G.987.3 alarms (see 08 §2.3) |
| T2 | RDK Telemetry 2.0 framework |
| Marker | T2 event name (`XPON_*`) |

## References

- BBF TR-181 Issue 2 Amendment 21: `Device.XPON`
- ITU-T G.984.3, G.987.3, G.9807.1, G.989.3
- RDK Telemetry 2.0 (`telemetry_busmessage_sender.h`)
- EPON Manager design documents (`rdk-eponmanager/design_docs/`)
