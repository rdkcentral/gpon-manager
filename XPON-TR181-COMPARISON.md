# BBF Device.XPON and RDK GPON Manager: Individual Comparisons

## Purpose and Scope

This document compares individual parameters in the supplied Broadband Forum (BBF) `Device:2.21` **`Device.XPON.` subtree** with this GPON Manager's **`Device.X_RDK_ONT.` subtree**. It does not describe availability across the entire RDK platform.

Sources: [config/RdkGponManager.xml](config/RdkGponManager.xml), [tr-181-2-21-0-cwmp-full.xml](tr-181-2-21-0-cwmp-full.xml#L123491). The more detailed analysis is in [XPON-TR181-MAPPING.md](XPON-TR181-MAPPING.md).

- Each row compares one parameter from each side, or one unmatched parameter. Missing counterparts have an **empty cell**.
- **Values** means declared type, allowed values, units, and access, not actual device readings. Each side is identified separately.
- Full data model paths are used. Each `{i}` is an independent instance; instance numbers cannot be assumed to match between models.
- The inventory includes **135 BBF parameters** and **95 RDK parameters**. Five RDK parameters require `WAN_MANAGER_UNIFICATION_ENABLED`; without it, this XML exposes 90 parameters.
- A match means corresponding information is defined in both XML trees. It does **not** prove runtime support, identical units, or complete BBF compliance. Conversion and verification requirements remain in the descriptions.

## Color Key

| Color | Meaning |
| --- | --- |
| <span style="color: #15803d;"><strong>GREEN: Matched</strong></span> | A corresponding parameter is available in the RDK XML. Renaming or value conversion can still be necessary. |
| <span style="color: #a16207;"><strong>AMBER: Partial / candidate</strong></span> | A possible counterpart exists, but its meaning, value coverage, or association is incomplete or unverified. |
| <span style="color: #b91c1c;"><strong>RED: No RDK match</strong></span> | No established counterpart in this GPON Manager's compared subtree. The RDK cell is empty. |
| <span style="color: #0369a1;"><strong>BLUE: No BBF XPON match</strong></span> | No established counterpart inside the compared BBF XPON subtree. The BBF cell is empty. |

Color labels use inline HTML. Viewers that remove inline colors still show the status text. Acronyms within DML paths are the exact XML names, not shortened path aliases.

## Optical Module and Measurements

BBF separates the optical transceiver from the access-network interface. RDK combines much of this information under `Device.X_RDK_ONT.PhysicalMedia.{i}.`.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.ID` | | BBF: unsignedInt, 0..1; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Transceiver identifier; not its table instance number. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Identifier` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Cage` | BBF: unsignedInt, 0..255; read-only.<br>RDK: mapped string, BoB(0), SFP(1); read-only. | <span style="color: #a16207;"><strong>Partial.</strong></span> BBF uses SFF-8024 transceiver identifiers; RDK describes packaging. Numeric values cannot be copied directly. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorName` | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleVendor` | BBF: string(256); read-only.<br>RDK: string(256); read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Likely vendor-name mapping; confirm the RDK value identifies the transceiver vendor. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorPartNumber` | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleName` | BBF: string(256); read-only.<br>RDK: string(256); read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> A module name is not necessarily its vendor part number. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorRevision` | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleVersion` | BBF: string(256); read-only.<br>RDK: string(256); read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Confirm the module version is the transceiver vendor revision, not ONU firmware. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.PONMode` | `Device.X_RDK_ONT.PhysicalMedia.{i}.PonMode` | BBF: Unknown, G-PON, XG-PON, NG-PON2, XGS-PON; read-only.<br>RDK: GPON(0), XG-PON(1), NG-PON2(2), XGS-PON2(3); read-only. | <span style="color: #a16207;"><strong>Partial.</strong></span> GPON becomes G-PON. Verify XGS-PON2 means XGS-PON. RDK lacks Unknown. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Connector` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Connector` | BBF: Unknown, LC, SC, ST, FC, MT-RJ; read-only.<br>RDK: LC(0), ST(1), FC(2), SC(3), MT-RJ(4); read-only. | <span style="color: #a16207;"><strong>Partial.</strong></span> Connector values overlap, but RDK lacks Unknown. Translate by label, not enum position. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateDownstream` | `Device.X_RDK_ONT.PhysicalMedia.{i}.NominalBitRateDownstream` | BBF: unsignedInt, kbps; read-only.<br>RDK: unsignedInt, units unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Downstream nominal rate. Confirm RDK's unit before transferring values. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateUpstream` | `Device.X_RDK_ONT.PhysicalMedia.{i}.NominalBitRateUpstream` | BBF: unsignedInt, kbps; read-only.<br>RDK: unsignedInt, units unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Upstream nominal rate. Confirm RDK's unit before transferring values. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.RxPower` | `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevel` | BBF: int, 0.1 dBm; read-only.<br>RDK: int, scale unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Received optical power; flattened path. Verify the RDK scale. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.TxPower` | `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevel` | BBF: int, 0.1 dBm; read-only.<br>RDK: int, scale unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Transmitted optical power; flattened path. Verify the RDK scale. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Voltage` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Voltage.VoltageLevel` | BBF: unsignedInt, mV; read-only.<br>RDK: int, units unspecified; read-only. | <span style="color: #a16207;"><strong>Partial.</strong></span> Supply voltage. Signedness differs; verify units and handling of negative/invalid readings. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Bias` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Bias.CurrentBias` | BBF: unsignedInt, microamperes; read-only.<br>RDK: unsignedInt, units unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Laser bias current; flattened path. Verify the RDK unit. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Temperature` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Temperature.CurrentTemp` | BBF: int, Celsius, minimum -274; read-only.<br>RDK: int, units/range unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Module temperature. BBF uses -274 for no valid reading; confirm RDK units and invalid-value handling. |
| | `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleFirmwareVersion` | RDK: string(256); read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Module firmware is not established as equivalent to an ONU software-image version. |
| | `Device.X_RDK_ONT.PhysicalMedia.{i}.RedundancyState` | RDK: Active(0), Standby(1); read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Transceiver redundancy state. BBF's two-transceiver capacity does not expose this state. |
| | `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelLowerThreshold` | RDK: int; read-write. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Lower received-optical-power threshold. |
| | `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelUpperThreshold` | RDK: int; read-write. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Upper received-optical-power threshold. |
| | `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelLowerThreshold` | RDK: int; read-write. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Lower transmitted-optical-power threshold. |
| | `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelUpperThreshold` | RDK: int; read-write. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Upper transmitted-optical-power threshold. |

## Access-Network Interface Control

The BBF access-network interface represents the PON network-facing connection. It is not a stackable TR-181 interface and has no LowerLayers parameter.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.Enable` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Enable` | BBF: boolean; read-write.<br>RDK: boolean; read-write. | <span style="color: #a16207;"><strong>Partial.</strong></span> RDK requires `WAN_MANAGER_UNIFICATION_ENABLED`. BBF disabling forces activation state O1 and disables associated transmit lasers; RDK XML does not specify these effects. |
| `Device.XPON.ONU.{i}.ANI.{i}.Status` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Status` | BBF: Up, Down, Unknown, Dormant, NotPresent, Error (optional), NotAllowed; read-only.<br>RDK: Up(0), Down(1), Unknown(2), Dormant(3), NotPresent(4), LowerLayerDown(5), Error(6); read-only. | <span style="color: #a16207;"><strong>Partial.</strong></span> RDK lacks NotAllowed. BBF forbids LowerLayerDown here; its Up state is tied to activation state O5. |
| `Device.XPON.ONU.{i}.ANI.{i}.Alias` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alias` | BBF: _AliasCWMP; read-write.<br>RDK: string(64); read-write. | <span style="color: #15803d;"><strong>Matched.</strong></span> RDK requires `WAN_MANAGER_UNIFICATION_ENABLED`. BBF alias identity/persistence rules still require verification. |
| `Device.XPON.ONU.{i}.ANI.{i}.LastChange` | `Device.X_RDK_ONT.PhysicalMedia.{i}.LastChange` | BBF: unsignedInt, seconds; read-only.<br>RDK: unsignedInt, units unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> RDK requires `WAN_MANAGER_UNIFICATION_ENABLED`. BBF measures time since the current operational state began. |
| `Device.XPON.ONU.{i}.ANI.{i}.Name` | | BBF: string(64); read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Name of the network-facing PON interface. |
| `Device.XPON.ONU.{i}.ANI.{i}.PONMode` | | BBF: Unknown, G-PON, XG-PON, NG-PON2, XGS-PON; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Separate detected/configured interface mode. RDK has one physical-media mode, paired above with transceiver mode; the two BBF modes must not be assumed identical. |
| `Device.XPON.ONU.{i}.ANI.{i}.TransceiverNumberOfEntries` | | BBF: unsignedInt; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Number of transceivers associated with this network-facing interface. |
| `Device.XPON.ONU.{i}.ANI.{i}.SFPReferenceList` | | BBF: list of strong path references, string items max 256; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Associated pluggable-module cage references; added in 2.19. The external cage tree is outside this comparison. |
| | `Device.X_RDK_ONT.PhysicalMedia.{i}.LowerLayers` | RDK: string(1024); read-write. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Requires `WAN_MANAGER_UNIFICATION_ENABLED`. BBF has no corresponding network-facing-interface field; EthernetUNI.LowerLayers belongs to a different interface. |
| | `Device.X_RDK_ONT.PhysicalMedia.{i}.Upstream` | RDK: boolean; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Requires `WAN_MANAGER_UNIFICATION_ENABLED`. BBF's EthernetUNI.Upstream is at a different interface, not a direct substitute. |

## Error-Rate Thresholds and Alarms

The thresholds below concern bit errors, not optical power. For every matched alarm, convert RDK **Active(0) to true** and **Inactive(1) to false**; directly casting the numeric code to boolean would invert its meaning.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalFail` | `Device.X_RDK_ONT.PhysicalMedia.{i}.PerformanceThreshold.SignalFail` | BBF: unsignedInt, 3..8; read-only.<br>RDK: unsignedInt, range unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Signal-failure bit-error-rate threshold, expressed by BBF as $10^{-n}$. Verify RDK uses this exponent convention. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalDegrade` | `Device.X_RDK_ONT.PhysicalMedia.{i}.PerformanceThreshold.SignalDegrade` | BBF: unsignedInt, 4..10; read-only.<br>RDK: unsignedInt, range unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Signal-degradation threshold exponent; BBF requires it to exceed the SignalFail exponent. Verify RDK semantics. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.RDI` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.RDI` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Remote defect indication; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.PEE` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.PEE` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Receipt of a physical-equipment-error message; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOS` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LOS` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Loss of signal; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOF` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LOF` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Loss of frame; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DACT` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.DACT` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Deactivate ONU-ID indication; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DIS` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.DIS` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Disabled optical network unit; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MIS` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.MIS` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Link mismatch; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MEM` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.MEM` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Message error message; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SUF` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SUF` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Start-up failure; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SF` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SF` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Signal failure; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SD` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SD` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Signal degradation; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LCDG` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LCDG` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Loss of GEM channel delineation; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.TF` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.TF` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Transmitter failure; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.ROGUE` | `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.ROGUE` | BBF: boolean; read-only.<br>RDK: Active(0), Inactive(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Non-conforming upstream transmission; convert enum to boolean. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LODS` | | BBF: boolean; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Loss of downstream synchronization. |

## Physical-Layer Counters

`StatsCounter32` and `StatsCounter64` are BBF unsigned 32-bit and 64-bit statistics types. Widening an RDK integer does not recover past counter wraps. BBF counters are per network-facing interface; RDK's Gtc object is a singleton.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECBytes` | `Device.X_RDK_ONT.Gtc.CorrectedFecBytes` | BBF: StatsCounter64; read-only.<br>RDK: unsignedInt (32-bit); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Bytes corrected by forward error correction. Requires a genuine 64-bit source for BBF. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECCodewords` | `Device.X_RDK_ONT.Gtc.CorrectedFecCodeWords` | BBF: StatsCounter64; read-only.<br>RDK: unsignedInt (32-bit); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Erroneous codewords successfully corrected. Counter width and spelling differ. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UncorrectableFECCodewords` | `Device.X_RDK_ONT.Gtc.UnCorrectedFecCodeWords` | BBF: StatsCounter64; read-only.<br>RDK: unsignedInt (32-bit); read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Confirm RDK counts uncorrectable codewords, not all codewords left unchanged. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.TotalFECCodewords` | `Device.X_RDK_ONT.Gtc.TotalFecCodeWords` | BBF: StatsCounter64; read-only.<br>RDK: unsignedInt (32-bit); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Total received forward-error-correction codewords. Counter width and spelling differ. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.PSBdHECErrorCount` | `Device.X_RDK_ONT.Gtc.PSBdHecErrors` | BBF: StatsCounter32; read-only.<br>RDK: unsignedInt (32-bit); read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Header-error-control errors in the downstream physical synchronization block. Confirm RDK counts the same fields. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.HeaderHECErrorCount` | | BBF: StatsCounter32; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Downstream XGTC/FS header errors. RDK's HecErrorCount and FrameHecErrors lack definitions establishing which counter belongs here. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UnknownProfile` | | BBF: StatsCounter32; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Transmission prevented by an unknown burst profile. |
| | `Device.X_RDK_ONT.Gtc.HecErrorCount` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> No established one-to-one mapping: the XML does not identify which header layer this counts. |
| | `Device.X_RDK_ONT.Gtc.FrameHecErrors` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> No established one-to-one mapping: distinguish transmission-convergence headers from GEM frame headers using backend definitions. |
| | `Device.X_RDK_ONT.Gtc.FramesLost` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Lost frames; not established as equivalent to a header-error counter. |

## Activation and Physical-Layer Management Messages

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUID` | `Device.X_RDK_ONT.Ploam.OnuId` | BBF: unsignedInt, 0..1022; read-only.<br>RDK: unsignedInt, 0..1022; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Identifier assigned during activation by the optical line terminal. Path and case change. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.VendorID` | `Device.X_RDK_ONT.Ploam.VendorId` | BBF: string(4); read-only.<br>RDK: string(128); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Optical network unit vendor identifier. Enforce BBF's shorter length. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.SerialNumber` | `Device.X_RDK_ONT.Ploam.SerialNumber` | BBF: string(12); read-only.<br>RDK: string(128); read-only. | <span style="color: #a16207;"><strong>Partial.</strong></span> BBF requires vendor ID plus vendor-specific serial number in human-readable form. RDK's XML does not define its format. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUState` | `Device.X_RDK_ONT.Ploam.RegistrationState` | BBF: O1, O2, O3, O2-3, O4, O5, O6, O7, O8, O9; read-only.<br>RDK: O1(0)..O9(8); read-only. | <span style="color: #a16207;"><strong>Partial.</strong></span> Activation state. RDK has no combined O2-3 value. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.UpstreamMessageCount` | `Device.X_RDK_ONT.Ploam.TxMessageCount` | BBF: StatsCounter64; read-only.<br>RDK: unsignedInt; read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Upstream management messages. BBF excludes acknowledgements; RDK counting rules are unspecified. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.DownstreamMessageCount` | `Device.X_RDK_ONT.Ploam.RxMessageCount` | BBF: StatsCounter64; read-only.<br>RDK: unsignedInt; read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Received broadcast and ONU-addressed management messages. Confirm RDK filtering and widen the counter. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.MICErrors` | `Device.X_RDK_ONT.Ploam.MicErrors` | BBF: StatsCounter32; read-only.<br>RDK: unsignedInt; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Received physical-layer management messages with message-integrity-check errors. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.RangingTime` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Counts Ranging_Time messages. Despite the name, this is not a duration or registration timer. |
| | `Device.X_RDK_ONT.Ploam.ActivationCounter` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Activation count. |
| | `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO1` | RDK: unsignedInt, units unspecified; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Registration timer TO1; no equivalent XPON timer. |
| | `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO2` | RDK: unsignedInt, units unspecified; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Registration timer TO2; no equivalent XPON timer. |

## ONU Management and Control Messages

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.BaselineMessagesReceived` | `Device.X_RDK_ONT.Omci.RxBaseLineMessageCountValid` | BBF: StatsCounter64; read-only.<br>RDK: int (signed); read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Baseline control-message count. Confirm valid-message filtering and negative sentinels before conversion to an unsigned 64-bit counter. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.ExtendedMessagesReceived` | `Device.X_RDK_ONT.Omci.RxExtendedMessageCountValid` | BBF: StatsCounter64; read-only.<br>RDK: int (signed); read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Extended control-message count. Confirm valid-message filtering and negative sentinels. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.MICErrors` | `Device.X_RDK_ONT.Omci.MicErrors` | BBF: StatsCounter32; read-only.<br>RDK: unsignedInt; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Received control messages with message-integrity-check errors. |

## GEM Ports and Frame Counters

GEM is the PON encapsulation layer. Its frame counters are not substitutes for Ethernet packet/byte counters. BBF distinguishes per-port counters from totals for the network-facing interface.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.PortNumberOfEntries` | | BBF: unsignedInt; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Explicit GEM-port count. RDK table-enumeration callbacks are not an exposed count parameter. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortID` | `Device.X_RDK_ONT.Gem.{i}.PortId` | BBF: unsignedInt, 0..65534; read-only.<br>RDK: unsignedInt, range unspecified; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> GEM port identifier; not a table instance number. Enforce the BBF range. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.Direction` | `Device.X_RDK_ONT.Gem.{i}.TrafficType` | BBF: UNI-to-ANI, ANI-to-UNI, bidirectional; read-only.<br>RDK: Unidirectional(0), Bidirectional(1); read-only. | <span style="color: #a16207;"><strong>Partial.</strong></span> RDK cannot distinguish the two unidirectional cases. Bidirectional needs a spelling/case conversion. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortType` | | BBF: unicast, multicast, broadcast; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Destination category, separate from direction. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesSent` | `Device.X_RDK_ONT.Gem.{i}.TransmittedFrames` | BBF: StatsCounter64; read-only.<br>RDK: unsignedInt; read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Per-port transmitted GEM frames. Confirm counted frame layer and widen the counter. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesReceived` | `Device.X_RDK_ONT.Gem.{i}.ReceivedFrames` | BBF: StatsCounter64; read-only.<br>RDK: unsignedInt; read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Per-port received GEM frames. Confirm counted frame layer and widen the counter. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesSent` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Interface-wide transmitted GEM frames. Per-port counts are not directly equivalent; aggregation needs verified inclusion/reset rules. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesReceived` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Interface-wide received GEM frames; not a single port's received-frame counter. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FrameHeaderHECErrors` | | BBF: StatsCounter32; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> GEM-frame header errors. RDK's two unspecified header-error counters cannot be assigned here from XML alone. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.KeyErrors` | | BBF: StatsCounter32; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Frames discarded for invalid/unknown encryption keys; included in BBF's received-frame total. |

## Ethernet User-Network Interface

BBF EthernetUNI can represent physical or virtual user-network interfaces. An RDK virtual Ethernet interface point (Veip) is a possible counterpart for a virtual interface, not every BBF EthernetUNI instance.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Enable` | `Device.X_RDK_ONT.Veip.{i}.AdministrativeState` | BBF: boolean; read-only.<br>RDK: Lock(0), Unlock(1); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Administrative state: Lock becomes false, Unlock becomes true. BBF reflects optical-line-terminal control; raw OMCI state codes differ from RDK's enum codes. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Status` | `Device.X_RDK_ONT.Veip.{i}.OperationalState` | BBF: Up, Down, Unknown, Dormant, NotPresent, LowerLayerDown, Error (optional); read-only.<br>RDK: same labels mapped to 0..6; read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Operational state. BBF says LowerLayerDown should not be used for this layer-1 interface; confirm traffic-provisioning semantics. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Name` | `Device.X_RDK_ONT.Veip.{i}.InterfaceName` | BBF: string(64); read-only.<br>RDK: string(256); read-only. | <span style="color: #a16207;"><strong>Candidate.</strong></span> Interface name; verify naming semantics and enforce the shorter BBF length. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainID` | `Device.X_RDK_ONT.Veip.{i}.MeId` | BBF: string(256); read-only.<br>RDK: unsignedInt; read-only. | <span style="color: #a16207;"><strong>Partial.</strong></span> For a VEIP service, format the managed-entity identifier as a string such as (VEIP,1025). BBF also supports other service types. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainName` | `Device.X_RDK_ONT.Veip.{i}.InterDomainName` | BBF: string(25); read-only.<br>RDK: string(25); read-only. | <span style="color: #15803d;"><strong>Matched.</strong></span> Interdomain service name for the associated virtual Ethernet interface; capitalization changes. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Alias` | | BBF: _AliasCWMP; read-write. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> User-network-interface alias; RDK physical-media Alias belongs to another interface. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Description` | | BBF: string(256); read-write. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> User-friendly interface description; added in 2.21. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.LastChange` | | BBF: unsignedInt, seconds; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Time since this user-network interface entered its current operational state. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.LowerLayers` | | BBF: strong interface-reference list, maximum length 1024; read-write. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Interface stacking references; expected unused for this layer-1 interface. Not the RDK physical-media LowerLayers parameter. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Upstream` | | BBF: boolean; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Whether this interface points towards the Internet; added in 2.19. Not the physical-media flag. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.ANIs` | | BBF: strong network-facing-interface reference list, maximum length 1024; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Associated network-facing interfaces. The list alone does not mean forwarding has been provisioned. |

## Network-Facing Interface Throughput Statistics

**This GPON Manager's `Device.X_RDK_ONT.` tree has no corresponding throughput-statistics subtree.** These BBF counters describe Ethernet traffic, not GEM frames. Transmit means towards the PON; receive means from the PON. BBF specifies counter resets following administrative enable/disable transitions.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesSent` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Transmitted bytes, including framing characters. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesReceived` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received bytes, including framing characters. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsSent` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Transmitted Ethernet frames. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsReceived` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received Ethernet frames. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsSent` | | BBF: StatsCounter32; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Outbound frames that could not be transmitted because of errors. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsReceived` | | BBF: StatsCounter32; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Inbound frames whose errors prevented forwarding to user-network interfaces. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsSent` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Unicast transmission requests, including frames discarded or not sent. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsReceived` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received unicast frames forwarded to user-network interfaces. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsSent` | | BBF: StatsCounter32; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Outbound frames deliberately discarded without detected errors, for example to free buffer space. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsReceived` | | BBF: StatsCounter32; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Inbound frames deliberately discarded without detected errors. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsSent` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Multicast transmission requests, including frames discarded or not sent. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsReceived` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received multicast frames forwarded to user-network interfaces. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsSent` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Broadcast transmission requests, including frames discarded or not sent. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsReceived` | | BBF: StatsCounter64; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received broadcast frames forwarded to user-network interfaces. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.UnknownProtoPacketsReceived` | | BBF: StatsCounter32; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received frames discarded because their protocol is unknown or unsupported. |

## User-Network Interface Throughput Statistics

These are separate BBF counters for each EthernetUNI, not aliases of the network-facing counters above. The RDK counterpart is also absent here. BBF defines transmit as towards the PON and receive as from the PON, with administrative-transition reset rules.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesSent` | | BBF: StatsCounter64, bytes; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Transmitted bytes, including framing characters. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesReceived` | | BBF: StatsCounter64, bytes; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received bytes, including framing characters. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsSent` | | BBF: StatsCounter64, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Total transmitted packets. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsReceived` | | BBF: StatsCounter64, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Total received packets. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsSent` | | BBF: StatsCounter32, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Outbound packets that could not be transmitted because of errors. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsReceived` | | BBF: StatsCounter32, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Inbound packets whose errors prevented delivery to a higher layer. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsSent` | | BBF: StatsCounter64, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Unicast transmission requests, including packets discarded or not sent. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsReceived` | | BBF: StatsCounter64, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received unicast packets delivered to a higher layer. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsSent` | | BBF: StatsCounter32, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Outbound packets deliberately discarded without detected errors. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsReceived` | | BBF: StatsCounter32, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Inbound packets deliberately discarded without detected errors. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsSent` | | BBF: StatsCounter64, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Multicast transmission requests, including packets discarded or not sent. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsReceived` | | BBF: StatsCounter64, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received multicast packets delivered to a higher layer. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsSent` | | BBF: StatsCounter64, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Broadcast transmission requests, including packets discarded or not sent. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsReceived` | | BBF: StatsCounter64, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received broadcast packets delivered to a higher layer. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnknownProtoPacketsReceived` | | BBF: StatsCounter32, packets; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Received packets discarded because their protocol is unknown or unsupported. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.Reset` | | BBF: boolean; read-write command. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Set true to reset this interface's statistics; added in 2.19. BBF does not define the same command under ANI.Stats in this XML. |

## ONU Identity and Inventory

The BBF optical network unit (ONU) table has no equivalent parent table in the RDK tree. Whole-unit controls must not be confused with physical-media controls.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONUNumberOfEntries` | | BBF: unsignedInt; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Number of optical network units. |
| `Device.XPON.ONU.{i}.Enable` | | BBF: boolean; read-write. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Enables/disables the entire optical network unit, including traffic, notification, and power effects. Not physical-media Enable. |
| `Device.XPON.ONU.{i}.Name` | | BBF: string(64); read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Device-assigned optical network unit name. |
| `Device.XPON.ONU.{i}.Version` | | BBF: string(14); read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Vendor-defined optical network unit version; not established as the module revision. |
| `Device.XPON.ONU.{i}.EquipmentID` | | BBF: string(20); read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Identifier of the optical network unit equipment type. |
| `Device.XPON.ONU.{i}.PowerCapability` | | BBF: enumeration list, On, Off, LowPower (optional); read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Supported power states; added in 2.20. |
| `Device.XPON.ONU.{i}.PowerStatus` | | BBF: state from PowerCapability, or Error; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Current whole-unit power state; added in 2.20. |
| `Device.XPON.ONU.{i}.SoftwareImageNumberOfEntries` | | BBF: unsignedInt; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Number of stored software-image entries. |
| `Device.XPON.ONU.{i}.EthernetUNINumberOfEntries` | | BBF: unsignedInt; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Number of Ethernet user-network-interface entries. |
| `Device.XPON.ONU.{i}.ANINumberOfEntries` | | BBF: unsignedInt; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Number of network-facing-interface entries. |

## Software Images

BBF permits up to two image entries per optical network unit. RDK's module firmware string alone does not establish this inventory or a direct image-version mapping.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.SoftwareImage.{i}.ID` | | BBF: unsignedInt, 0..1; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Software-image identifier, separate from table instance. |
| `Device.XPON.ONU.{i}.SoftwareImage.{i}.Version` | | BBF: string(14); read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Version of an individual ONU software image. A module firmware version may concern different firmware/hardware. |
| `Device.XPON.ONU.{i}.SoftwareImage.{i}.IsCommitted` | | BBF: boolean; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Whether the image is committed. |
| `Device.XPON.ONU.{i}.SoftwareImage.{i}.IsActive` | | BBF: boolean; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Whether the image is active. |
| `Device.XPON.ONU.{i}.SoftwareImage.{i}.IsValid` | | BBF: boolean; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Whether the image is valid. |

## Authentication

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.Password` | | BBF: secured string(72); read-write. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Authentication password/registration ID; added in 2.17. Effective lengths: G-PON 10 ASCII or 20 hex characters; other listed modes 36 ASCII or 72 hex characters. Empty means no password authentication. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.HexadecimalPassword` | | BBF: boolean; read-write. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> False selects ASCII password representation; true selects hexadecimal. |

## Power Management

This entire BBF object was added in 2.20 and has no counterpart in the compared RDK tree. Timing values use units of 125-microsecond physical-layer frames.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| `Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementCapability` | | BBF: enumeration list, WatchfulSleepMode, DozeMode, CyclicSleepMode; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Supported sleep modes. Empty means no supported modes. |
| `Device.XPON.ONU.{i}.PowerManagement.OLTSleepAllow` | | BBF: boolean; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Whether the optical line terminal allows power reduction. |
| `Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementStatus` | | BBF: Active, WatchfulSleepMode, DozeMode, CyclicSleepMode; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Current power-reduction state. |
| `Device.XPON.ONU.{i}.PowerManagement.ITransInit` | | BBF: unsignedInt, 125-microsecond frames; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Complete transceiver initialization time after sleep. |
| `Device.XPON.ONU.{i}.PowerManagement.ITxInit` | | BBF: unsignedInt, 125-microsecond frames; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Transmitter initialization time after listening. |
| `Device.XPON.ONU.{i}.PowerManagement.MaxSleepInterval` | | BBF: unsignedInt, 125-microsecond frames; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Maximum asleep/listen/low-power interval. |
| `Device.XPON.ONU.{i}.PowerManagement.MinAwareInterval` | | BBF: unsignedInt, 125-microsecond frames; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Minimum aware interval before re-entering sleep or listen. |
| `Device.XPON.ONU.{i}.PowerManagement.MinActiveHeldInterval` | | BBF: unsignedInt, 125-microsecond frames; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Minimum interval in the active-held state. |
| `Device.XPON.ONU.{i}.PowerManagement.MaxSleepIntervalExtension` | | BBF: unsignedLong, two 4-byte interval values; read-only. | <span style="color: #b91c1c;"><strong>No RDK match.</strong></span> Separate doze/cyclic-sleep limits in 125-microsecond units. Overrides MaxSleepInterval when supported; ignored for watchful sleep. |

## RDK GEM VLAN Flows

There is no matching VLAN-flow subtree **inside BBF Device.XPON**. Other BBF branches were not compared. RDK declares these numeric fields as unsignedInt without VLAN-specific ranges.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.Tagged` | RDK: Single(0), Double(1); read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Ingress tagging mode for this GEM port. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Vid` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Ingress service-VLAN identifier. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Pcp` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Ingress service-VLAN priority code point. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Dei` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Ingress service-VLAN drop eligibility indicator. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Vid` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Ingress customer-VLAN identifier. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Pcp` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Ingress customer-VLAN priority code point. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Dei` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Ingress customer-VLAN drop eligibility indicator. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.Tagged` | RDK: Single(0), Double(1); read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Egress tagging mode for this GEM port. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Vid` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Egress service-VLAN identifier. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Pcp` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Egress service-VLAN priority code point. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Dei` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Egress service-VLAN drop eligibility indicator. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Vid` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Egress customer-VLAN identifier. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Pcp` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Egress customer-VLAN priority code point. |
| | `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Dei` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Egress customer-VLAN drop eligibility indicator. |

## RDK Virtual-Interface VLAN Flows and TR-69

These parameters also have no counterparts in the compared BBF XPON subtree. RDK's virtual-interface tagging codes differ from its GEM tagging codes.

| BBF | RDK | Values | Description |
| --- | --- | --- | --- |
| | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Tagged` | RDK: Untagged(0), Single(1), Double(2); read-write. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Configurable ingress tagging mode. |
| | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Vid` | RDK: unsignedInt; read-write. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Configurable ingress VLAN identifier. |
| | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Pcp` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Ingress VLAN priority code point. |
| | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Dei` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Ingress VLAN drop eligibility indicator. |
| | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Tagged` | RDK: Untagged(0), Single(1), Double(2); read-write. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Configurable egress tagging mode. |
| | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Vid` | RDK: unsignedInt; read-write. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Configurable egress VLAN identifier. |
| | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Pcp` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Egress VLAN priority code point. |
| | `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Dei` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> Egress VLAN drop eligibility indicator. |
| | `Device.X_RDK_ONT.TR69.url` | RDK: string(256); read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> TR-69 provisioning URL; no corresponding XPON parameter. |
| | `Device.X_RDK_ONT.TR69.AssociatedTag` | RDK: unsignedInt; read-only. | <span style="color: #0369a1;"><strong>No BBF XPON match.</strong></span> TR-69 associated tag; its semantics are not defined by the RDK XML. |

## Key Points for Discussion

1. Green means RDK already exposes corresponding information, not that its existing path or value can always be copied unchanged.
2. Amber entries need conversion, additional information, or backend verification before being treated as equivalent.
3. Red entries show gaps in this GPON Manager's XML exposure, not proof that the whole RDK platform or hardware lacks the functionality.
4. Blue entries can remain vendor extensions if required; they have no established counterpart within the BBF XPON scope.
5. Throughput statistics are listed individually for both BBF interface types, each with a blank RDK cell. Neither is represented by the existing GEM frame counters.

Validation scope: XML path existence, individual-row structure, complete parameter coverage, and color/empty-cell consistency. Runtime behavior and hardware capabilities are not validated.