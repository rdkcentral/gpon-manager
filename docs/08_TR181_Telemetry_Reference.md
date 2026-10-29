# 08 — GPON Manager TR-181 & Telemetry Reference

**Document:** 08 | **Version:** 1.0 | **Date:** October 29, 2026 | **Component:** RDK GPON Manager (`rdkgponmanager`)

> An HTML version of this document with the same content, styled for printing, is in [08_TR181_Telemetry_Reference.html](08_TR181_Telemetry_Reference.html).

## Table of Contents

1. [TR-181 Data Model Parameters](#1-tr-181-data-model-parameters)
   1. [`Device.X_RDK_ONT` (RDK GPON Data Model)](#11-devicex_rdk_ont-rdk-gpon-data-model)
      1. [PhysicalMedia — Module & Interface](#111-physicalmedia--module--interface)
      2. [PhysicalMedia — Optical Diagnostics & Thresholds](#112-physicalmedia--optical-diagnostics--thresholds)
      3. [PhysicalMedia — Alarms](#113-physicalmedia--alarms)
      4. [GTC (Transmission Convergence) Counters](#114-gtc-transmission-convergence-counters)
      5. [PLOAM](#115-ploam)
      6. [GEM Ports](#116-gem-ports)
      7. [OMCI](#117-omci)
      8. [VEIP (Virtual Ethernet Interface Point)](#118-veip-virtual-ethernet-interface-point)
      9. [TR-069 Management](#119-tr-069-management)
   2. [`Device.XPON` (BBF TR-181 2.21) — Supported Parameters](#12-devicexpon-bbf-tr-181-221--supported-parameters)
      1. [XPON Root, ONU & EthernetUNI](#121-xpon-root-onu--ethernetuni)
      2. [ANI](#122-ani)
      3. [ANI.TC — ONU Activation](#123-anitc--onu-activation)
      4. [ANI.TC — Performance Monitoring (PHY / GEM / PLOAM / OMCI)](#124-anitc--performance-monitoring-phy--gem--ploam--omci)
      5. [ANI.TC — GEM Ports](#125-anitc--gem-ports)
      6. [ANI.TC — Performance Thresholds](#126-anitc--performance-thresholds)
      7. [ANI.TC — Alarms](#127-anitc--alarms)
      8. [ANI.Transceiver](#128-anitransceiver)
2. [Telemetry Events (T2)](#2-telemetry-events-t2)
   1. [ONU Activation (PLOAM) Events](#21-onu-activation-ploam-events)
   2. [Interface Link Events](#22-interface-link-events)
   3. [PhysicalMedia Alarms (G.984.3 / G.987.3)](#23-physicalmedia-alarms-g9843--g9873)
   4. [System Lifecycle Events](#24-system-lifecycle-events)
   5. [Error Events](#25-error-events)
   6. [ONU State → Event Mapping](#26-onu-state--event-mapping)
   7. [Value Formats & Duplicate Suppression](#27-value-formats--duplicate-suppression)
   8. [Priority Definitions](#28-priority-definitions)
   9. [Summary Statistics](#29-summary-statistics)
   10. [Parity with EPON Manager](#210-parity-with-epon-manager)
- [Appendix A. Unsupported `Device.XPON` Parameters](#appendix-a-unsupported-devicexpon-parameters)
- [References](#references)

## 1. TR-181 Data Model Parameters

GPON Manager exposes two data models. Both are filled from the same in-memory DML structure (`GPON_DML_DATA`), which the JSON-HAL client keeps up to date from HAL push events and on-demand reads.

> **Legend:** Read-Only = Green   Read/Write = Red   Table = Dynamic table (`{i}`) populated as the HAL reports instances

### 1.1 `Device.X_RDK_ONT` (RDK GPON Data Model)

Defined in `config/RdkGponManager.xml` and served over the CCSP message bus by the DML plug-in in `source/TR-181/middle_layer_src`. 95 parameters in total (90 without `WAN_MANAGER_UNIFICATION_ENABLED`). Tables: Table `PhysicalMedia.{i}`, `Gem.{i}`, `Veip.{i}`.

#### 1.1.1 PhysicalMedia — Module & Interface

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 1 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Cage` | Physical cage of the optical module: *BoB* (bond-on-board) or *SFP*. | string (enum) | Read-Only |
| 2 | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleVendor` | Optical module vendor name. | string(256) | Read-Only |
| 3 | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleName` | Optical module model/part name. | string(256) | Read-Only |
| 4 | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleVersion` | Optical module hardware version. | string(256) | Read-Only |
| 5 | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleFirmwareVersion` | Optical module firmware version. | string(256) | Read-Only |
| 6 | `Device.X_RDK_ONT.PhysicalMedia.{i}.PonMode` | PON technology of the module: *GPON*, *XG-PON*, *NG-PON2*, *XGS-PON*. An HAL value containing *EPON* raises `XPON_SYSTEM_HAL_WRONG_PON_MODE`. | string (enum) | Read-Only |
| 7 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Connector` | Fibre connector type: *LC*, *ST*, *FC*, *SC*, *MT-RJ*. | string (enum) | Read-Only |
| 8 | `Device.X_RDK_ONT.PhysicalMedia.{i}.NominalBitRateDownstream` | Nominal downstream line rate reported by the module (HAL unit; BBF uses kbps). | uint32 | Read-Only |
| 9 | `Device.X_RDK_ONT.PhysicalMedia.{i}.NominalBitRateUpstream` | Nominal upstream line rate reported by the module (HAL unit; BBF uses kbps). | uint32 | Read-Only |
| 10 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Enable` | Administrative enable of the physical media (WAN Manager unification). *Registered only when built with `WAN_MANAGER_UNIFICATION_ENABLED`.* | boolean | Read/Write |
| 11 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Status` | Operational status: *Up*, *Down*, *Unknown*, *Dormant*, *NotPresent*, *LowerLayerDown*, *Error*. Pushed by the HAL status event. | string (enum) | Read-Only |
| 12 | `Device.X_RDK_ONT.PhysicalMedia.{i}.RedundancyState` | Redundancy role of this media: *Active* or *Standby*. | string (enum) | Read-Only |
| 13 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alias` | User-assignable alias for the instance (WAN Manager unification). *Registered only when built with `WAN_MANAGER_UNIFICATION_ENABLED`.* | string(64) | Read/Write |
| 14 | `Device.X_RDK_ONT.PhysicalMedia.{i}.LastChange` | Seconds since the interface last changed status (WAN Manager unification). *Registered only when built with `WAN_MANAGER_UNIFICATION_ENABLED`.* | uint32 | Read-Only |
| 15 | `Device.X_RDK_ONT.PhysicalMedia.{i}.LowerLayers` | Comma-separated list of lower-layer paths (WAN Manager unification). *Registered only when built with `WAN_MANAGER_UNIFICATION_ENABLED`.* | string(1024) | Read/Write |
| 16 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Upstream` | Whether this interface is upstream-facing (WAN Manager unification). *Registered only when built with `WAN_MANAGER_UNIFICATION_ENABLED`.* | boolean | Read-Only |

#### 1.1.2 PhysicalMedia — Optical Diagnostics & Thresholds

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 17 | `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevel` | Received optical power (HAL scale; BBF uses 0.1 dBm). | int32 | Read-Only |
| 18 | `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelLowerThreshold` | Lower RX optical power threshold. | int32 | Read/Write |
| 19 | `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelUpperThreshold` | Upper RX optical power threshold. | int32 | Read/Write |
| 20 | `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevel` | Transmitted optical power (HAL scale; BBF uses 0.1 dBm). | int32 | Read-Only |
| 21 | `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelLowerThreshold` | Lower TX optical power threshold. | int32 | Read/Write |
| 22 | `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelUpperThreshold` | Upper TX optical power threshold. | int32 | Read/Write |
| 23 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Voltage.VoltageLevel` | Module supply voltage (HAL scale; BBF uses mV). | int32 | Read-Only |
| 24 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Bias.CurrentBias` | Laser bias current (HAL scale; BBF uses µA). | uint32 | Read-Only |
| 25 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Temperature.CurrentTemp` | Module temperature (HAL scale; BBF uses °C). | int32 | Read-Only |
| 26 | `Device.X_RDK_ONT.PhysicalMedia.{i}.PerformanceThreshold.SignalFail` | BER threshold exponent for the Signal Fail (SF) alarm. | uint32 | Read-Only |
| 27 | `Device.X_RDK_ONT.PhysicalMedia.{i}.PerformanceThreshold.SignalDegrade` | BER threshold exponent for the Signal Degrade (SD) alarm. | uint32 | Read-Only |

#### 1.1.3 PhysicalMedia — Alarms

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 28 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.RDI` | Remote Defect Indication — OLT reports it is receiving a defective upstream signal from this ONU. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_RDI`. | string (enum) | Read-Only |
| 29 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.PEE` | Physical Equipment Error reported by the OLT. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_PEE`. | string (enum) | Read-Only |
| 30 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LOS` | Loss Of Signal — no downstream optical signal at the ONU receiver. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_LOS`. | string (enum) | Read-Only |
| 31 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LOF` | Loss Of Frame — downstream frame delineation lost. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_LOF`. | string (enum) | Read-Only |
| 32 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.DACT` | Deactivate ONU-ID — OLT has deactivated this ONU's ID. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_DACT`. | string (enum) | Read-Only |
| 33 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.DIS` | Disabled ONU — OLT has administratively disabled this ONU (serial-number based). Value *Active*/*Inactive*; every edge raises `XPON_ALARM_DIS`. | string (enum) | Read-Only |
| 34 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.MIS` | Link Mismatch — downstream/upstream wavelength or link mismatch detected. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_MIS`. | string (enum) | Read-Only |
| 35 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.MEM` | Message Error Message — unexpected/unknown PLOAM message received. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_MEM`. | string (enum) | Read-Only |
| 36 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SUF` | Start-Up Failure — ranging/activation of the ONU failed. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_SUF`. | string (enum) | Read-Only |
| 37 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SF` | Signal Fail — downstream BER exceeded the `PerformanceThreshold.SignalFail` threshold. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_SF`. | string (enum) | Read-Only |
| 38 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SD` | Signal Degrade — downstream BER exceeded the `PerformanceThreshold.SignalDegrade` threshold. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_SD`. | string (enum) | Read-Only |
| 39 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LCDG` | Loss of GEM Channel Delineation. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_LCDG`. | string (enum) | Read-Only |
| 40 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.TF` | Transmitter Failure — ONU laser/transmitter fault. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_TF`. | string (enum) | Read-Only |
| 41 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.ROGUE` | Rogue ONU — this ONU is transmitting outside its allocated upstream window. Value *Active*/*Inactive*; every edge raises `XPON_ALARM_ROGUE`. | string (enum) | Read-Only |

#### 1.1.4 GTC (Transmission Convergence) Counters

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 42 | `Device.X_RDK_ONT.Gtc.CorrectedFecBytes` | Bytes corrected by downstream FEC. | uint32 | Read-Only |
| 43 | `Device.X_RDK_ONT.Gtc.CorrectedFecCodeWords` | Downstream FEC codewords that were corrected. | uint32 | Read-Only |
| 44 | `Device.X_RDK_ONT.Gtc.UnCorrectedFecCodeWords` | Downstream FEC codewords that could not be corrected. | uint32 | Read-Only |
| 45 | `Device.X_RDK_ONT.Gtc.TotalFecCodeWords` | Total downstream FEC codewords received. | uint32 | Read-Only |
| 46 | `Device.X_RDK_ONT.Gtc.HecErrorCount` | Header Error Control (HEC) errors in the downstream frame header. | uint32 | Read-Only |
| 47 | `Device.X_RDK_ONT.Gtc.PSBdHecErrors` | HEC errors detected in the downstream Physical Synchronisation Block (PSBd). | uint32 | Read-Only |
| 48 | `Device.X_RDK_ONT.Gtc.FrameHecErrors` | HEC errors detected in GEM/XGEM frame headers. | uint32 | Read-Only |
| 49 | `Device.X_RDK_ONT.Gtc.FramesLost` | Downstream frames lost. | uint32 | Read-Only |

#### 1.1.5 PLOAM

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 50 | `Device.X_RDK_ONT.Ploam.OnuId` | ONU-ID assigned by the OLT during activation (0–1022). | uint32 | Read-Only |
| 51 | `Device.X_RDK_ONT.Ploam.VendorId` | ONU vendor ID used in the serial-number PLOAM. | string(128) | Read-Only |
| 52 | `Device.X_RDK_ONT.Ploam.SerialNumber` | ONU serial number used for OLT activation. | string(128) | Read-Only |
| 53 | `Device.X_RDK_ONT.Ploam.RegistrationState` | ITU-T G.984/G.987 activation state *O1*–*O9*. Every transition is mapped to the `XPON_ONU_*` telemetry events (§2.1). | string (enum) | Read-Only |
| 54 | `Device.X_RDK_ONT.Ploam.ActivationCounter` | Number of times the ONU has reached the operational state. | uint32 | Read-Only |
| 55 | `Device.X_RDK_ONT.Ploam.TxMessageCount` | Upstream PLOAM messages transmitted. | uint32 | Read-Only |
| 56 | `Device.X_RDK_ONT.Ploam.RxMessageCount` | Downstream PLOAM messages received. | uint32 | Read-Only |
| 57 | `Device.X_RDK_ONT.Ploam.MicErrors` | Received PLOAM messages with a MIC (message integrity check) error. | uint32 | Read-Only |
| 58 | `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO1` | TO1 (serial-number / ranging) timer value. | uint32 | Read-Only |
| 59 | `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO2` | TO2 (POPUP) timer value. | uint32 | Read-Only |

#### 1.1.6 GEM Ports

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 60 | `Device.X_RDK_ONT.Gem.{i}.PortId` | GEM / XGEM port identifier. | uint32 | Read-Only |
| 61 | `Device.X_RDK_ONT.Gem.{i}.TrafficType` | Direction of the GEM port: *Unidirectional* or *Bidirectional*. | string (enum) | Read-Only |
| 62 | `Device.X_RDK_ONT.Gem.{i}.TransmittedFrames` | Frames transmitted on this GEM port. | uint32 | Read-Only |
| 63 | `Device.X_RDK_ONT.Gem.{i}.ReceivedFrames` | Frames received on this GEM port. | uint32 | Read-Only |
| 64 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.Tagged` | Ingress tagging mode of the flow. | string (enum) | Read-Only |
| 65 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Vid` | Ingress S-VLAN VLAN ID. | uint32 | Read-Only |
| 66 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Pcp` | Ingress S-VLAN priority code point. | uint32 | Read-Only |
| 67 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Dei` | Ingress S-VLAN drop eligible indicator. | uint32 | Read-Only |
| 68 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Vid` | Ingress C-VLAN VLAN ID. | uint32 | Read-Only |
| 69 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Pcp` | Ingress C-VLAN priority code point. | uint32 | Read-Only |
| 70 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Dei` | Ingress C-VLAN drop eligible indicator. | uint32 | Read-Only |
| 71 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.Tagged` | Egress tagging mode of the flow. | string (enum) | Read-Only |
| 72 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Vid` | Egress S-VLAN VLAN ID. | uint32 | Read-Only |
| 73 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Pcp` | Egress S-VLAN priority code point. | uint32 | Read-Only |
| 74 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Dei` | Egress S-VLAN drop eligible indicator. | uint32 | Read-Only |
| 75 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Vid` | Egress C-VLAN VLAN ID. | uint32 | Read-Only |
| 76 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Pcp` | Egress C-VLAN priority code point. | uint32 | Read-Only |
| 77 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Dei` | Egress C-VLAN drop eligible indicator. | uint32 | Read-Only |

#### 1.1.7 OMCI

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 78 | `Device.X_RDK_ONT.Omci.RxBaseLineMessageCountValid` | Valid baseline-format OMCI messages received. | int32 | Read-Only |
| 79 | `Device.X_RDK_ONT.Omci.RxExtendedMessageCountValid` | Valid extended-format OMCI messages received. | int32 | Read-Only |
| 80 | `Device.X_RDK_ONT.Omci.MicErrors` | Received OMCI messages with a MIC error. | uint32 | Read-Only |

#### 1.1.8 VEIP (Virtual Ethernet Interface Point)

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 81 | `Device.X_RDK_ONT.Veip.{i}.MeId` | OMCI Managed Entity ID of the VEIP. | uint32 | Read-Only |
| 82 | `Device.X_RDK_ONT.Veip.{i}.AdministrativeState` | OMCI administrative state: *Lock* / *Unlock*. | string (enum) | Read-Only |
| 83 | `Device.X_RDK_ONT.Veip.{i}.OperationalState` | VEIP operational state. *Up*/*Down* transitions drive the link state machine and the `XPON_INTF_LINK_*` / `XPON_PHY_STATUS_*` events. | string (enum) | Read-Only |
| 84 | `Device.X_RDK_ONT.Veip.{i}.InterDomainName` | OMCI inter-domain name of the VEIP. | string(25) | Read-Only |
| 85 | `Device.X_RDK_ONT.Veip.{i}.InterfaceName` | Linux interface name of the VEIP (e.g. *veip0*); carried in the `XPON_INTF_LINK_*` event value. | string(256) | Read-Only |
| 86 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Tagged` | Ingress tagging mode of the flow. | string (enum) | Read/Write |
| 87 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Vid` | Ingress Q-VLAN VLAN ID. | uint32 | Read/Write |
| 88 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Pcp` | Ingress Q-VLAN priority code point. | uint32 | Read-Only |
| 89 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Dei` | Ingress Q-VLAN drop eligible indicator. | uint32 | Read-Only |
| 90 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Tagged` | Egress tagging mode of the flow. | string (enum) | Read/Write |
| 91 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Vid` | Egress Q-VLAN VLAN ID. | uint32 | Read/Write |
| 92 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Pcp` | Egress Q-VLAN priority code point. | uint32 | Read-Only |
| 93 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Dei` | Egress Q-VLAN drop eligible indicator. | uint32 | Read-Only |

#### 1.1.9 TR-069 Management

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 94 | `Device.X_RDK_ONT.TR69.url` | ACS URL provisioned through OMCI (TR-069 management ME). | string(256) | Read-Only |
| 95 | `Device.X_RDK_ONT.TR69.AssociatedTag` | VLAN tag associated with the TR-069 management path. | uint32 | Read-Only |

### 1.2 `Device.XPON` (BBF TR-181 2.21) — Supported Parameters

Registered as an RBUS provider by `source/TR-181/rbus/tr181/gponmgr_xpon_tr181.c` (`GponMgrXponRbus_Init()`). 66 of the 135 registered parameters are backed by `Device.X_RDK_ONT` data, as listed below. The other 69 are registered for completeness but return `RBUS_ERROR_INVALID_OPERATION`; see Appendix A.

> **Instances:** only `ONU.1` and `ANI.1` exist. `EthernetUNI.{i}`, `ANI.1.Transceiver.{i}` and `TC.GEM.Port.{i}` follow the populated `Veip`, `PhysicalMedia` and `Gem` rows. ANI-level PhysicalMedia fields need exactly one active PhysicalMedia row. Values are passed through without unit conversion (BBF units: power 0.1 dBm, voltage mV, bias µA, temperature °C, bit rate kbps).

#### 1.2.1 XPON Root, ONU & EthernetUNI

| # | TR-181 Parameter | Description | Source (<code>X_RDK_ONT</code>) | Data Type | Access |
|---|---|---|---|---|---|
| 1 | `Device.XPON.ONUNumberOfEntries` | Number of entries in the `ONU` table. *Always 1.* | `constant` | unsignedInt | Read-Only |
| 2 | `Device.XPON.ONU.{i}.SoftwareImageNumberOfEntries` | Number of entries in the `SoftwareImage` table. *Always 0 — no software-image rows are registered.* | `constant` | unsignedInt | Read-Only |
| 3 | `Device.XPON.ONU.{i}.EthernetUNINumberOfEntries` | Number of entries in the `EthernetUNI` table. *Number of populated `Veip.{i}` rows.* | `Veip table` | unsignedInt | Read-Only |
| 4 | `Device.XPON.ONU.{i}.ANINumberOfEntries` | Number of entries in the `ANI` table. *Always 1.* | `constant` | unsignedInt | Read-Only |
| 5 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Enable` | Indicates whether this interface is enabled or not. *Unlock* → true, *Lock* → false. | `Veip.{i}.AdministrativeState` | boolean | Read-Only |
| 6 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Status` | The current operational state of the interface. *Same enumeration.* | `Veip.{i}.OperationalState` | string | Read-Only |
| 7 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Name` | The textual name of the interface as assigned by the CPE. *Truncated to 64 characters.* | `Veip.{i}.InterfaceName` | string | Read-Only |
| 8 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.ANIs` | References all associated ANI interfaces. *Always `Device.XPON.ONU.1.ANI.1`.* | `constant` | `list<string>` | Read-Only |
| 9 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainID` | String to identify the associated service in the ONU management domain to the TR-181 management domain. *Formatted as `(VEIP,<MeId>)`.* | `Veip.{i}.MeId` | string | Read-Only |
| 10 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainName` | String that provides an additional way to identify the associated service to the TR-181 management domain. | `Veip.{i}.InterDomainName` | string | Read-Only |

#### 1.2.2 ANI

| # | TR-181 Parameter | Description | Source (<code>X_RDK_ONT</code>) | Data Type | Access |
|---|---|---|---|---|---|
| 11 | `Device.XPON.ONU.{i}.ANI.{i}.Enable` | Enables or disables the interface. If disabled, the device should force the ONU state of this ANI to O1 (Initial state). *Requires `WAN_MANAGER_UNIFICATION_ENABLED`; otherwise rejected with a warning.* | `PhysicalMedia.{i}.Enable` | boolean | Read/Write |
| 12 | `Device.XPON.ONU.{i}.ANI.{i}.Status` | The current operational state of the interface. *Uses the uniquely active PhysicalMedia row.* | `PhysicalMedia.{i}.Status` | string | Read-Only |
| 13 | `Device.XPON.ONU.{i}.ANI.{i}.Alias` | Unique, non-volatile alias for this instance. *Requires `WAN_MANAGER_UNIFICATION_ENABLED`; otherwise rejected with a warning.* | `PhysicalMedia.{i}.Alias` | _AliasCWMP | Read/Write |
| 14 | `Device.XPON.ONU.{i}.ANI.{i}.LastChange` | The accumulated time in 125 µs frames since the interface entered its current operational state. *Requires `WAN_MANAGER_UNIFICATION_ENABLED`; otherwise rejected with a warning.* | `PhysicalMedia.{i}.LastChange` | unsignedInt | Read-Only |
| 15 | `Device.XPON.ONU.{i}.ANI.{i}.TransceiverNumberOfEntries` | Number of entries in the `Transceiver` table. *Number of populated PhysicalMedia rows.* | `PhysicalMedia table` | unsignedInt | Read-Only |

#### 1.2.3 ANI.TC — ONU Activation

| # | TR-181 Parameter | Description | Source (<code>X_RDK_ONT</code>) | Data Type | Access |
|---|---|---|---|---|---|
| 16 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUState` | ONU activation state. *O1–O9 (the BBF combined O2-3* value is never reported). | `Ploam.RegistrationState` | string | Read-Only |
| 17 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.VendorID` | Identifies the vendor of the ONU. *Truncated to 4 characters.* | `Ploam.VendorId` | string | Read-Only |
| 18 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.SerialNumber` | Represents the combination of the Vendor-ID and the Vendor-specific serial number (VSSN). *Truncated to 12 characters.* | `Ploam.SerialNumber` | string | Read-Only |
| 19 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUID` | Identifier that the OLT assigns to this ANI during the activation. | `Ploam.OnuId` | unsignedInt | Read-Only |

#### 1.2.4 ANI.TC — Performance Monitoring (PHY / GEM / PLOAM / OMCI)

| # | TR-181 Parameter | Description | Source (<code>X_RDK_ONT</code>) | Data Type | Access |
|---|---|---|---|---|---|
| 20 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECBytes` | The number of bytes that were corrected by the FEC function. | `Gtc.CorrectedFecBytes` | StatsCounter64 | Read-Only |
| 21 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECCodewords` | Count of FEC codewords that contained errors but were corrected by the FEC function. | `Gtc.CorrectedFecCodeWords` | StatsCounter64 | Read-Only |
| 22 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UncorrectableFECCodewords` | Count of FEC codewords that contained errors and could not be corrected by the FEC function. | `Gtc.UnCorrectedFecCodeWords` | StatsCounter64 | Read-Only |
| 23 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.TotalFECCodewords` | Count of total received FEC codewords. | `Gtc.TotalFecCodeWords` | StatsCounter64 | Read-Only |
| 24 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.PSBdHECErrorCount` | HEC error in any of the fields of PSBd. | `Gtc.PSBdHecErrors` | StatsCounter32 | Read-Only |
| 25 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.HeaderHECErrorCount` | HEC errors received in the DS XGTC or FS header. *Mapping is a best match (see `XPON-TR181-MAPPING.md`).* | `Gtc.HecErrorCount` | StatsCounter32 | Read-Only |
| 26 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FrameHeaderHECErrors` | XGEM frame header HEC errors. Number of events involving loss of XGEM channel delineation. *Mapping is a best match (see `XPON-TR181-MAPPING.md`).* | `Gtc.FrameHecErrors` | StatsCounter32 | Read-Only |
| 27 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.MICErrors` | PLOAM MIC errors. Counter of received PLOAM messages with MIC errors. | `Ploam.MicErrors` | StatsCounter32 | Read-Only |
| 28 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.DownstreamMessageCount` | Downstream PLOAM message count. Count of PLOAM messages sent by OLT, received by ONU, either broadcast or directed to the specific ONU-ID. | `Ploam.RxMessageCount` | StatsCounter64 | Read-Only |
| 29 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.UpstreamMessageCount` | Upstream PLOAM message count. Count of messages (other than Acknowledgement) sent by ONU to OLT. | `Ploam.TxMessageCount` | StatsCounter64 | Read-Only |
| 30 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.BaselineMessagesReceived` | OMCI baseline message count. | `Omci.RxBaseLineMessageCountValid` | StatsCounter64 | Read-Only |
| 31 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.ExtendedMessagesReceived` | OMCI extended message count. | `Omci.RxExtendedMessageCountValid` | StatsCounter64 | Read-Only |
| 32 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.MICErrors` | Count of received OMCI messages with MIC errors. | `Omci.MicErrors` | StatsCounter32 | Read-Only |

#### 1.2.5 ANI.TC — GEM Ports

| # | TR-181 Parameter | Description | Source (<code>X_RDK_ONT</code>) | Data Type | Access |
|---|---|---|---|---|---|
| 33 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.PortNumberOfEntries` | Number of entries in the `Port` table. *Number of populated `Gem.{i}` rows.* | `Gem table` | unsignedInt | Read-Only |
| 34 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortID` | Identifies a GEM port. | `Gem.{i}.PortId` | unsignedInt | Read-Only |
| 35 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.Direction` | Type of connection this GEM port is used for. *Only Bidirectional* maps; *Unidirectional* is rejected because the HAL omits the direction. | `Gem.{i}.TrafficType` | string | Read-Only |
| 36 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesSent` | (X)GEM frames transmitted by this (X)GEM port. | `Gem.{i}.TransmittedFrames` | StatsCounter64 | Read-Only |
| 37 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesReceived` | (X)GEM frames received by this (X)GEM port. | `Gem.{i}.ReceivedFrames` | StatsCounter64 | Read-Only |

#### 1.2.6 ANI.TC — Performance Thresholds

| # | TR-181 Parameter | Description | Source (<code>X_RDK_ONT</code>) | Data Type | Access |
|---|---|---|---|---|---|
| 38 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalFail` | Signal fail (SF) threshold. This parameter specifies the downstream bit error rate (BER) threshold to detect the SF alarm. | `PhysicalMedia.{i}.PerformanceThreshold.SignalFail` | unsignedInt | Read-Only |
| 39 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalDegrade` | Signal degrade (SD) threshold. This parameter specifies the downstream BER threshold to detect the SD alarm. | `PhysicalMedia.{i}.PerformanceThreshold.SignalDegrade` | unsignedInt | Read-Only |

#### 1.2.7 ANI.TC — Alarms

| # | TR-181 Parameter | Description | Source (<code>X_RDK_ONT</code>) | Data Type | Access |
|---|---|---|---|---|---|
| 40 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOS` | Loss of signal. *Active* → true. | `PhysicalMedia.{i}.Alarm.LOS` | boolean | Read-Only |
| 41 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOF` | Loss of frame. *Active* → true. | `PhysicalMedia.{i}.Alarm.LOF` | boolean | Read-Only |
| 42 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SF` | Signal failed. *Active* → true. | `PhysicalMedia.{i}.Alarm.SF` | boolean | Read-Only |
| 43 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SD` | Signal degraded. *Active* → true. | `PhysicalMedia.{i}.Alarm.SD` | boolean | Read-Only |
| 44 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LCDG` | Loss of GEM channel delineation. *Active* → true. | `PhysicalMedia.{i}.Alarm.LCDG` | boolean | Read-Only |
| 45 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.TF` | Transmitter failure. *Active* → true. | `PhysicalMedia.{i}.Alarm.TF` | boolean | Read-Only |
| 46 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SUF` | Start-up failure. *Active* → true. | `PhysicalMedia.{i}.Alarm.SUF` | boolean | Read-Only |
| 47 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MEM` | Message error message. *Active* → true. | `PhysicalMedia.{i}.Alarm.MEM` | boolean | Read-Only |
| 48 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DACT` | Deactivate ONU-ID. *Active* → true. | `PhysicalMedia.{i}.Alarm.DACT` | boolean | Read-Only |
| 49 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DIS` | Disabled ONU. *Active* → true. | `PhysicalMedia.{i}.Alarm.DIS` | boolean | Read-Only |
| 50 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MIS` | Link mismatching. *Active* → true. | `PhysicalMedia.{i}.Alarm.MIS` | boolean | Read-Only |
| 51 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.PEE` | When the ONU receives a PEE message. *Active* → true. | `PhysicalMedia.{i}.Alarm.PEE` | boolean | Read-Only |
| 52 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.RDI` | Remote defect indication. *Active* → true. | `PhysicalMedia.{i}.Alarm.RDI` | boolean | Read-Only |
| 53 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.ROGUE` | The ANI behaves rogue: it is not transmitting in a manner consistent with parameters specified in the ITU-T PON standards. *Active* → true. | `PhysicalMedia.{i}.Alarm.ROGUE` | boolean | Read-Only |

#### 1.2.8 ANI.Transceiver

| # | TR-181 Parameter | Description | Source (<code>X_RDK_ONT</code>) | Data Type | Access |
|---|---|---|---|---|---|
| 54 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.ID` | The ID as assigned by the CPE to this Transceiver entry. | `PhysicalMedia row index` | unsignedInt | Read-Only |
| 55 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorName` | Vendor name. | `PhysicalMedia.{i}.ModuleVendor` | string | Read-Only |
| 56 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorPartNumber` | Vendor part number. | `PhysicalMedia.{i}.ModuleName` | string | Read-Only |
| 57 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorRevision` | Vendor revision. | `PhysicalMedia.{i}.ModuleVersion` | string | Read-Only |
| 58 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.PONMode` | PON mode, reports the PON mode of the transceiver. *Mapped to G-PON*, *XG-PON*, *NG-PON2*, *XGS-PON*. | `PhysicalMedia.{i}.PonMode` | string | Read-Only |
| 59 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Connector` | Connector type. | `PhysicalMedia.{i}.Connector` | string | Read-Only |
| 60 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateDownstream` | Nominal data rate downstream in 125 µs frames. *Passed through without unit conversion.* | `PhysicalMedia.{i}.NominalBitRateDownstream` | unsignedInt | Read-Only |
| 61 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateUpstream` | Nominal data rate upstream in 125 µs frames. *Passed through without unit conversion.* | `PhysicalMedia.{i}.NominalBitRateUpstream` | unsignedInt | Read-Only |
| 62 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.RxPower` | Measured RX power in 125 µs frames. *Passed through without unit conversion.* | `PhysicalMedia.{i}.RxPower.SignalLevel` | int | Read-Only |
| 63 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.TxPower` | Measured TX power in 125 µs frames. *Passed through without unit conversion.* | `PhysicalMedia.{i}.TxPower.SignalLevel` | int | Read-Only |
| 64 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Voltage` | Measured supply voltage in 125 µs frames. *Negative values are rejected.* | `PhysicalMedia.{i}.Voltage.VoltageLevel` | unsignedInt | Read-Only |
| 65 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Bias` | Measured bias current in 125 µs frames. *Passed through without unit conversion.* | `PhysicalMedia.{i}.Bias.CurrentBias` | unsignedInt | Read-Only |
| 66 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Temperature` | Measured temperature in 125 µs frames. *Values below −274 are rejected.* | `PhysicalMedia.{i}.Temperature.CurrentTemp` | int | Read-Only |

## 2. Telemetry Events (T2)

All events are sent through the RDK T2 framework (`libtelemetry_msgsender`) by the telemetry module in `source/telemetry/`, registered with T2 as component `GponManager` (EPON uses `EponManager`). Markers use the `XPON_` prefix. Events with a value use `t2_event_s(marker, value)`; events without one use `t2_event_d(marker, 1)`. Every event is also logged as `[T2] <PRIORITY> <marker> [value]` in the GPON Manager log, even when T2 is compiled out.

> Telemetry is compiled in only with `--enable-telemetry2-0` (`ENABLE_FEATURE_TELEMETRY2_0`). Otherwise the API stays linked and only writes the log line.

### 2.1 ONU Activation (PLOAM) Events

Derived from transitions of `Device.X_RDK_ONT.Ploam.RegistrationState` (O1–O9) in `Map_hal_dml_ploam()`, for both the HAL push event and polled reads. One state change can raise more than one event (see §2.6).

| Event Name | Meaning | Priority | Value | Estimated Frequency |
|---|---|---|---|---|
| `XPON_ONU_LOS` | Downstream signal lost: the ONU entered *O6* (Intermittent LODS), or fell back to *O1* (Initial) from *O2*–*O5*. | Critical | — | On fibre cut, OLT/splitter failure or optical power loss. Rare in stable networks (0–2/day); bursts during outages. |
| `XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED` | Downstream signal acquired: transition out of *O1* into any state other than O1. | Info | — | Startup and after each LOS recovery. 1–2 per boot. |
| `XPON_ONU_REGISTRATION` | ONU reached *O5* (Operation) — ranging done, ONU-ID assigned, ONU in service. | Info | — | Once per boot; repeats after each recovery (1–3/day). |
| `XPON_ONU_DEREGISTRATION` | ONU left *O5* (Operation) for any other state. | Warning | — | OLT deactivation, LOS, or ranging loss. 0–1/day normally. |
| `XPON_ONU_EMERGENCY_STOP` | ONU entered *O7* (Emergency Stop) — OLT ordered the transmitter off (Disable_Serial_Number PLOAM). GPON-specific; no EPON equivalent. | Critical | — | Only on OLT action against this ONU (rogue / operator disable). Very rare. |

### 2.2 Interface Link Events

Raised by the GPON link state machine (`gponmgr_link_state_machine.c`) when a VEIP link completes a *Down→Up* or *Up→Down* transition and WAN Manager has been told.

| Event Name | Meaning | Priority | Value | Estimated Frequency |
|---|---|---|---|---|
| `XPON_INTF_LINK_UP` | VEIP interface is up and has been reported to WAN Manager. | Info | `Interface=<ifname>` | Once per link-up. Typically 1/boot plus 1 per recovery. |
| `XPON_INTF_LINK_DOWN` | VEIP interface went down and WAN Manager was told. | Warning | `Interface=<ifname>` | Once per link-down. 0–2/day normally. |
| `XPON_PHY_STATUS_UP` | Aggregate PHY state went up: the first VEIP link of the ONU came up (0→1 links up). | Info | — | Once per boot plus 1 per full recovery. |
| `XPON_PHY_STATUS_DOWN` | Aggregate PHY state went down: the last VEIP link went down (1→0 links up). The ONU has no WAN connectivity. | Critical | — | Only on full loss of WAN connectivity. 0–2/day. |

### 2.3 PhysicalMedia Alarms (G.984.3 / G.987.3)

Edge-detected on each `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` field in `Map_hal_dml_pm()`. *Inactive→Active* sends `RAISED`; *Active→Inactive* sends `CLEARED`. Unchanged values send nothing. One field per marker, so an alarm can't be masked by another alarm on the same poll.

| Event Name | Meaning | Priority | Value | Estimated Frequency |
|---|---|---|---|---|
| `XPON_ALARM_LOS` | Loss Of Signal — no downstream optical signal. | Critical | `RAISED\|CLEARED,PM=<n>` | On fibre/optical fault. 0–2/day. |
| `XPON_ALARM_LOF` | Loss Of Frame — downstream frame delineation lost. | Critical | `RAISED\|CLEARED,PM=<n>` | Severe line impairment. Rare. |
| `XPON_ALARM_SF` | Signal Fail — downstream BER above the SF threshold. | Error | `RAISED\|CLEARED,PM=<n>` | Marginal optical budget. Rare. |
| `XPON_ALARM_SD` | Signal Degrade — downstream BER above the SD threshold. | Warning | `RAISED\|CLEARED,PM=<n>` | Early warning of degradation. 0–5/day on marginal links. |
| `XPON_ALARM_LCDG` | Loss of GEM Channel Delineation. | Error | `RAISED\|CLEARED,PM=<n>` | Line impairment. Rare. |
| `XPON_ALARM_TF` | Transmitter Failure — ONU laser fault. | Critical | `RAISED\|CLEARED,PM=<n>` | Hardware fault. Very rare. |
| `XPON_ALARM_SUF` | Start-Up Failure — ranging/activation failed. | Error | `RAISED\|CLEARED,PM=<n>` | Provisioning or ranging problem. Rare. |
| `XPON_ALARM_MEM` | Message Error Message — unknown PLOAM message. | Warning | `RAISED\|CLEARED,PM=<n>` | OLT/ONU interoperability issue. Rare. |
| `XPON_ALARM_DACT` | Deactivate ONU-ID received from the OLT. | Error | `RAISED\|CLEARED,PM=<n>` | OLT-initiated deactivation. Rare. |
| `XPON_ALARM_DIS` | Disabled ONU — OLT disabled this serial number. | Critical | `RAISED\|CLEARED,PM=<n>` | Operator action / rogue isolation. Very rare. |
| `XPON_ALARM_MIS` | Link Mismatch. | Error | `RAISED\|CLEARED,PM=<n>` | Wrong ODN / wavelength. Rare. |
| `XPON_ALARM_PEE` | Physical Equipment Error reported by the OLT. | Error | `RAISED\|CLEARED,PM=<n>` | Hardware fault. Rare. |
| `XPON_ALARM_RDI` | Remote Defect Indication — the OLT sees a defective upstream signal. | Warning | `RAISED\|CLEARED,PM=<n>` | Upstream impairment. 0–2/day on marginal links. |
| `XPON_ALARM_ROGUE` | Rogue ONU — transmitting outside the allocated window. | Critical | `RAISED\|CLEARED,PM=<n>` | ONU laser/timing fault. Very rare. |

### 2.4 System Lifecycle Events

Raised from `ssp_main.c`, `GponMgrDml_Initialize()` and the PhysicalMedia `PonMode` mapper.

| Event Name | Meaning | Priority | Value | Estimated Frequency |
|---|---|---|---|---|
| `XPON_SYSTEM_INIT_SUCCESS` | `GponMgrDml_Initialize()` completed: DML data, HAL connection, initial HAL data and the internal controller are up. Also sent when the optional `Device.XPON` RBUS provider fails, because `Device.X_RDK_ONT` still works; that failure is reported separately as `XPON_ERROR_RBUS_REGISTER_FAILED`. | Info | — | Once per process start. |
| `XPON_SYSTEM_INIT_FAILURE` | Initialisation failed (DML data/HAL init or `Cdm_Init`). The process may exit. | Critical | — | Only on startup failure. |
| `XPON_SYSTEM_SHUTDOWN` | Graceful shutdown requested through the interactive `q` command loop. | Info | — | Only on graceful (non-daemon) shutdown. |
| `XPON_SYSTEM_HAL_WRONG_PON_MODE` | Optical module reports an EPON `PonMode` that GPON Manager can't serve. Latched: sent once until a valid ITU PON mode is reported. | Critical | — | Only on mis-provisioned hardware/HAL. |

### 2.5 Error Events

Operational failures in the HAL, RBUS and state-machine paths. Repeating paths are latched so a persistent failure gives one event per failure streak.

| Event Name | Meaning | Priority | Value | Estimated Frequency |
|---|---|---|---|---|
| `XPON_ERROR_HAL_CALL_FAILED` | JSON-HAL client init/connect failed, a HAL `setParam` request couldn't be sent, or the HAL returned failure. | Error | — | On HAL/daemon unavailability or rejected set. |
| `XPON_ERROR_HAL_EVENT_SUBSCRIBE_FAILED` | Subscribing to HAL push events failed. | Error | — | Startup only. |
| `XPON_ERROR_HAL_EVENT_PARSE_FAILED` | A HAL push event couldn't be parsed or mapped into the data model. | Warning | — | Per malformed HAL event. |
| `XPON_ERROR_STATS_COLLECTION_FAILED` | Bulk HAL data read (`GponHal_get_data`) failed. Latched; re-armed by the next success. | Warning | — | Once per failure streak. |
| `XPON_ERROR_RBUS_PUBLISH_FAILED` | Telling WAN Manager about a VEIP PHY status change failed (in non-unification builds, the Ethernet interface add/enable). Latched per state machine; re-armed by the next success. | Error | — | Once per failure streak. |
| `XPON_ERROR_RBUS_REGISTER_FAILED` | Registering the `Device.XPON` RBUS provider failed. Non-fatal: `Device.X_RDK_ONT` stays available. | Error | — | Startup only. |
| `XPON_ERROR_STATE_MACHINE_START_FAILED` | The VEIP link state machine couldn't be started (allocation or thread creation failed). | Critical | — | Only under resource exhaustion. |

### 2.6 ONU State → Event Mapping

Rules applied by `map_onu_state()`. Several events from one transition are sent in this order: DOWNSTREAM_SIGNAL_DETECTED, LOS, DEREGISTRATION, REGISTRATION, EMERGENCY_STOP. States: O1 Initial, O2 Standby, O3 Serial-Number, O4 Ranging, O5 Operation, O6 Intermittent LODS, O7 Emergency Stop, O8/O9 (G.987/G.9807 sync and profile-learning states).

| Transition | Events (<code>XPON_ONU_*</code>) |
|---|---|
| O1 → O2 / O3 / O4 | DOWNSTREAM_SIGNAL_DETECTED |
| O1 → O5 | DOWNSTREAM_SIGNAL_DETECTED, REGISTRATION |
| O1 → O7 | DOWNSTREAM_SIGNAL_DETECTED, EMERGENCY_STOP |
| O2 / O3 / O4 → O5 | REGISTRATION |
| O6 → O5 | REGISTRATION |
| O2 / O3 / O4 → O1 | LOS |
| O5 → O1 | LOS, DEREGISTRATION |
| O5 → O6 | LOS, DEREGISTRATION |
| O5 → O7 | DEREGISTRATION, EMERGENCY_STOP |
| O5 → O2 / O3 / O4 / O8 / O9 | DEREGISTRATION |
| any other → O6 | LOS |
| any other → O7 | EMERGENCY_STOP |
| O6 / O7 / O8 / O9 → O1 | none |
| unchanged | none |

### 2.7 Value Formats & Duplicate Suppression

| Format | Value | Used by | T2 call |
|---|---|---|---|
| None | — | ONU, PHY status, lifecycle, error events | `t2_event_d(marker, 1)` |
| Interface | `Interface=<ifname>` (`unknown` if empty) | `XPON_INTF_LINK_UP/DOWN` | `t2_event_s(marker, value)` |
| Alarm | `RAISED,PM=<n>` / `CLEARED,PM=<n>` (n = PhysicalMedia instance) | `XPON_ALARM_*` | `t2_event_s(marker, value)` |

| Mechanism | Applies to | Behaviour |
|---|---|---|
| Edge detection | ONU state, alarms | Previous vs. new value compared under the DML data lock; nothing sent when unchanged. Defaults (*O1*, all alarms *Inactive*) mean the first read gives no false events. |
| Transition detection | INTF_LINK_*, PHY_STATUS_* | Only on successful state-machine transitions. PHY_STATUS uses a mutex-protected count of links up (0↔1 edges). |
| Latch (one per streak) | STATS_COLLECTION_FAILED, RBUS_PUBLISH_FAILED, HAL_WRONG_PON_MODE | Sent on the first failure only and re-armed by the next success, so retry loops (500 ms state machine, init busy-loop) can't flood T2. |
| Pre-init drop | All | Events raised before `xponMgr_telemetry_init()` or after cleanup are dropped silently. |

### 2.8 Priority Definitions

| Priority | Description | Action |
|---|---|---|
| Critical | Service-affecting: the ONU has lost (or can't get) WAN connectivity, or the component can't run. | Immediate investigation. Correlate with ONU state and optical levels. |
| Error | Function degraded or a request failed, but service may continue. | Investigate if it persists or repeats. |
| Warning | Abnormal condition that may lead to an error. | Trend and alert on rate. |
| Info | Normal lifecycle / recovery milestone. | Use for availability and MTTR analytics. |

### 2.9 Summary Statistics

| Category | Total Events | Critical | Error | Warning | Info |
|---|---|---|---|---|---|
| ONU Activation (PLOAM) Events | 5 | 2 | 0 | 1 | 2 |
| Interface Link Events | 4 | 1 | 0 | 1 | 2 |
| PhysicalMedia Alarms (G.984.3 / G.987.3) | 14 | 5 | 6 | 3 | 0 |
| System Lifecycle Events | 4 | 2 | 0 | 0 | 2 |
| Error Events | 7 | 1 | 4 | 2 | 0 |
| **Total** | **34** | **11** | **10** | **7** | **6** |

> **Note:** Each alarm event carries a raised/cleared state in its value. The listed priority is for the raised condition; cleared events are informational.

### 2.10 Parity with EPON Manager

The GPON module mirrors the EPON Manager telemetry (`rdk-eponmanager`, `src/telemetry/`): same API shape, marker naming, priority scheme, value formats and T2 encoding. Differences come only from the technology.

| EPON Event(s) | GPON Event(s) | Notes |
|---|---|---|
| `EPON_ONU_LOS / _DOWNSTREAM_SIGNAL_DETECTED / _REGISTRATION / _DEREGISTRATION` | `XPON_ONU_LOS / _DOWNSTREAM_SIGNAL_DETECTED / _REGISTRATION / _DEREGISTRATION` | Same names and priorities; driven by PLOAM O-states instead of MPCP/OAM state. |
| — | `XPON_ONU_EMERGENCY_STOP` | GPON-only (O7, Disable_Serial_Number). |
| `EPON_INTF_LINK_UP / _DOWN, EPON_PHY_STATUS_UP / _DOWN` | `XPON_INTF_LINK_UP / _DOWN, XPON_PHY_STATUS_UP / _DOWN` | Same semantics and value format (`Interface=<ifname>`). |
| `EPON_ALARM_STD_* (7), EPON_ALARM_VENDOR_* (8)` | `XPON_ALARM_* (14)` | Technology-specific: the GPON set is the 14 ITU-T G.984.3/G.987.3 alarms in `PhysicalMedia.{i}.Alarm`. Same `RAISED`/`CLEARED` value semantics. |
| `EPON_SYSTEM_INIT_SUCCESS / _INIT_FAILURE / _SHUTDOWN / _HAL_WRONG_PON_MODE` | `XPON_SYSTEM_INIT_SUCCESS / _INIT_FAILURE / _SHUTDOWN / _HAL_WRONG_PON_MODE` | Same. Wrong PON mode = module reports EPON. SHUTDOWN only on the graceful `q` path. |
| `EPON_SYSTEM_FACTORY_RESET, EPON_SYSTEM_ONU_RESET` | — | Not applicable: GPON Manager exposes no factory-reset / ONU-reset operation. |
| `EPON_ERROR_HAL_CALL_FAILED, _STATS_COLLECTION_FAILED, _RBUS_PUBLISH_FAILED` | `XPON_ERROR_HAL_CALL_FAILED, _STATS_COLLECTION_FAILED, _RBUS_PUBLISH_FAILED` | Same names and priorities. |
| `EPON_ERROR_EVENT_QUEUE_FULL` | — | Not applicable: GPON Manager has no internal event queue (HAL callbacks update the DML directly). |
| `EPON_ERROR_PSM_ACCESS_FAILED` | — | Not applicable: GPON Manager doesn't use PSM. |
| — | `XPON_ERROR_HAL_EVENT_SUBSCRIBE_FAILED, _HAL_EVENT_PARSE_FAILED, _RBUS_REGISTER_FAILED, _STATE_MACHINE_START_FAILED` | GPON-only failure points with no EPON equivalent. |

## Appendix A. Unsupported `Device.XPON` Parameters

These 69 parameters are registered in `g_elements[]` (`gponmgr_xpon_tr181.c`) to keep the BBF tree complete, but the unified GPON HAL has no matching field. A get returns `RBUS_ERROR_INVALID_OPERATION` and logs `XPON: BBF TR-181 <name> not supported yet`. Writable ones reject a set the same way, and subscriptions are rejected. No telemetry is sent for them.

> `Device.XPON.ONU.{i}.SoftwareImage.{i}` is registered as a table, but no rows are ever added because the HAL has no software-image inventory. Its parameters can't be reached at run time.

### A.1 ONU (6)

| # | TR-181 Parameter | BBF Description | BBF Type | BBF Access | Behaviour |
|---|---|---|---|---|---|
| 1 | `Device.XPON.ONU.{i}.Enable` | Enables or disables this ONU. If disabled, the ONU should prevent user traffic from flowing, suppress notifications, and power down as much as possible. | boolean | Read/Write | Get/Set rejected |
| 2 | `Device.XPON.ONU.{i}.PowerCapability` | Describes the possible power states of the ONU regardless of its current administrative state. | `list<string>` | Read-Only | Get rejected |
| 3 | `Device.XPON.ONU.{i}.PowerStatus` | Reports the current power status of the ONU. | string | Read-Only | Get rejected |
| 4 | `Device.XPON.ONU.{i}.Name` | The textual name of the ONU as assigned by the CPE. | string | Read-Only | Get rejected |
| 5 | `Device.XPON.ONU.{i}.Version` | This attribute identifies the version of the ONU as defined by the vendor. | string | Read-Only | Get rejected |
| 6 | `Device.XPON.ONU.{i}.EquipmentID` | This attribute may be used to identify the specific type of ONU. | string | Read-Only | Get rejected |

### A.2 ONU.EthernetUNI (21)

| # | TR-181 Parameter | BBF Description | BBF Type | BBF Access | Behaviour |
|---|---|---|---|---|---|
| 7 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Alias` | Unique, non-volatile alias for this instance. | _AliasCWMP | Read/Write | Get/Set rejected |
| 8 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Description` | A user-friendly description of the interface. | string | Read/Write | Get/Set rejected |
| 9 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.LastChange` | The accumulated time in 125 µs frames since the interface entered its current operational state. | unsignedInt | Read-Only | Get rejected |
| 10 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.LowerLayers` | Comma-separated list of references to an interface object that is stacked immediately below this interface object. | `list<string>` | Read/Write | Get/Set rejected |
| 11 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Upstream` | Indicates whether the interface points towards the Internet or towards End Devices. | boolean | Read-Only | Get rejected |
| 12 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.Reset` | Set this to to reset all this object's statistics counters to zero. | boolean | Read/Write | Get/Set rejected |
| 13 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesSent` | The total number of bytes transmitted out of the interface, including framing characters. | StatsCounter64 | Read-Only | Get rejected |
| 14 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesReceived` | The total number of bytes received on the interface, including framing characters. | StatsCounter64 | Read-Only | Get rejected |
| 15 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsSent` | The total number of packets transmitted out of the interface. | StatsCounter64 | Read-Only | Get rejected |
| 16 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsReceived` | The total number of packets received on the interface. | StatsCounter64 | Read-Only | Get rejected |
| 17 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsSent` | The total number of outbound 125 µs frames that could not be transmitted because of errors. | StatsCounter32 | Read-Only | Get rejected |
| 18 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsReceived` | The total number of inbound 125 µs frames that contained errors preventing them from being delivered to a higher-layer protocol. | StatsCounter32 | Read-Only | Get rejected |
| 19 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsSent` | The total number of packets requested for transmission which were not addressed to a multicast or broadcast address at this layer, including those that were discarded or not sent. | StatsCounter64 | Read-Only | Get rejected |
| 20 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsSent` | The total number of outbound packets which were chosen to be discarded even though no errors had been detected to prevent their being transmitted. | StatsCounter32 | Read-Only | Get rejected |
| 21 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsReceived` | The total number of inbound packets which were chosen to be discarded even though no errors had been detected to prevent their being delivered. | StatsCounter32 | Read-Only | Get rejected |
| 22 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsSent` | The total number of packets that higher-level protocols requested for transmission and which were addressed to a multicast address at this layer, including those that were discarded or not sent. | StatsCounter64 | Read-Only | Get rejected |
| 23 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsReceived` | The total number of received packets, delivered by this layer to a higher layer, which were not addressed to a multicast or broadcast address at this layer. | StatsCounter64 | Read-Only | Get rejected |
| 24 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsReceived` | The total number of received packets, delivered by this layer to a higher layer, which were addressed to a multicast address at this layer. | StatsCounter64 | Read-Only | Get rejected |
| 25 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsSent` | The total number of packets that higher-level protocols requested for transmission and which were addressed to a broadcast address at this layer, including those that were discarded or not sent. | StatsCounter64 | Read-Only | Get rejected |
| 26 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsReceived` | The total number of received packets, delivered by this layer to a higher layer, which were addressed to a broadcast address at this layer. | StatsCounter64 | Read-Only | Get rejected |
| 27 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnknownProtoPacketsReceived` | The total number of packets received via the interface which were discarded because of an unknown or unsupported protocol. | StatsCounter32 | Read-Only | Get rejected |

### A.3 ONU.SoftwareImage (5)

| # | TR-181 Parameter | BBF Description | BBF Type | BBF Access | Behaviour |
|---|---|---|---|---|---|
| 28 | `Device.XPON.ONU.{i}.SoftwareImage.{i}.ID` | The ID as assigned by the CPE to this SoftwareImage entry. | unsignedInt | Read-Only | Get rejected |
| 29 | `Device.XPON.ONU.{i}.SoftwareImage.{i}.Version` | A string identifying the software version. | string | Read-Only | Get rejected |
| 30 | `Device.XPON.ONU.{i}.SoftwareImage.{i}.IsCommitted` | This parameter indicates whether or not the associated software image is committed. | boolean | Read-Only | Get rejected |
| 31 | `Device.XPON.ONU.{i}.SoftwareImage.{i}.IsActive` | This parameter indicates whether the associated software image is active or inactive. | boolean | Read-Only | Get rejected |
| 32 | `Device.XPON.ONU.{i}.SoftwareImage.{i}.IsValid` | This parameter indicates whether the associated software image is valid or invalid. | boolean | Read-Only | Get rejected |

### A.4 ONU.ANI (base & Stats) (18)

| # | TR-181 Parameter | BBF Description | BBF Type | BBF Access | Behaviour |
|---|---|---|---|---|---|
| 33 | `Device.XPON.ONU.{i}.ANI.{i}.Name` | The textual name of the ANI entry as assigned by the CPE. | string | Read-Only | Get rejected |
| 34 | `Device.XPON.ONU.{i}.ANI.{i}.PONMode` | PON mode, defines the detected or configured PON mode of the ANI. | string | Read-Only | Get rejected |
| 35 | `Device.XPON.ONU.{i}.ANI.{i}.SFPReferenceList` | Small-form Factor Pluggable (SFP) entries associated with this interface. | `list<string>` | Read-Only | Get rejected |
| 36 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesSent` | The total number of bytes transmitted out of the interface, including framing characters. | StatsCounter64 | Read-Only | Get rejected |
| 37 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesReceived` | The total number of bytes received on the interface, including framing characters. | StatsCounter64 | Read-Only | Get rejected |
| 38 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsSent` | The total number of Ethernet frames transmitted out of the interface. | StatsCounter64 | Read-Only | Get rejected |
| 39 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsReceived` | The total number of Ethernet frames received on the interface. | StatsCounter64 | Read-Only | Get rejected |
| 40 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsSent` | The total number of outbound Ethernet frames that could not be transmitted because of errors. | StatsCounter32 | Read-Only | Get rejected |
| 41 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsReceived` | The total number of inbound Ethernet frames that contained errors preventing them from being forwarded to the UNI(s). | StatsCounter32 | Read-Only | Get rejected |
| 42 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsSent` | The total number of Ethernet frames requested for transmission which were not addressed to a multicast or broadcast address at this layer, including those that were discarded or not sent. | StatsCounter64 | Read-Only | Get rejected |
| 43 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsReceived` | The total number of Ethernet frames, forwarded by this interface to the UNI(s), which were not addressed to a multicast or broadcast address at this layer. | StatsCounter64 | Read-Only | Get rejected |
| 44 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsSent` | The total number of outbound Ethernet frames which were chosen to be discarded even though no errors had been detected to prevent their being transmitted. | StatsCounter32 | Read-Only | Get rejected |
| 45 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsReceived` | The total number of inbound Ethernet frames which were chosen to be discarded even though no errors had been detected to prevent their being delivered. | StatsCounter32 | Read-Only | Get rejected |
| 46 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsSent` | The total number of Ethernet frames requested for transmission and which were addressed to a multicast address at this layer, including those that were discarded or not sent. | StatsCounter64 | Read-Only | Get rejected |
| 47 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsReceived` | The total number of received Ethernet frames, forwarded by this interface to the UNI(s), which were addressed to a multicast address at this layer. | StatsCounter64 | Read-Only | Get rejected |
| 48 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsSent` | The total number of Ethernet frames requested for transmission on the PON and which were addressed to a broadcast address at this layer, including those that were discarded or not sent. | StatsCounter64 | Read-Only | Get rejected |
| 49 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsReceived` | The total number of received Ethernet frames, forwarded by this interface to the UNI(s), which were addressed to a broadcast address at this layer. | StatsCounter64 | Read-Only | Get rejected |
| 50 | `Device.XPON.ONU.{i}.ANI.{i}.Stats.UnknownProtoPacketsReceived` | The total number of Ethernet frames received via the interface which were discarded because of an unknown or unsupported protocol. | StatsCounter32 | Read-Only | Get rejected |

### A.5 ONU.ANI.TC (9)

| # | TR-181 Parameter | BBF Description | BBF Type | BBF Access | Behaviour |
|---|---|---|---|---|---|
| 51 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.Password` | All ITU based PON standards specify authentication by PLOAM password or registration ID. | string | Read/Write | Get/Set rejected |
| 52 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.HexadecimalPassword` | If, `Password` is in ASCII format. | boolean | Read/Write | Get/Set rejected |
| 53 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UnknownProfile` | ONU could not transmit because the specified burst profile was not known. | StatsCounter32 | Read-Only | Get rejected |
| 54 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesSent` | Total number of (X)GEM frames transmitted. | StatsCounter64 | Read-Only | Get rejected |
| 55 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesReceived` | Total number of (X)GEM frames received. | StatsCounter64 | Read-Only | Get rejected |
| 56 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.KeyErrors` | XGEM key errors. XGEM frames discarded because of unknown or invalid encryption key. | StatsCounter32 | Read-Only | Get rejected |
| 57 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.RangingTime` | Count of Ranging_Time PLOAM messages sent by OLT. | StatsCounter64 | Read-Only | Get rejected |
| 58 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortType` | GEM port type. | string | Read-Only | Get rejected |
| 59 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LODS` | Loss of downstream synchronization. | boolean | Read-Only | Get rejected |

### A.6 ONU.ANI.Transceiver (1)

| # | TR-181 Parameter | BBF Description | BBF Type | BBF Access | Behaviour |
|---|---|---|---|---|---|
| 60 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Identifier` | Transceiver type. Allowed values are given by. | unsignedInt | Read-Only | Get rejected |

### A.7 ONU.PowerManagement (9)

| # | TR-181 Parameter | BBF Description | BBF Type | BBF Access | Behaviour |
|---|---|---|---|---|---|
| 61 | `Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementCapability` | This parameter indicates the sleep mode capability of the PowerManagement. | `list<string>` | Read-Only | Get rejected |
| 62 | `Device.XPON.ONU.{i}.PowerManagement.OLTSleepAllow` | This parameter indicates that the ONU is allowed, by the OLT, to apply the power reduction mode. | boolean | Read-Only | Get rejected |
| 63 | `Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementStatus` | This parameter indicates the current power reduction management status of the ONU. | string | Read-Only | Get rejected |
| 64 | `Device.XPON.ONU.{i}.PowerManagement.ITransInit` | Optical Network Unit (ONU) vendor's statement of the complete transceiver initialization time: the worst-case time required for the ONU to regain full functionality when leaving the asleep state (i.e., turning on both the receiver and the transmitter and acquiring synchronization to the downstream flow), measured in units of 125 µs frames. | unsignedInt | Read-Only | Get rejected |
| 65 | `Device.XPON.ONU.{i}.PowerManagement.ITxInit` | This parameter is the Optical Network Unit (ONU) vendor's statement of the transmitter initialization time: the time required for the ONU to regain full functionality when leaving the listen state (i.e., turning on the transmitter), measured in units of 125 µs frames. | unsignedInt | Read-Only | Get rejected |
| 66 | `Device.XPON.ONU.{i}.PowerManagement.MaxSleepInterval` | The Isleep/Ilowpower attribute specifies the maximum time the ONU spends in its asleep, listen, or low-power states, as a count of 125 µs frames. | unsignedInt | Read-Only | Get rejected |
| 67 | `Device.XPON.ONU.{i}.PowerManagement.MinAwareInterval` | The Iaware attribute specifies the time the ONU spends in its aware state, as a count of 125 µs frames, before it re-enters asleep or listen states. | unsignedInt | Read-Only | Get rejected |
| 68 | `Device.XPON.ONU.{i}.PowerManagement.MinActiveHeldInterval` | The Ihold attribute specifies the minimum time during which the ONU remains in the active held state, as a count of 125 µs frames. | unsignedInt | Read-Only | Get rejected |
| 69 | `Device.XPON.ONU.{i}.PowerManagement.MaxSleepIntervalExtension` | This parameter designates maximum sleep interval values for doze mode and cyclic sleep mode separately. | unsignedLong | Read-Only | Get rejected |

## References

- BBF TR-181 Issue 2 Amendment 21 — Device Data Model (`Device.XPON`)
- ITU-T G.984.3 (GPON TC), G.987.3 (XG-PON TC), G.9807.1 (XGS-PON), G.989.3 (NG-PON2) — activation states and alarms
- RDK T2 Telemetry Framework (`telemetry_busmessage_sender.h`)
- GPON Manager data model `config/RdkGponManager.xml`
- RDK → BBF mapping notes `XPON-TR181-MAPPING.md`
- GPON telemetry design `docs/07_Telemetry_Design.md`
- EPON Manager telemetry design (`rdk-eponmanager/design_docs/07_Telemetry_Design.md`, `08_TR181_Telemetry_Reference`)
