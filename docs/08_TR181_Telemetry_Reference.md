# 08 — GPON Manager TR-181 & Telemetry Reference

**Document:** 08 | **Version:** 1.2 | **Date:** October 29, 2026 | **Component:** RDK GPON Manager (`rdkgponmanager`)

> An HTML version of this document with the same content, styled for printing, is in [08_TR181_Telemetry_Reference.html](08_TR181_Telemetry_Reference.html).

## Table of Contents

1. [TR-181 Data Model Parameters](#1-tr-181-data-model-parameters)
   1. [`Device.XPON` (BBF TR-181 2.21) — Supported Parameters](#11-devicexpon-bbf-tr-181-221--supported-parameters)
      1. [XPON Root, ONU & EthernetUNI](#111-xpon-root-onu--ethernetuni)
      2. [ANI](#112-ani)
      3. [ANI.TC — ONU Activation](#113-anitc--onu-activation)
      4. [ANI.TC — Performance Monitoring (PHY / GEM / PLOAM / OMCI)](#114-anitc--performance-monitoring-phy--gem--ploam--omci)
      5. [ANI.TC — GEM Ports](#115-anitc--gem-ports)
      6. [ANI.TC — Performance Thresholds](#116-anitc--performance-thresholds)
      7. [ANI.TC — Alarms](#117-anitc--alarms)
      8. [ANI.Transceiver](#118-anitransceiver)
   2. [`Device.X_RDK_ONT` — RDK Extensions](#12-devicex_rdk_ont--rdk-extensions)
      1. [PhysicalMedia — Module & Interface](#121-physicalmedia--module--interface)
      2. [PhysicalMedia — Optical Power Thresholds](#122-physicalmedia--optical-power-thresholds)
      3. [GTC Counters](#123-gtc-counters)
      4. [PLOAM](#124-ploam)
      5. [GEM Port Ethernet Flow (VLAN Classification)](#125-gem-port-ethernet-flow-vlan-classification)
      6. [VEIP Ethernet Flow (VLAN Classification)](#126-veip-ethernet-flow-vlan-classification)
      7. [TR-069 Management (OMCI)](#127-tr-069-management-omci)
2. [Telemetry Events (T2)](#2-telemetry-events-t2)
   1. [ONU Activation (PLOAM) Events](#21-onu-activation-ploam-events)
   2. [Interface Link Events](#22-interface-link-events)
   3. [PhysicalMedia Alarms (G.984.3 / G.987.3)](#23-physicalmedia-alarms-g9843--g9873)
   4. [System Lifecycle Events](#24-system-lifecycle-events)
   5. [Error Events](#25-error-events)
   6. [ONU State → Event Mapping](#26-onu-state--event-mapping)
      1. [ONU Activation States](#261-onu-activation-states)
      2. [Transition Reference](#262-transition-reference)
      3. [Common Sequences](#263-common-sequences)
   7. [Value Formats & Duplicate Suppression](#27-value-formats--duplicate-suppression)
   8. [Priority Definitions](#28-priority-definitions)
   9. [Summary Statistics](#29-summary-statistics)
- [Appendix A. Unsupported `Device.XPON` Parameters](#appendix-a-unsupported-devicexpon-parameters)
- [Appendix B. `Device.XPON` ↔ `Device.X_RDK_ONT` Mapping](#appendix-b-devicexpon--devicex_rdk_ont-mapping)
  - [B.1 Supported `Device.XPON` → `Device.X_RDK_ONT`](#b1-supported-devicexpon--devicex_rdk_ont)
  - [B.2 Unsupported `Device.XPON` with Existing `X_RDK_ONT` Data](#b2-unsupported-devicexpon-with-existing-x_rdk_ont-data)
- [References](#references)

## 1. TR-181 Data Model Parameters

GPON Manager exposes the standard BBF `Device.XPON` model and the RDK `Device.X_RDK_ONT` model. Both are filled from the same in-memory DML structure (`GPON_DML_DATA`), which the JSON-HAL client keeps up to date from HAL push events and on-demand reads.

> **Precedence:** `Device.XPON` is the primary model (§1.1). `Device.X_RDK_ONT` is listed only as an extension (§1.2), with the 37 parameters that have no equivalent anywhere in BBF TR-181 2.21 `Device.XPON`. The other 58 `X_RDK_ONT` parameters carry the same data as a supported `Device.XPON` parameter and are listed in Appendix B only. New integrations should use `Device.XPON` wherever it has the data.

> **Legend:** Read-Only = Green   Read/Write = Red   Table = Dynamic table (`{i}`) populated as the HAL reports instances

### 1.1 `Device.XPON` (BBF TR-181 2.21) — Supported Parameters

Registered as an RBUS provider by `source/TR-181/rbus/tr181/gponmgr_xpon_tr181.c` (`GponMgrXponRbus_Init()`), which registers the complete BBF 2.21 `Device.XPON` tree. 66 of the 135 registered parameters are backed by GPON data, as listed below. The other 69 return `RBUS_ERROR_INVALID_OPERATION`; see Appendix A. The *X_RDK_ONT Equivalent* column names the backing RDK parameter; the full mapping is in Appendix B.

> **Instances:** only `ONU.1` and `ANI.1` exist. `EthernetUNI.{i}`, `ANI.1.Transceiver.{i}` and `TC.GEM.Port.{i}` follow the populated `Veip`, `PhysicalMedia` and `Gem` rows. ANI-level PhysicalMedia fields need exactly one active PhysicalMedia row. Values are passed through without unit conversion (BBF units: power 0.1 dBm, voltage mV, bias µA, temperature °C, bit rate kbps).

#### 1.1.1 XPON Root, ONU & EthernetUNI

| # | TR-181 Parameter | Description | X_RDK_ONT Equivalent | Data Type | Access |
|---|---|---|---|---|---|
| 1 | `Device.XPON.ONUNumberOfEntries` | Number of entries in the `ONU` table. *Always 1.* | `constant` | unsignedInt | Read-Only |
| 2 | `Device.XPON.ONU.{i}.SoftwareImageNumberOfEntries` | Number of entries in the `SoftwareImage` table. *Always 0 — no software-image rows are registered.* | `constant` | unsignedInt | Read-Only |
| 3 | `Device.XPON.ONU.{i}.EthernetUNINumberOfEntries` | Number of entries in the `EthernetUNI` table. *Number of populated `Veip.{i}` rows.* | `Veip table` | unsignedInt | Read-Only |
| 4 | `Device.XPON.ONU.{i}.ANINumberOfEntries` | Number of entries in the `ANI` table. *Always 1.* | `constant` | unsignedInt | Read-Only |
| 5 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Enable` | Indicates whether this interface is enabled or not. *Unlock* → true, *Lock* → false. **Telemetry:** Every change of the backing OMCI AdministrativeState raises `XPON_INTF_ADMIN_LOCK` / `_UNLOCK` (§2.2). | `Veip.{i}.AdministrativeState` | boolean | Read-Only |
| 6 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Status` | The current operational state of the interface. *Same enumeration.* **Telemetry:** *Up*/*Down* drives the VEIP link state machine and the `XPON_INTF_LINK_*` / `XPON_PHY_STATUS_*` events (§2.2). | `Veip.{i}.OperationalState` | string | Read-Only |
| 7 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Name` | The textual name of the interface as assigned by the CPE. *Get fails if longer than 64 characters.* **Telemetry:** Carried as `Interface=<name>` in `XPON_INTF_LINK_*` (§2.2). | `Veip.{i}.InterfaceName` | string | Read-Only |
| 8 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.ANIs` | References all associated ANI interfaces. *Always `Device.XPON.ONU.1.ANI.1`.* | `constant` | `list<string>` | Read-Only |
| 9 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainID` | String to identify the associated service in the ONU management domain to the TR-181 management domain. *Formatted as `(VEIP,<MeId>)`.* | `Veip.{i}.MeId` | string | Read-Only |
| 10 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainName` | String that provides an additional way to identify the associated service to the TR-181 management domain. | `Veip.{i}.InterDomainName` | string | Read-Only |

#### 1.1.2 ANI

| # | TR-181 Parameter | Description | X_RDK_ONT Equivalent | Data Type | Access |
|---|---|---|---|---|---|
| 11 | `Device.XPON.ONU.{i}.ANI.{i}.Enable` | Enables or disables the interface. If disabled, the device should force the ONU state of this ANI to O1 (Initial state). | `PhysicalMedia.{i}.Enable` | boolean | Read/Write |
| 12 | `Device.XPON.ONU.{i}.ANI.{i}.Status` | The current operational state of the interface. *Uses the uniquely active PhysicalMedia row.* | `PhysicalMedia.{i}.Status` | string | Read-Only |
| 13 | `Device.XPON.ONU.{i}.ANI.{i}.Alias` | Unique, non-volatile alias for this instance. | `PhysicalMedia.{i}.Alias` | _AliasCWMP | Read/Write |
| 14 | `Device.XPON.ONU.{i}.ANI.{i}.LastChange` | The accumulated time in seconds since the interface entered its current operational state. | `PhysicalMedia.{i}.LastChange` | unsignedInt | Read-Only |
| 15 | `Device.XPON.ONU.{i}.ANI.{i}.TransceiverNumberOfEntries` | Number of entries in the `Transceiver` table. *Number of populated PhysicalMedia rows.* | `PhysicalMedia table` | unsignedInt | Read-Only |

#### 1.1.3 ANI.TC — ONU Activation

| # | TR-181 Parameter | Description | X_RDK_ONT Equivalent | Data Type | Access |
|---|---|---|---|---|---|
| 16 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUState` | ONU activation state. *O1–O9 (the BBF combined O2-3* value is never reported). **Telemetry:** Every change raises `XPON_ONU_STATE_CHANGE` (`From=O<a>,To=O<b>`). Milestone transitions (leaving O1, falling back to O1, entering O6/O7, leaving O7, entering/leaving O5) also raise the specific `XPON_ONU_*` events (§2.1, mapping in §2.6). | `Ploam.RegistrationState` | string | Read-Only |
| 17 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.VendorID` | Identifies the vendor of the ONU. *Get fails if longer than 4 characters.* | `Ploam.VendorId` | string | Read-Only |
| 18 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.SerialNumber` | Represents the combination of the Vendor-ID and the Vendor-specific serial number (VSSN). *Get fails if not a valid 12-character (or 16 hex digit) serial.* | `Ploam.SerialNumber` | string | Read-Only |
| 19 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUID` | Identifier that the OLT assigns to this ANI during the activation. | `Ploam.OnuId` | unsignedInt | Read-Only |

#### 1.1.4 ANI.TC — Performance Monitoring (PHY / GEM / PLOAM / OMCI)

| # | TR-181 Parameter | Description | X_RDK_ONT Equivalent | Data Type | Access |
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

#### 1.1.5 ANI.TC — GEM Ports

| # | TR-181 Parameter | Description | X_RDK_ONT Equivalent | Data Type | Access |
|---|---|---|---|---|---|
| 33 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.PortNumberOfEntries` | Number of entries in the `Port` table. *Number of populated `Gem.{i}` rows.* | `Gem table` | unsignedInt | Read-Only |
| 34 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortID` | Identifies a GEM port. | `Gem.{i}.PortId` | unsignedInt | Read-Only |
| 35 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.Direction` | Type of connection this GEM port is used for. *Only Bidirectional* maps; *Unidirectional* is rejected because the HAL omits the direction. | `Gem.{i}.TrafficType` | string | Read-Only |
| 36 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesSent` | (X)GEM frames transmitted by this (X)GEM port. | `Gem.{i}.TransmittedFrames` | StatsCounter64 | Read-Only |
| 37 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesReceived` | (X)GEM frames received by this (X)GEM port. | `Gem.{i}.ReceivedFrames` | StatsCounter64 | Read-Only |

#### 1.1.6 ANI.TC — Performance Thresholds

| # | TR-181 Parameter | Description | X_RDK_ONT Equivalent | Data Type | Access |
|---|---|---|---|---|---|
| 38 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalFail` | Signal fail (SF) threshold. This parameter specifies the downstream bit error rate (BER) threshold to detect the SF alarm. | `PhysicalMedia.{i}.PerformanceThreshold.SignalFail` | unsignedInt | Read-Only |
| 39 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalDegrade` | Signal degrade (SD) threshold. This parameter specifies the downstream BER threshold to detect the SD alarm. | `PhysicalMedia.{i}.PerformanceThreshold.SignalDegrade` | unsignedInt | Read-Only |

#### 1.1.7 ANI.TC — Alarms

| # | TR-181 Parameter | Description | X_RDK_ONT Equivalent | Data Type | Access |
|---|---|---|---|---|---|
| 40 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOS` | Loss of signal. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_LOS` (§2.3). | `PhysicalMedia.{i}.Alarm.LOS` | boolean | Read-Only |
| 41 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOF` | Loss of frame. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_LOF` (§2.3). | `PhysicalMedia.{i}.Alarm.LOF` | boolean | Read-Only |
| 42 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SF` | Signal failed. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_SF` (§2.3). | `PhysicalMedia.{i}.Alarm.SF` | boolean | Read-Only |
| 43 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SD` | Signal degraded. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_SD` (§2.3). | `PhysicalMedia.{i}.Alarm.SD` | boolean | Read-Only |
| 44 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LCDG` | Loss of GEM channel delineation. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_LCDG` (§2.3). | `PhysicalMedia.{i}.Alarm.LCDG` | boolean | Read-Only |
| 45 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.TF` | Transmitter failure. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_TF` (§2.3). | `PhysicalMedia.{i}.Alarm.TF` | boolean | Read-Only |
| 46 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SUF` | Start-up failure. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_SUF` (§2.3). | `PhysicalMedia.{i}.Alarm.SUF` | boolean | Read-Only |
| 47 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MEM` | Message error message. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_MEM` (§2.3). | `PhysicalMedia.{i}.Alarm.MEM` | boolean | Read-Only |
| 48 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DACT` | Deactivate ONU-ID. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_DACT` (§2.3). | `PhysicalMedia.{i}.Alarm.DACT` | boolean | Read-Only |
| 49 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DIS` | Disabled ONU. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_DIS` (§2.3). | `PhysicalMedia.{i}.Alarm.DIS` | boolean | Read-Only |
| 50 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MIS` | Link mismatching. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_MIS` (§2.3). | `PhysicalMedia.{i}.Alarm.MIS` | boolean | Read-Only |
| 51 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.PEE` | When the ONU receives a PEE message. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_PEE` (§2.3). | `PhysicalMedia.{i}.Alarm.PEE` | boolean | Read-Only |
| 52 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.RDI` | Remote defect indication. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_RDI` (§2.3). | `PhysicalMedia.{i}.Alarm.RDI` | boolean | Read-Only |
| 53 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.ROGUE` | The ANI behaves rogue: it is not transmitting in a manner consistent with parameters specified in the ITU-T PON standards. *Active* → true. **Telemetry:** Every edge raises `XPON_ALARM_ROGUE` (§2.3). | `PhysicalMedia.{i}.Alarm.ROGUE` | boolean | Read-Only |

#### 1.1.8 ANI.Transceiver

| # | TR-181 Parameter | Description | X_RDK_ONT Equivalent | Data Type | Access |
|---|---|---|---|---|---|
| 54 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.ID` | The ID as assigned by the CPE to this Transceiver entry. | `PhysicalMedia row index` | unsignedInt | Read-Only |
| 55 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorName` | Vendor name. | `PhysicalMedia.{i}.ModuleVendor` | string | Read-Only |
| 56 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorPartNumber` | Vendor part number. | `PhysicalMedia.{i}.ModuleName` | string | Read-Only |
| 57 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorRevision` | Vendor revision. | `PhysicalMedia.{i}.ModuleVersion` | string | Read-Only |
| 58 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.PONMode` | PON mode, reports the PON mode of the transceiver. *Mapped to G-PON*, *XG-PON*, *NG-PON2*, *XGS-PON*. **Telemetry:** An EPON module raises `XPON_SYSTEM_HAL_WRONG_PON_MODE` (§2.4). | `PhysicalMedia.{i}.PonMode` | string | Read-Only |
| 59 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Connector` | Connector type. | `PhysicalMedia.{i}.Connector` | string | Read-Only |
| 60 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateDownstream` | Nominal data rate downstream in kbps. *Passed through without unit conversion.* | `PhysicalMedia.{i}.NominalBitRateDownstream` | unsignedInt | Read-Only |
| 61 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateUpstream` | Nominal data rate upstream in kbps. *Passed through without unit conversion.* | `PhysicalMedia.{i}.NominalBitRateUpstream` | unsignedInt | Read-Only |
| 62 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.RxPower` | Measured RX power in 0.1 dBm. *Passed through without unit conversion.* | `PhysicalMedia.{i}.RxPower.SignalLevel` | int | Read-Only |
| 63 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.TxPower` | Measured TX power in 0.1 dBm. *Passed through without unit conversion.* | `PhysicalMedia.{i}.TxPower.SignalLevel` | int | Read-Only |
| 64 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Voltage` | Measured supply voltage in mV. *Negative values are rejected.* | `PhysicalMedia.{i}.Voltage.VoltageLevel` | unsignedInt | Read-Only |
| 65 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Bias` | Measured bias current in µA. *Passed through without unit conversion.* | `PhysicalMedia.{i}.Bias.CurrentBias` | unsignedInt | Read-Only |
| 66 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Temperature` | Measured temperature in °C. *Values below −274 are rejected.* | `PhysicalMedia.{i}.Temperature.CurrentTemp` | int | Read-Only |

### 1.2 `Device.X_RDK_ONT` — RDK Extensions

Defined in `config/RdkGponManager.xml` and served over the CCSP message bus by the DML plug-in in `source/TR-181/middle_layer_src`. The model has 95 parameters. Only the 37 with no BBF `Device.XPON` equivalent are listed here. Tables: Table `PhysicalMedia.{i}`, `Gem.{i}`, `Veip.{i}`.

#### 1.2.1 PhysicalMedia — Module & Interface

> **Not in `Device.XPON`:** BBF `ANI.{i}.Transceiver.{i}` has no cage, firmware-version or redundancy fields, and `ANI.{i}` has no `LowerLayers`/`Upstream`.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 1 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Cage` | Physical cage of the optical module: *BoB* (bond-on-board) or *SFP*. | string (enum) | Read-Only |
| 2 | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleFirmwareVersion` | Optical module firmware version. | string(256) | Read-Only |
| 3 | `Device.X_RDK_ONT.PhysicalMedia.{i}.RedundancyState` | Redundancy role of this media: *Active* or *Standby*. | string (enum) | Read-Only |
| 4 | `Device.X_RDK_ONT.PhysicalMedia.{i}.LowerLayers` | Comma-separated list of lower-layer paths. | string(1024) | Read/Write |
| 5 | `Device.X_RDK_ONT.PhysicalMedia.{i}.Upstream` | Whether this interface is upstream-facing. | boolean | Read-Only |

#### 1.2.2 PhysicalMedia — Optical Power Thresholds

> **Not in `Device.XPON`:** BBF `Transceiver` reports `RxPower`/`TxPower` only; it has no lower/upper threshold parameters.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 6 | `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelLowerThreshold` | Lower RX optical power threshold. | int32 | Read/Write |
| 7 | `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelUpperThreshold` | Upper RX optical power threshold. | int32 | Read/Write |
| 8 | `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelLowerThreshold` | Lower TX optical power threshold. | int32 | Read/Write |
| 9 | `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelUpperThreshold` | Upper TX optical power threshold. | int32 | Read/Write |

#### 1.2.3 GTC Counters

> **Not in `Device.XPON`:** No BBF `TC.PM.*` counter for lost downstream frames.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 10 | `Device.X_RDK_ONT.Gtc.FramesLost` | Downstream frames lost. | uint32 | Read-Only |

#### 1.2.4 PLOAM

> **Not in `Device.XPON`:** BBF `TC.ONUActivation` has no activation counter or TO1/TO2 timers.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 11 | `Device.X_RDK_ONT.Ploam.ActivationCounter` | Number of times the ONU has reached the operational state. | uint32 | Read-Only |
| 12 | `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO1` | TO1 (serial-number / ranging) timer value. | uint32 | Read-Only |
| 13 | `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO2` | TO2 (POPUP) timer value. | uint32 | Read-Only |

#### 1.2.5 GEM Port Ethernet Flow (VLAN Classification)

> **Not in `Device.XPON`:** `Device.XPON` doesn't model per-GEM-port VLAN classification.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 14 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.Tagged` | Ingress tagging mode of the flow. | string (enum) | Read-Only |
| 15 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Vid` | Ingress S-VLAN VLAN ID. | uint32 | Read-Only |
| 16 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Pcp` | Ingress S-VLAN priority code point. | uint32 | Read-Only |
| 17 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Dei` | Ingress S-VLAN drop eligible indicator. | uint32 | Read-Only |
| 18 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Vid` | Ingress C-VLAN VLAN ID. | uint32 | Read-Only |
| 19 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Pcp` | Ingress C-VLAN priority code point. | uint32 | Read-Only |
| 20 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Dei` | Ingress C-VLAN drop eligible indicator. | uint32 | Read-Only |
| 21 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.Tagged` | Egress tagging mode of the flow. | string (enum) | Read-Only |
| 22 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Vid` | Egress S-VLAN VLAN ID. | uint32 | Read-Only |
| 23 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Pcp` | Egress S-VLAN priority code point. | uint32 | Read-Only |
| 24 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Dei` | Egress S-VLAN drop eligible indicator. | uint32 | Read-Only |
| 25 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Vid` | Egress C-VLAN VLAN ID. | uint32 | Read-Only |
| 26 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Pcp` | Egress C-VLAN priority code point. | uint32 | Read-Only |
| 27 | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Dei` | Egress C-VLAN drop eligible indicator. | uint32 | Read-Only |

#### 1.2.6 VEIP Ethernet Flow (VLAN Classification)

> **Not in `Device.XPON`:** BBF `EthernetUNI` doesn't model VEIP VLAN tagging.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 28 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Tagged` | Ingress tagging mode of the flow. | string (enum) | Read/Write |
| 29 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Vid` | Ingress Q-VLAN VLAN ID. | uint32 | Read/Write |
| 30 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Pcp` | Ingress Q-VLAN priority code point. | uint32 | Read-Only |
| 31 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Dei` | Ingress Q-VLAN drop eligible indicator. | uint32 | Read-Only |
| 32 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Tagged` | Egress tagging mode of the flow. | string (enum) | Read/Write |
| 33 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Vid` | Egress Q-VLAN VLAN ID. | uint32 | Read/Write |
| 34 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Pcp` | Egress Q-VLAN priority code point. | uint32 | Read-Only |
| 35 | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Dei` | Egress Q-VLAN drop eligible indicator. | uint32 | Read-Only |

#### 1.2.7 TR-069 Management (OMCI)

> **Not in `Device.XPON`:** The OMCI TR-069 management ME has no `Device.XPON` representation.

| # | TR-181 Parameter | Description | Data Type | Access |
|---|---|---|---|---|
| 36 | `Device.X_RDK_ONT.TR69.url` | ACS URL provisioned through OMCI (TR-069 management ME). | string(256) | Read-Only |
| 37 | `Device.X_RDK_ONT.TR69.AssociatedTag` | VLAN tag associated with the TR-069 management path. | uint32 | Read-Only |

## 2. Telemetry Events (T2)

All events are sent through the RDK T2 framework (`libtelemetry_msgsender`) by the telemetry module in `source/telemetry/`, registered with T2 as component `GponManager` (EPON uses `EponManager`). Markers use the `XPON_` prefix. Events with a value use `t2_event_s(marker, value)`; events without one use `t2_event_d(marker, 1)`. Every event is also logged as `[T2] <PRIORITY> <marker> [value]` in the GPON Manager log, even when T2 is compiled out.

> Telemetry is compiled in only with `--enable-telemetry2-0` (`ENABLE_FEATURE_TELEMETRY2_0`). Otherwise the API stays linked and only writes the log line.

> **Event-driven:** Every event is raised from a HAL push event, a state-machine transition or a lifecycle/error path. TR-181 polling (CCSP/RBUS gets, `GponHal_get_data()`) updates the data model but never raises telemetry, except for poll failures (`XPON_ERROR_STATS_COLLECTION_FAILED`, `XPON_SYSTEM_HAL_WRONG_PON_MODE`). A change seen first by a poll doesn't suppress the event: the HAL event callbacks compare against their own event-fed state, not against the polled value.

### 2.1 ONU Activation (PLOAM) Events

Derived from transitions of the ONU activation state (`Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUState`, backed by `Device.X_RDK_ONT.Ploam.RegistrationState`; O1–O9) in the HAL push event callback `eventcb_PloamRegistrationState()`. Polled reads never raise these events. One state change can raise more than one event: `XPON_ONU_STATE_CHANGE` for every change, plus milestone events (see §2.6).

| Event Name | Meaning | Priority | Value | Estimated Frequency |
|---|---|---|---|---|
| `XPON_ONU_LOS` | Downstream signal lost: the ONU entered *O6* (Intermittent LODS), or fell back to *O1* (Initial) from *O2*–*O5*. | Critical | — | On fibre cut, OLT/splitter failure or optical power loss. Rare in stable networks (0–2/day); bursts during outages. |
| `XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED` | Downstream signal acquired: transition out of *O1* into any state other than O1. | Info | — | Startup and after each LOS recovery. 1–2 per boot. |
| `XPON_ONU_REGISTRATION` | ONU reached *O5* (Operation) — ranging done, ONU-ID assigned, ONU in service. | Info | — | Once per boot; repeats after each recovery (1–3/day). |
| `XPON_ONU_DEREGISTRATION` | ONU left *O5* (Operation) for any other state. | Warning | — | OLT deactivation, LOS, or ranging loss. 0–1/day normally. |
| `XPON_ONU_EMERGENCY_STOP` | ONU entered *O7* (Emergency Stop) — OLT ordered the transmitter off (Disable_Serial_Number PLOAM). GPON-specific; no EPON equivalent. | Critical | — | Only on OLT action against this ONU (rogue / operator disable). Very rare. |
| `XPON_ONU_EMERGENCY_STOP_CLEARED` | ONU left *O7* (Emergency Stop) — the OLT re-enabled the transmitter (Disable_Serial_Number with *enable*). GPON-specific; no EPON equivalent. | Info | — | Paired with each EMERGENCY_STOP. Very rare. |
| `XPON_ONU_STATE_CHANGE` | Every ONU activation state change, including the ones the milestone events above don't cover (moves among *O2*/*O3*/*O4*, *O8*/*O9* tuning, returns from *O6*–*O9* to *O1*). Always sent first for a transition. The `From`/`To` pair identifies the transition (ranging failure, TO1/TO2 expiry, POPUP recovery, OLT disable, tuning), so no per-transition markers are defined; see §2.6.2 for the meaning and possible reasons of each pair. GPON-specific; no EPON equivalent. | Info | `From=O<a>,To=O<b>` | Every PLOAM state change. About 4–5 per activation (O1→O2→O3→O4→O5), more while ranging is unstable. |

### 2.2 Interface Link Events

`XPON_INTF_LINK_*` / `XPON_PHY_STATUS_*` are raised by the GPON link state machine (`gponmgr_link_state_machine.c`) when a VEIP link (`Device.XPON.ONU.{i}.EthernetUNI.{i}`) completes a *Down→Up* or *Up→Down* transition and WAN Manager has been told. `XPON_INTF_ADMIN_*` are edge-detected on the OMCI VEIP administrative state (`Device.XPON.ONU.{i}.EthernetUNI.{i}.Enable`, backed by `Device.X_RDK_ONT.Veip.{i}.AdministrativeState`) in the HAL push event callback `eventcb_VeipAdministrativeState()`. Polled reads never raise these events.

| Event Name | Meaning | Priority | Value | Estimated Frequency |
|---|---|---|---|---|
| `XPON_INTF_LINK_UP` | VEIP interface is up and has been reported to WAN Manager. | Info | `Interface=<ifname>` | Once per link-up. Typically 1/boot plus 1 per recovery. |
| `XPON_INTF_LINK_DOWN` | VEIP interface went down and WAN Manager was told. | Warning | `Interface=<ifname>` | Once per link-down. 0–2/day normally. |
| `XPON_PHY_STATUS_UP` | Aggregate PHY state went up: the first VEIP link of the ONU came up (0→1 links up). | Info | — | Once per boot plus 1 per full recovery. |
| `XPON_PHY_STATUS_DOWN` | Aggregate PHY state went down: the last VEIP link went down (1→0 links up). The ONU has no WAN connectivity. | Critical | — | Only on full loss of WAN connectivity. 0–2/day. |
| `XPON_INTF_ADMIN_UNLOCK` | The OLT unlocked the VEIP over OMCI (AdministrativeState *Lock→Unlock*). An Unlock also starts the VEIP link state machine. The default is *Lock*, so the first Unlock after start raises this event. GPON-specific; no EPON equivalent. | Info | `Interface=<ifname>,Veip=<n>` | Once per boot per VEIP after OMCI provisioning, plus 1 per re-provisioning. |
| `XPON_INTF_ADMIN_LOCK` | The OLT locked the VEIP over OMCI (AdministrativeState *Unlock→Lock*): the UNI is administratively disabled and won't pass traffic. GPON-specific; no EPON equivalent. | Warning | `Interface=<ifname>,Veip=<n>` | Only on OLT/operator action. Rare. |

### 2.3 PhysicalMedia Alarms (G.984.3 / G.987.3)

Edge-detected on each alarm field (`Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.*`, backed by `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*`) in the HAL push event callback `eventcb_PhysicalMediaAlarmsAll()`. Polled reads never raise these events. *Inactive→Active* sends `RAISED`; *Active→Inactive* sends `CLEARED`. Unchanged values send nothing. One field per marker, so one alarm can't mask another.

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
| `XPON_SYSTEM_HAL_WRONG_PON_MODE` | Optical module reports an EPON `PonMode` that GPON Manager can't serve. `PonMode` has no HAL event, so this is a poll failure (the polled value is rejected). Latched: sent once until a valid ITU PON mode is reported. | Critical | — | Only on mis-provisioned hardware/HAL. |

### 2.5 Error Events

Operational failures in the HAL, RBUS and state-machine paths. Repeating paths are latched so a persistent failure gives one event per failure streak.

| Event Name | Meaning | Priority | Value | Estimated Frequency |
|---|---|---|---|---|
| `XPON_ERROR_HAL_CALL_FAILED` | JSON-HAL client init/connect failed, a HAL `setParam` request couldn't be sent, or the HAL returned failure. | Error | — | On HAL/daemon unavailability or rejected set. |
| `XPON_ERROR_HAL_EVENT_SUBSCRIBE_FAILED` | Subscribing to HAL push events failed. | Error | — | Startup only. |
| `XPON_ERROR_HAL_EVENT_PARSE_FAILED` | A HAL push event couldn't be parsed or mapped into the data model. | Warning | — | Per malformed HAL event. |
| `XPON_ERROR_STATS_COLLECTION_FAILED` | Poll failure: bulk HAL data read (`GponHal_get_data`) failed. Latched; re-armed by the next success. | Warning | — | Once per failure streak. |
| `XPON_ERROR_RBUS_PUBLISH_FAILED` | Telling WAN Manager about a VEIP PHY status change failed. Latched per state machine; re-armed by the next success. | Error | — | Once per failure streak. |
| `XPON_ERROR_RBUS_REGISTER_FAILED` | Registering the `Device.XPON` RBUS provider failed. Non-fatal: `Device.X_RDK_ONT` stays available. | Error | — | Startup only. |
| `XPON_ERROR_STATE_MACHINE_START_FAILED` | The VEIP link state machine couldn't be started (allocation or thread creation failed). | Critical | — | Only under resource exhaustion. |

### 2.6 ONU State → Event Mapping

`XPON_ONU_STATE_CHANGE` carries the previous and the new state (`From=O<a>,To=O<b>`), so every transition, including ranging failures, TO1/TO2 expiry, POPUP recovery and NG-PON2 tuning, is identified from that pair alone. No extra per-transition markers are defined. The milestone events (`XPON_ONU_LOS`, `_DOWNSTREAM_SIGNAL_DETECTED`, `_REGISTRATION`, `_DEREGISTRATION`, `_EMERGENCY_STOP`, `_EMERGENCY_STOP_CLEARED`) are kept for EPON parity and simple dashboards; they never carry information that isn't already in the pair.

Rules applied by `map_onu_state()`: every change raises `XPON_ONU_STATE_CHANGE` first; milestone events follow in this order: EMERGENCY_STOP_CLEARED, DOWNSTREAM_SIGNAL_DETECTED, LOS, DEREGISTRATION, REGISTRATION, EMERGENCY_STOP. Unchanged state raises nothing.

#### 2.6.1 ONU Activation States

| State | G.984.3 (GPON) | G.9807.1 (XGS-PON) / G.989.3 (NG-PON2) | Meaning |
|---|---|---|---|
| *O1* | Initial | Initial (O1.1 Off-sync / O1.2 Profile learning) | No downstream sync (LOS/LOF). Default state after power-up and after any unrecovered downstream loss. |
| *O2* | Standby | Serial Number (O2-3) | Downstream synchronised. Waiting for the network parameters (Upstream_Overhead in G.984.3, Burst_Profile in G.989.3/G.9807.1). |
| *O3* | Serial Number | Serial Number (O2-3) | Answering the OLT's serial-number requests. Waiting for Assign_ONU-ID. |
| *O4* | Ranging | Ranging | ONU-ID assigned. Waiting for the OLT to complete ranging (Ranging_Time with the equalisation delay). |
| *O5* | Operation | Operation (O5.1 Associated / O5.2 Pending) | In service: ONU-ID and equalisation delay set, PLOAM and OMCI channels up. |
| *O6* | POPUP | Intermittent LODS | Downstream lost while in O5. Timer TO2 runs; the ONU keeps its ONU-ID and equalisation delay until TO2 expires. |
| *O7* | Emergency Stop | Emergency Stop | OLT sent Disable_Serial_Number (disable). Transmitter is off until the OLT re-enables the serial number. |
| *O8* | — | Downstream Tuning (G.989.3 only) | NG-PON2 TWDM: tuning the receiver to a new wavelength channel. Never entered on GPON / XG-PON / XGS-PON. |
| *O9* | — | Upstream Tuning (G.989.3 only) | NG-PON2 TWDM: tuning the transmitter to a new wavelength channel. Never entered on GPON / XG-PON / XGS-PON. |

> **Numbering:** The HAL reports O1–O9 with the G.984.3 numbering for O1–O7 and the G.989.3 numbering for O8/O9 (O5 is always Operation). O2 and O3 are reported separately; the G.989.3 / G.9807.1 combined *O2-3* state is reported as O2 or O3. G.987.3 (XG-PON) uses its own O1–O6 numbering natively; the HAL is expected to map it onto the states above.

#### 2.6.2 Transition Reference

All 72 transitions. *Milestones* lists the `XPON_ONU_*` events sent after `XPON_ONU_STATE_CHANGE`. *Category*: **Normal** activation step, **Skipped states** (normal outcome, intermediate states not reported), **Activation failure**, **Downstream loss**, **Re-ranging**, **Recovery**, **OLT action**, **Tuning** / **Tuning failure** (NG-PON2 only), **Abnormal** (not a direct ITU-T transition; investigate).

> **Skipped states:** The HAL pushes `Ploam.RegistrationState` on change and only the latest value. When several transitions happen between two pushes (fast activation, short outages), intermediate states are not reported, so the pair can be one that ITU-T doesn't define directly (for example *O1 → O5* or *O4 → O6*). The table lists the most likely path for those pairs. For the reason behind a transition, correlate with the `XPON_ALARM_*` events (§2.3) raised around the same time: LOS/LOF (downstream loss), SUF (start-up failure), DACT (Deactivate_ONU-ID), DIS (Disable_Serial_Number), SD/SF (marginal optical budget).

| From → To | Category | Meaning | Possible reasons | Milestones |
|---|---|---|---|---|
| O1 → O2 | Normal | Downstream synchronisation acquired; activation starts. | Power-up / reboot, fibre reconnected, OLT port enabled, recovery after a downstream loss. | DOWNSTREAM_SIGNAL_DETECTED |
| O1 → O3 | Normal | Downstream sync acquired and network parameters received (O2 not reported). | The HAL publishes only the latest state, so intermediate states were not reported (fast activation), or the HAL reports the G.989.3 combined O2-3 state as O3. | DOWNSTREAM_SIGNAL_DETECTED |
| O1 → O4 | Skipped states | Sync, serial-number acquisition and ONU-ID assignment done between two reports. | The HAL publishes only the latest state, so intermediate states were not reported. Normal outcome, fast activation. | DOWNSTREAM_SIGNAL_DETECTED |
| O1 → O5 | Skipped states | Full activation completed between two reports; ONU in service. | The HAL publishes only the latest state, so intermediate states were not reported. Normal outcome. | DOWNSTREAM_SIGNAL_DETECTED, REGISTRATION |
| O1 → O6 | Abnormal | Not a direct ITU-T transition (O6 is entered only from O5). The ONU activated and lost downstream again before O5 was reported. | Unstable downstream: Rx power near sensitivity, loose connector, OLT port flapping. Check `XPON_ALARM_LOS/LOF/SD/SF`. | DOWNSTREAM_SIGNAL_DETECTED, LOS |
| O1 → O7 | OLT action | ONU disabled by the OLT right after it announced its serial number (O2–O4 not reported), or the ONU restored a persisted Emergency Stop at boot. | Disable_Serial_Number (disable) from the OLT: rogue-ONU isolation, operator disable or subscriber suspension. Serial number blocked or not authorised on the OLT. Check `XPON_ALARM_DIS`. | DOWNSTREAM_SIGNAL_DETECTED, EMERGENCY_STOP |
| O1 → O8 | Abnormal | Not a direct ITU-T transition (O8 is entered from O5/O6). | The HAL publishes only the latest state, so intermediate states were not reported, or vendor HAL reports channel scanning as O8 (NG-PON2 only). | DOWNSTREAM_SIGNAL_DETECTED |
| O1 → O9 | Abnormal | Not a direct ITU-T transition (O9 is entered from O8). | The HAL publishes only the latest state, so intermediate states were not reported, or vendor-specific state reporting (NG-PON2 only). | DOWNSTREAM_SIGNAL_DETECTED |
| O2 → O1 | Downstream loss | Downstream lost before the network parameters were received. | Fibre cut / unplugged / dirty connector, Rx power below sensitivity, OLT port or PON card down, splitter fault. | LOS |
| O2 → O3 | Normal | Network parameters received; ONU now answers serial-number requests. | Normal activation step. | — |
| O2 → O4 | Normal | ONU-ID assigned quickly; O3 not reported. | The HAL publishes only the latest state, so intermediate states were not reported. Normal activation. | — |
| O2 → O5 | Skipped states | Activation completed between two reports; ONU in service. | The HAL publishes only the latest state, so intermediate states were not reported. Normal outcome. | REGISTRATION |
| O2 → O6 | Abnormal | Not a direct ITU-T transition. O5 was reached and downstream was lost before O5 was reported. | Unstable downstream right after activation: Rx power near sensitivity, connector fault. | LOS |
| O2 → O7 | OLT action | OLT disabled this serial number during activation. | Disable_Serial_Number (disable) from the OLT: rogue-ONU isolation, operator disable or subscriber suspension. | EMERGENCY_STOP |
| O2 → O8 | Abnormal | Not a direct ITU-T transition. | The HAL publishes only the latest state, so intermediate states were not reported, or vendor-specific state reporting (NG-PON2 only). | — |
| O2 → O9 | Abnormal | Not a direct ITU-T transition. | The HAL publishes only the latest state, so intermediate states were not reported, or vendor-specific state reporting (NG-PON2 only). | — |
| O3 → O1 | Downstream loss | Downstream lost during serial-number acquisition. | Fibre cut / unplugged / dirty connector, Rx power below sensitivity, OLT port or PON card down, splitter fault. | LOS |
| O3 → O2 | Activation failure | Serial-number phase abandoned: TO1 expired (typically 10 s) before an ONU-ID was assigned. | OLT doesn't accept the serial number: ONU not provisioned on the OLT, wrong SN / password / registration ID, ONU on the wrong PON port. Or the OLT doesn't receive the SN bursts: upstream Tx power too low, high ODN loss, ONU outside the OLT ranging window. Repeated O2↔O3 cycling is the typical signature of an ONU that isn't provisioned on the OLT. | — |
| O3 → O4 | Normal | Assign_ONU-ID received; ranging starts. | Normal activation step. | — |
| O3 → O5 | Normal | Ranging completed quickly; O4 not reported. ONU in service. | The HAL publishes only the latest state, so intermediate states were not reported. Normal activation. | REGISTRATION |
| O3 → O6 | Abnormal | Not a direct ITU-T transition. O5 was reached and downstream was lost before O5 was reported. | Unstable downstream right after activation. | LOS |
| O3 → O7 | OLT action | OLT disabled this serial number during serial-number acquisition. | Disable_Serial_Number (disable) from the OLT: rogue-ONU isolation, operator disable or subscriber suspension. Often an unauthorised / blacklisted serial number. | EMERGENCY_STOP |
| O3 → O8 | Abnormal | Not a direct ITU-T transition. | The HAL publishes only the latest state, so intermediate states were not reported, or vendor-specific state reporting (NG-PON2 only). | — |
| O3 → O9 | Abnormal | Not a direct ITU-T transition. | The HAL publishes only the latest state, so intermediate states were not reported, or vendor-specific state reporting (NG-PON2 only). | — |
| O4 → O1 | Downstream loss | Downstream lost during ranging. | Fibre cut / unplugged / dirty connector, Rx power below sensitivity, OLT port or PON card down, splitter fault. | LOS |
| O4 → O2 | Activation failure | Ranging failed: TO1 expired or Deactivate_ONU-ID received. ONU-ID discarded. | OLT can't range the ONU: upstream bursts not received (low Tx power, high ODN loss), ONU beyond the OLT's distance / differential-reach window, upstream collision with a rogue ONU. Or the OLT rejected the ONU after ONU-ID assignment (password / registration-ID / LOID authentication failure). Check `XPON_ALARM_SUF` and `XPON_ALARM_DACT`. | — |
| O4 → O3 | Activation failure | Same as O4 → O2 (O2 not reported, or the HAL reports the G.989.3 O2-3 state as O3). | See O4 → O2. | — |
| O4 → O5 | Normal | Ranging_Time received; ONU in service. | Normal activation completed. | REGISTRATION |
| O4 → O6 | Abnormal | Not a direct ITU-T transition. O5 was reached and downstream was lost before O5 was reported. | Unstable downstream right after activation. | LOS |
| O4 → O7 | OLT action | OLT disabled this serial number during ranging. | Disable_Serial_Number (disable) from the OLT: rogue-ONU isolation, operator disable or subscriber suspension. Can follow rogue-ONU detection during ranging. | EMERGENCY_STOP |
| O4 → O8 | Abnormal | Not a direct ITU-T transition. | The HAL publishes only the latest state, so intermediate states were not reported, or vendor-specific state reporting (NG-PON2 only). | — |
| O4 → O9 | Abnormal | Not a direct ITU-T transition. | The HAL publishes only the latest state, so intermediate states were not reported, or vendor-specific state reporting (NG-PON2 only). | — |
| O5 → O1 | Downstream loss | Service lost and not recovered: downstream loss longer than TO2 (O6 not reported), or the PON stack was re-initialised. | Fibre cut / unplugged / dirty connector, Rx power below sensitivity, OLT port or PON card down, splitter fault, OLT reboot, vendor PON software reset. | LOS, DEREGISTRATION |
| O5 → O2 | OLT action | ONU deregistered by the OLT (Deactivate_ONU-ID); activation restarts. | Operator deleted / re-provisioned the ONU, authentication failure (password / registration ID / LOID), OLT detected excessive errors or drift of window, OLT PON card reset. Check `XPON_ALARM_DACT`. | DEREGISTRATION |
| O5 → O3 | OLT action | Same as O5 → O2 (O2 not reported, or G.989.3 O2-3 reported as O3). | See O5 → O2. | DEREGISTRATION |
| O5 → O4 | Re-ranging | ONU re-ranged without a full re-activation (O6 not reported): after a short downstream loss the OLT sent a broadcast POPUP (G.984.3). | PON-wide downstream interruption: OLT port / laser glitch, Type-B protection switch-over, OLT software restart. | DEREGISTRATION |
| O5 → O6 | Downstream loss | Downstream lost while in service; TO2 started (intermittent LODS / POPUP). | Fibre cut / unplugged / dirty connector, Rx power below sensitivity, OLT port or PON card down, splitter fault, OLT protection switch-over. Followed by O6 → O5 (glitch recovered) or O6 → O1 (outage). | LOS, DEREGISTRATION |
| O5 → O7 | OLT action | Emergency stop while in service; transmitter off. | Disable_Serial_Number (disable) from the OLT: rogue-ONU isolation, operator disable or subscriber suspension. | DEREGISTRATION, EMERGENCY_STOP |
| O5 → O8 | Tuning | OLT moved the ONU to another wavelength channel (Tuning_Control). | NG-PON2 load balancing, channel protection or maintenance. | DEREGISTRATION |
| O5 → O9 | Tuning | Upstream-only channel change (O8 not reported). | NG-PON2 load balancing, channel protection or maintenance. | DEREGISTRATION |
| O6 → O1 | Downstream loss | TO2 expired: the downstream outage outlasted TO2. ONU-ID and equalisation delay discarded; full re-activation needed. | Sustained outage: fibre cut, unplugged or dirty connector, Rx power below sensitivity, OLT port or PON card down, splitter fault, OLT reboot. | — |
| O6 → O2 | Downstream loss | TO2 expired and downstream was re-acquired before O1 was reported. | The HAL publishes only the latest state, so intermediate states were not reported. Same causes as O6 → O1. | — |
| O6 → O3 | Downstream loss | TO2 expired and re-activation reached O3 before O1/O2 were reported. | The HAL publishes only the latest state, so intermediate states were not reported. Same causes as O6 → O1. | — |
| O6 → O4 | Re-ranging | Downstream recovered and the OLT sent a broadcast POPUP: ONU re-ranges (G.984.3). | PON-wide downstream interruption (OLT port / laser glitch, protection switch-over) affecting all ONUs on the port. | — |
| O6 → O5 | Recovery | Downstream recovered before TO2 expired (directed POPUP in G.984.3, re-sync in G.989.3). ONU back in service with the same ONU-ID. | Short downstream glitch: connector disturbance, momentary optical power dip, OLT port glitch. Frequent O5 → O6 → O5 points to a marginal optical budget; check `XPON_ALARM_SD/SF`. | REGISTRATION |
| O6 → O7 | OLT action | OLT disabled the serial number while downstream was interrupted. | Disable_Serial_Number (disable) from the OLT: rogue-ONU isolation, operator disable or subscriber suspension. | EMERGENCY_STOP |
| O6 → O8 | Tuning | ONU tunes to the configured protection channel after the downstream loss. | NG-PON2 TWDM channel protection. | — |
| O6 → O9 | Tuning | Protection-channel tuning, O8 not reported. | NG-PON2 TWDM channel protection. | — |
| O7 → O1 | Recovery | OLT re-enabled the serial number (Disable_Serial_Number enable); activation restarts (G.987.3 / G.9807.1 / G.989.3). | Operator re-enabled the ONU, rogue condition cleared, subscriber reinstated. Also a reboot on implementations that don't persist O7. | EMERGENCY_STOP_CLEARED |
| O7 → O2 | Recovery | OLT re-enabled the serial number; ONU returns to Standby (G.984.3). | Same as O7 → O1. | EMERGENCY_STOP_CLEARED |
| O7 → O3 | Recovery | Re-enabled; re-activation reached O3 before O1/O2 were reported. | Same as O7 → O1. The HAL publishes only the latest state, so intermediate states were not reported. | EMERGENCY_STOP_CLEARED |
| O7 → O4 | Recovery | Re-enabled; re-activation reached O4 before earlier states were reported. | Same as O7 → O1. The HAL publishes only the latest state, so intermediate states were not reported. | EMERGENCY_STOP_CLEARED |
| O7 → O5 | Recovery | Re-enabled and fully re-activated between two reports; ONU in service. | Same as O7 → O1. The HAL publishes only the latest state, so intermediate states were not reported. | EMERGENCY_STOP_CLEARED, REGISTRATION |
| O7 → O6 | Abnormal | Not a direct ITU-T transition. Re-enabled, re-activated and lost downstream again before O5 was reported. | Re-enable followed by an unstable downstream. | EMERGENCY_STOP_CLEARED, LOS |
| O7 → O8 | Abnormal | Not a direct ITU-T transition. | The HAL publishes only the latest state, so intermediate states were not reported, or vendor-specific state reporting (NG-PON2 only). | EMERGENCY_STOP_CLEARED |
| O7 → O9 | Abnormal | Not a direct ITU-T transition. | The HAL publishes only the latest state, so intermediate states were not reported, or vendor-specific state reporting (NG-PON2 only). | EMERGENCY_STOP_CLEARED |
| O8 → O1 | Tuning failure | Downstream tuning failed: no sync on the target channel before the tuning timer expired. | Target channel not lit / not available, tunable receiver fault, wrong channel profile. | — |
| O8 → O2 | Tuning failure | Tuning failed and re-activation started (O1 not reported). | See O8 → O1. The HAL publishes only the latest state, so intermediate states were not reported. | — |
| O8 → O3 | Tuning failure | Tuning failed and re-activation started (O1/O2 not reported). | See O8 → O1. The HAL publishes only the latest state, so intermediate states were not reported. | — |
| O8 → O4 | Tuning | Re-ranging on the new channel. | Vendor-specific: the OLT ranges the ONU again on the target channel. | — |
| O8 → O5 | Tuning | Tuning complete; ONU in service on the new channel (no upstream tuning needed, or O9 not reported). | Normal end of a channel change. | REGISTRATION |
| O8 → O6 | Abnormal | Not a direct ITU-T transition. Tuning completed and downstream was lost before O5 was reported. | Unstable downstream on the new channel. | LOS |
| O8 → O7 | OLT action | OLT disabled the serial number during tuning. | Disable_Serial_Number (disable) from the OLT: rogue-ONU isolation, operator disable or subscriber suspension. | EMERGENCY_STOP |
| O8 → O9 | Tuning | Downstream locked on the target channel; upstream tuning starts. | Normal channel-change step. | — |
| O9 → O1 | Tuning failure | Upstream tuning failed (tuning timer expired) or downstream lost; full re-activation. | OLT doesn't see the ONU on the target upstream channel: tunable laser fault, wrong channel profile, upstream power too low. | — |
| O9 → O2 | Tuning failure | Upstream tuning failed and re-activation started (O1 not reported). | See O9 → O1. The HAL publishes only the latest state, so intermediate states were not reported. | — |
| O9 → O3 | Tuning failure | Upstream tuning failed and re-activation started (O1/O2 not reported). | See O9 → O1. The HAL publishes only the latest state, so intermediate states were not reported. | — |
| O9 → O4 | Tuning | Re-ranging on the new upstream channel. | Vendor-specific: the OLT ranges the ONU again on the target channel. | — |
| O9 → O5 | Tuning | Tuning complete; ONU in service on the new channel. | Normal end of a channel change. | REGISTRATION |
| O9 → O6 | Abnormal | Not a direct ITU-T transition. Tuning completed and downstream was lost before O5 was reported. | Unstable downstream on the new channel. | LOS |
| O9 → O7 | OLT action | OLT disabled the serial number during tuning. | Disable_Serial_Number (disable) from the OLT: rogue-ONU isolation, operator disable or subscriber suspension. | EMERGENCY_STOP |
| O9 → O8 | Tuning | Upstream tuning rolled back to downstream tuning (retry or channel change aborted). | NG-PON2 tuning rollback / retry. | — |

#### 2.6.3 Common Sequences

Read the `XPON_ONU_STATE_CHANGE` values in order (T2 `"use": "accumulate"`) to recognise these patterns:

| Sequence | Interpretation |
|---|---|
| O1→O2→O3→O4→O5 | Normal activation. Time from leaving O1 to reaching O5 is the activation time. |
| O2→O3→O2→O3… (never O4) | Serial number not accepted: ONU not provisioned on the OLT, wrong SN / password / registration ID, or SN bursts not received (upstream power, distance). |
| …→O4→O2 repeating | Ranging fails: upstream optical budget, distance outside the ranging window, rogue ONU on the PON. Usually with `XPON_ALARM_SUF`. |
| O4→O5→O2 shortly after | Authentication rejected after activation (password / registration ID / LOID). Usually with `XPON_ALARM_DACT`. |
| O5→O6→O5 | Short downstream glitch, recovered within TO2. Frequent occurrences point to a marginal optical budget. |
| O5→O6→O1→…→O5 | Downstream outage longer than TO2, followed by full re-activation. Correlate with `XPON_ALARM_LOS/LOF`. |
| O5→O6→O4→O5 | PON-wide interruption; the OLT re-ranged all ONUs with a broadcast POPUP (G.984.3). |
| O5→O2 / O3 | OLT deregistered the ONU (Deactivate_ONU-ID). Correlate with `XPON_ALARM_DACT`. |
| O*x*→O7 … O7→O1 / O2 | OLT disabled and later re-enabled the serial number. Correlate with `XPON_ALARM_DIS`. |
| O5→O8→O9→O5 | NG-PON2 wavelength-channel change completed. |

### 2.7 Value Formats & Duplicate Suppression

| Format | Value | Used by | T2 call |
|---|---|---|---|
| None | — | ONU milestone, PHY status, lifecycle, error events | `t2_event_d(marker, 1)` |
| ONU state | `From=O<a>,To=O<b>` | `XPON_ONU_STATE_CHANGE` | `t2_event_s(marker, value)` |
| Interface | `Interface=<ifname>` (`unknown` if empty) | `XPON_INTF_LINK_UP/DOWN` | `t2_event_s(marker, value)` |
| VEIP | `Interface=<ifname>,Veip=<n>` (`unknown` if empty; n = Veip instance) | `XPON_INTF_ADMIN_LOCK/UNLOCK` | `t2_event_s(marker, value)` |
| Alarm | `RAISED,PM=<n>` / `CLEARED,PM=<n>` (n = PhysicalMedia instance) | `XPON_ALARM_*` | `t2_event_s(marker, value)` |

| Mechanism | Applies to | Behaviour |
|---|---|---|
| Edge detection (event-only) | ONU state, alarms, VEIP admin state | Only in the HAL push event callbacks. The pushed value is compared with the last *event* value (kept per PLOAM / PhysicalMedia / VEIP instance under the DML data lock); nothing sent when unchanged. Polled reads update the data model only, so they neither raise an event nor hide one. Each instance is seeded on its first event from the value held just before it. Defaults (*O1*, all alarms *Inactive*, VEIP *Lock*) mean no false fault events; the first OMCI *Unlock* is reported, like REGISTRATION at boot. |
| Transition detection | INTF_LINK_*, PHY_STATUS_* | Only on successful state-machine transitions. PHY_STATUS uses a mutex-protected count of links up (0↔1 edges). |
| Latch (one per streak) | STATS_COLLECTION_FAILED, RBUS_PUBLISH_FAILED, HAL_WRONG_PON_MODE | Sent on the first failure only and re-armed by the next success, so retry and polling loops (500 ms state machine, init busy-loop, TR-181 gets) can't flood T2. |
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
| ONU Activation (PLOAM) Events | 7 | 2 | 0 | 1 | 4 |
| Interface Link Events | 6 | 1 | 0 | 2 | 3 |
| PhysicalMedia Alarms (G.984.3 / G.987.3) | 14 | 5 | 6 | 3 | 0 |
| System Lifecycle Events | 4 | 2 | 0 | 0 | 2 |
| Error Events | 7 | 1 | 4 | 2 | 0 |
| **Total** | **38** | **11** | **10** | **8** | **9** |

> **Note:** Each alarm event carries a raised/cleared state in its value. The listed priority is for the raised condition; cleared events are informational.

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
| 9 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.LastChange` | The accumulated time in seconds since the interface entered its current operational state. | unsignedInt | Read-Only | Get rejected |
| 10 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.LowerLayers` | Comma-separated list of references to an interface object that is stacked immediately below this interface object. | `list<string>` | Read/Write | Get/Set rejected |
| 11 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Upstream` | Indicates whether the interface points towards the Internet or towards End Devices. | boolean | Read-Only | Get rejected |
| 12 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.Reset` | Set this to true to reset all this object's statistics counters to zero. | boolean | Read/Write | Get/Set rejected |
| 13 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesSent` | The total number of bytes transmitted out of the interface, including framing characters. | StatsCounter64 | Read-Only | Get rejected |
| 14 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesReceived` | The total number of bytes received on the interface, including framing characters. | StatsCounter64 | Read-Only | Get rejected |
| 15 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsSent` | The total number of packets transmitted out of the interface. | StatsCounter64 | Read-Only | Get rejected |
| 16 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsReceived` | The total number of packets received on the interface. | StatsCounter64 | Read-Only | Get rejected |
| 17 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsSent` | The total number of outbound packets that could not be transmitted because of errors. | StatsCounter32 | Read-Only | Get rejected |
| 18 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsReceived` | The total number of inbound packets that contained errors preventing them from being delivered to a higher-layer protocol. | StatsCounter32 | Read-Only | Get rejected |
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
| 52 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.HexadecimalPassword` | If false, `Password` is in ASCII format. | boolean | Read/Write | Get/Set rejected |
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

## Appendix B. `Device.XPON` ↔ `Device.X_RDK_ONT` Mapping

Maps each supported `Device.XPON` parameter (§1.1) to the `Device.X_RDK_ONT` parameter that backs it. Both read the same `GPON_DML_DATA` field, so the values are always consistent. Because `Device.XPON` takes precedence, the 58 `X_RDK_ONT` parameters in B.1 are not repeated in §1.2. 8 `Device.XPON` parameters have no single `X_RDK_ONT` source (constants, table counts, row index).

|  | Parameters |
|---|---|
| `Device.X_RDK_ONT` total | 95 |
|   mapped to a supported `Device.XPON` parameter (B.1) | 58 |
|   RDK extensions, no BBF equivalent (§1.2) | 37 |
| `Device.XPON` supported (§1.1) | 66 |
| `Device.XPON` unsupported (Appendix A) | 69 |
|   of which X_RDK_ONT data exists but isn't wired (B.2) | 3 |

### B.1 Supported `Device.XPON` → `Device.X_RDK_ONT`

| # | `Device.XPON` Parameter | `Device.X_RDK_ONT` Equivalent | Conversion / Notes |
|---|---|---|---|
| 1 | `Device.XPON.ONUNumberOfEntries` | *constant* | Always 1. |
| 2 | `Device.XPON.ONU.{i}.SoftwareImageNumberOfEntries` | *constant* | Always 0 — no software-image rows are registered. |
| 3 | `Device.XPON.ONU.{i}.EthernetUNINumberOfEntries` | *Veip table* | Number of populated `Veip.{i}` rows. |
| 4 | `Device.XPON.ONU.{i}.ANINumberOfEntries` | *constant* | Always 1. |
| 5 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Enable` | `Device.X_RDK_ONT.Veip.{i}.AdministrativeState` | *Unlock* → true, *Lock* → false. |
| 6 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Status` | `Device.X_RDK_ONT.Veip.{i}.OperationalState` | Same enumeration. |
| 7 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Name` | `Device.X_RDK_ONT.Veip.{i}.InterfaceName` | Get fails if longer than 64 characters. |
| 8 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.ANIs` | *constant* | Always `Device.XPON.ONU.1.ANI.1`. |
| 9 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainID` | `Device.X_RDK_ONT.Veip.{i}.MeId` | Formatted as `(VEIP,<MeId>)`. |
| 10 | `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainName` | `Device.X_RDK_ONT.Veip.{i}.InterDomainName` | Direct copy. |
| 11 | `Device.XPON.ONU.{i}.ANI.{i}.Enable` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Enable` | Direct copy. |
| 12 | `Device.XPON.ONU.{i}.ANI.{i}.Status` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Status` | Uses the uniquely active PhysicalMedia row. |
| 13 | `Device.XPON.ONU.{i}.ANI.{i}.Alias` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alias` | Direct copy. |
| 14 | `Device.XPON.ONU.{i}.ANI.{i}.LastChange` | `Device.X_RDK_ONT.PhysicalMedia.{i}.LastChange` | Direct copy. |
| 15 | `Device.XPON.ONU.{i}.ANI.{i}.TransceiverNumberOfEntries` | *PhysicalMedia table* | Number of populated PhysicalMedia rows. |
| 16 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUState` | `Device.X_RDK_ONT.Ploam.RegistrationState` | O1–O9 (the BBF combined *O2-3* value is never reported). |
| 17 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.VendorID` | `Device.X_RDK_ONT.Ploam.VendorId` | Get fails if longer than 4 characters. |
| 18 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.SerialNumber` | `Device.X_RDK_ONT.Ploam.SerialNumber` | Get fails if not a valid 12-character (or 16 hex digit) serial. |
| 19 | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUID` | `Device.X_RDK_ONT.Ploam.OnuId` | Direct copy. |
| 20 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECBytes` | `Device.X_RDK_ONT.Gtc.CorrectedFecBytes` | Direct copy. |
| 21 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECCodewords` | `Device.X_RDK_ONT.Gtc.CorrectedFecCodeWords` | Direct copy. |
| 22 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UncorrectableFECCodewords` | `Device.X_RDK_ONT.Gtc.UnCorrectedFecCodeWords` | Direct copy. |
| 23 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.TotalFECCodewords` | `Device.X_RDK_ONT.Gtc.TotalFecCodeWords` | Direct copy. |
| 24 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.PSBdHECErrorCount` | `Device.X_RDK_ONT.Gtc.PSBdHecErrors` | Direct copy. |
| 25 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.HeaderHECErrorCount` | `Device.X_RDK_ONT.Gtc.HecErrorCount` | Mapping is a best match (see `XPON-TR181-MAPPING.md`). |
| 26 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FrameHeaderHECErrors` | `Device.X_RDK_ONT.Gtc.FrameHecErrors` | Mapping is a best match (see `XPON-TR181-MAPPING.md`). |
| 27 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.MICErrors` | `Device.X_RDK_ONT.Ploam.MicErrors` | Direct copy. |
| 28 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.DownstreamMessageCount` | `Device.X_RDK_ONT.Ploam.RxMessageCount` | Direct copy. |
| 29 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.UpstreamMessageCount` | `Device.X_RDK_ONT.Ploam.TxMessageCount` | Direct copy. |
| 30 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.BaselineMessagesReceived` | `Device.X_RDK_ONT.Omci.RxBaseLineMessageCountValid` | Direct copy. |
| 31 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.ExtendedMessagesReceived` | `Device.X_RDK_ONT.Omci.RxExtendedMessageCountValid` | Direct copy. |
| 32 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.MICErrors` | `Device.X_RDK_ONT.Omci.MicErrors` | Direct copy. |
| 33 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.PortNumberOfEntries` | *Gem table* | Number of populated `Gem.{i}` rows. |
| 34 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortID` | `Device.X_RDK_ONT.Gem.{i}.PortId` | Direct copy. |
| 35 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.Direction` | `Device.X_RDK_ONT.Gem.{i}.TrafficType` | Only *Bidirectional* maps (reported as `bidirectional`); *Unidirectional* is rejected because the HAL omits the direction. |
| 36 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesSent` | `Device.X_RDK_ONT.Gem.{i}.TransmittedFrames` | Direct copy. |
| 37 | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesReceived` | `Device.X_RDK_ONT.Gem.{i}.ReceivedFrames` | Direct copy. |
| 38 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalFail` | `Device.X_RDK_ONT.PhysicalMedia.{i}.PerformanceThreshold.SignalFail` | Direct copy. |
| 39 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalDegrade` | `Device.X_RDK_ONT.PhysicalMedia.{i}.PerformanceThreshold.SignalDegrade` | Direct copy. |
| 40 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOS` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LOS` | *Active* → true. |
| 41 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOF` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LOF` | *Active* → true. |
| 42 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SF` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SF` | *Active* → true. |
| 43 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SD` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SD` | *Active* → true. |
| 44 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LCDG` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LCDG` | *Active* → true. |
| 45 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.TF` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.TF` | *Active* → true. |
| 46 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SUF` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SUF` | *Active* → true. |
| 47 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MEM` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.MEM` | *Active* → true. |
| 48 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DACT` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.DACT` | *Active* → true. |
| 49 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DIS` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.DIS` | *Active* → true. |
| 50 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MIS` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.MIS` | *Active* → true. |
| 51 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.PEE` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.PEE` | *Active* → true. |
| 52 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.RDI` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.RDI` | *Active* → true. |
| 53 | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.ROGUE` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.ROGUE` | *Active* → true. |
| 54 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.ID` | *PhysicalMedia row index* | Direct copy. |
| 55 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorName` | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleVendor` | Direct copy. |
| 56 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorPartNumber` | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleName` | Direct copy. |
| 57 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorRevision` | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleVersion` | Direct copy. |
| 58 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.PONMode` | `Device.X_RDK_ONT.PhysicalMedia.{i}.PonMode` | Mapped to *G-PON*, *XG-PON*, *NG-PON2*, *XGS-PON*. |
| 59 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Connector` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Connector` | Direct copy. |
| 60 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateDownstream` | `Device.X_RDK_ONT.PhysicalMedia.{i}.NominalBitRateDownstream` | Passed through without unit conversion. |
| 61 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateUpstream` | `Device.X_RDK_ONT.PhysicalMedia.{i}.NominalBitRateUpstream` | Passed through without unit conversion. |
| 62 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.RxPower` | `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevel` | Passed through without unit conversion. |
| 63 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.TxPower` | `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevel` | Passed through without unit conversion. |
| 64 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Voltage` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Voltage.VoltageLevel` | Negative values are rejected. |
| 65 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Bias` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Bias.CurrentBias` | Passed through without unit conversion. |
| 66 | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Temperature` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Temperature.CurrentTemp` | Values below −274 are rejected. |

### B.2 Unsupported `Device.XPON` with Existing `X_RDK_ONT` Data

These BBF parameters are in Appendix A (they return `RBUS_ERROR_INVALID_OPERATION`), but the data is already in `Device.X_RDK_ONT`. They are candidates for wiring in a later release.

| # | `Device.XPON` Parameter | `Device.X_RDK_ONT` Data | Notes |
|---|---|---|---|
| 1 | `Device.XPON.ONU.{i}.ANI.{i}.PONMode` | `Device.X_RDK_ONT.PhysicalMedia.{i}.PonMode` | Same value already served by `Transceiver.{i}.PONMode`; the ANI-level getter is registered as unsupported. |
| 2 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesSent` | Σ `Device.X_RDK_ONT.Gem.{i}.TransmittedFrames` | Derivable as the sum over all GEM ports; not implemented. |
| 3 | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesReceived` | Σ `Device.X_RDK_ONT.Gem.{i}.ReceivedFrames` | Derivable as the sum over all GEM ports; not implemented. |

## References

- BBF TR-181 Issue 2 Amendment 21 — Device Data Model (`Device.XPON`)
- ITU-T G.984.3 (GPON TC), G.987.3 (XG-PON TC), G.9807.1 (XGS-PON), G.989.3 (NG-PON2) — activation states and alarms
- RDK T2 Telemetry Framework (`telemetry_busmessage_sender.h`)
- GPON Manager data model `config/RdkGponManager.xml`
- RDK → BBF mapping notes `XPON-TR181-MAPPING.md`
- GPON telemetry design `docs/07_Telemetry_Design.md`
- EPON Manager telemetry design (`rdk-eponmanager/design_docs/07_Telemetry_Design.md`, `08_TR181_Telemetry_Reference`)
