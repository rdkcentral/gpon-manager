# GPON Manager Device.X_RDK_ONT to BBF Device.XPON Mapping

## Scope and Reading Guide

Compared sources:

- [config/RdkGponManager.xml](config/RdkGponManager.xml): the complete `Device.X_RDK_ONT.` tree.
- [tr-181-2-21-0-cwmp-full.xml](tr-181-2-21-0-cwmp-full.xml#L123491): **only `Device.XPON.` and its descendants**, in the supplied BBF `Device:2.21` CWMP model. This comparison does not claim that this is the latest published BBF revision.

This is an XML contract comparison, not verification of C callbacks, HAL values, or runtime support. A proposed equivalent is not proof that the implementations use the same units, reset behavior, or instance association. "Absent" means absent from the compared XML subtree, not from all of TR-181 or the device.

Access is written as **read-only** or **read-write**. RDK parameters without `<writable>true</writable>` are treated as read-only. Parameters marked `WAN_MANAGER_UNIFICATION_ENABLED` exist only when that build option is enabled. RDK `uint32/mapped` fields expose strings backed by numeric enum codes; those codes are not BBF string values.

All object and parameter mappings use full data model (DML) paths, without shortened prefixes. Names within the paths retain the exact spelling from the source XML.

Each `{i}` is an independent table instance. RDK and BBF instance numbers must not be assumed to match. "Candidate" means the names suggest a relationship, but the RDK XML does not establish the BBF meaning.

| Inventory | RDK Device.X_RDK_ONT | BBF Device.XPON |
| --- | --- | --- |
| Object definitions, including containers | 28 | 22 |
| Table definitions | 3 | 6 |
| Parameter definitions | 95 with `WAN_MANAGER_UNIFICATION_ENABLED`; 90 without `WAN_MANAGER_UNIFICATION_ENABLED` | 135 |

These are definition counts, not runtime instances or a claim that every BBF field is mandatory on every device. Every parameter is accounted for below; grouped rows enumerate each path explicitly.

## Object and Tree Mapping

| RDK object | BBF object | Structural difference |
| --- | --- | --- |
| `Device.X_RDK_ONT.` | `Device.XPON.`, `Device.XPON.ONU.{i}.` | RDK has one vendor container. BBF has an explicit ONU table; it supports multiple ONUs. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.` | `Device.XPON.ONU.{i}.ANI.{i}.`, `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.` | Split: interface-like control/state belongs to ANI; optical module identity and measurements belong to Transceiver. RDK maximum is 128 physical-media entries; BBF allows unbounded ANIs per ONU and at most 2 transceivers per ANI. No association rule is provided by the RDK XML. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.`, `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.`, `Device.X_RDK_ONT.PhysicalMedia.{i}.Voltage.`, `Device.X_RDK_ONT.PhysicalMedia.{i}.Bias.`, `Device.X_RDK_ONT.PhysicalMedia.{i}.Temperature.` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.` | BBF exposes measurements as direct parameters, not nested singleton objects. RDK optical power threshold parameters have no XPON counterpart. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.PerformanceThreshold.` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.` | Renamed and moved from physical media to the ANI TC layer. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.` | Renamed and moved; alarm sets and representations differ. |
| `Device.X_RDK_ONT.Gtc.` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.`, `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.` | RDK singleton statistics are split by PHY/GEM meaning and become per-ANI. Some counters have no established match. |
| `Device.X_RDK_ONT.Ploam.` | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.`, `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.` | Activation identity/state and PLOAM monitoring are separate in BBF. |
| `Device.X_RDK_ONT.Ploam.RegistrationTimers.` | None | No corresponding XPON timer object. |
| `Device.X_RDK_ONT.Gem.{i}.` | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.`, `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.`, `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.` | RDK maximum is 128 GEM entries. BBF has an unbounded per-ANI port table; counters are under each port's PM object. |
| `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.` and its descendants | None | No equivalent VLAN-flow subtree inside BBF XPON. |
| `Device.X_RDK_ONT.Omci.` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.` | Monitoring moves from a global singleton to each ANI. |
| `Device.X_RDK_ONT.Veip.{i}.` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.` | Partial: BBF EthernetUNI covers physical and virtual UNIs, including VEIP-associated services. RDK maximum is 128 VEIPs; BBF UNI table is unbounded per ONU. |
| `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.` and its descendants | None | No equivalent VLAN-flow subtree inside BBF XPON. |
| `Device.X_RDK_ONT.TR69.` | None | No TR-69 provisioning object inside BBF XPON. |
| None | `Device.XPON.ONU.{i}.ANI.{i}.Stats.`, `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.`, `Device.XPON.ONU.{i}.SoftwareImage.{i}.`, `Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.`, `Device.XPON.ONU.{i}.PowerManagement.` | BBF-only statistics, software-image, authentication, and power-management objects. |
| No separate container | `Device.XPON.ONU.{i}.ANI.{i}.TC.`, `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.` | BBF groups transmission-convergence functionality and performance monitoring explicitly. |

BBF ANI is explicitly **not** a stackable TR-181 interface object: it has no `LowerLayers`, excludes `LowerLayerDown`, and does not appear in `Device.InterfaceStack`. EthernetUNI is a stackable interface object. Consequently, the RDK physical-media interface fields cannot all be moved to ANI unchanged.

## Physical Media and Optical Diagnostics

| RDK parameter | BBF parameter | Mapping and differences |
| --- | --- | --- |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Cage` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Identifier` | Related, not equivalent. RDK mapped `BoB(0), SFP(1)` describes packaging; BBF is an unsigned integer 0..255 using SFF-8024 transceiver type identifiers. Do not copy numeric codes. Both read-only. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleVendor` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorName` | Candidate rename; both string(256), read-only. BBF specifically identifies the transceiver vendor. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleName` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorPartNumber` | Candidate only: a module name is not necessarily the SFF vendor part number. Both string(256), read-only. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleVersion` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorRevision` | Candidate only: BBF requires the SFF vendor revision. Both string(256), read-only. Do not assume this is the ONU-wide `Device.XPON.ONU.{i}.Version`. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleFirmwareVersion` | No direct equivalent; possible `Device.XPON.ONU.{i}.SoftwareImage.{i}.Version` | RDK is string(256), read-only, for module firmware. BBF software-image version is string(14), read-only, in an ONU image table with up to 2 entries. Different owner and multiplicity; not a direct rename. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.PonMode` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.PONMode`; related `Device.XPON.ONU.{i}.ANI.{i}.PONMode` | RDK: `GPON(0), XG-PON(1), NG-PON2(2), XGS-PON2(3)`. BBF: `Unknown, G-PON, XG-PON, NG-PON2, XGS-PON`. Translate `GPON` to `G-PON`; verify the intended meaning of RDK `XGS-PON2` before translating to `XGS-PON`. RDK has no `Unknown`. BBF separates transceiver mode from detected/configured ANI mode; they need not match if the wrong module is inserted. All read-only. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Connector` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Connector` | RDK: `LC(0), ST(1), FC(2), SC(3), MT-RJ(4)`. BBF has these string values plus `Unknown`; enum order is not a numeric mapping. Both read-only. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.NominalBitRateDownstream` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateDownstream` | Same name and unsignedInt, read-only. BBF specifies kbps; RDK XML does not specify units. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.NominalBitRateUpstream` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateUpstream` | Same name and unsignedInt, read-only. BBF specifies kbps; RDK XML does not specify units. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Enable` (`WAN_MANAGER_UNIFICATION_ENABLED`) | `Device.XPON.ONU.{i}.ANI.{i}.Enable` | Boolean, read-write on both. BBF disable forces ANI activation state O1 and disables associated TX laser(s). RDK XML does not document these side effects. Not equivalent to the broader ONU-level `Device.XPON.ONU.{i}.Enable`. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Status` | `Device.XPON.ONU.{i}.ANI.{i}.Status` | Both read-only string enums. Shared: `Up, Down, Unknown, Dormant, NotPresent, Error`. RDK also has `LowerLayerDown`, which ANI forbids. BBF 2.21 adds `NotAllowed`, absent in RDK. BBF ties `Up` to ONU state O5 and `NotAllowed` to forcing O1. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.RedundancyState` | None | RDK-only mapped `Active(0), Standby(1)`, read-only. BBF's two-transceiver capacity does not provide this state parameter. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alias` (`WAN_MANAGER_UNIFICATION_ENABLED`) | `Device.XPON.ONU.{i}.ANI.{i}.Alias` | RDK string(64), read-write; BBF `_AliasCWMP`, read-write, with standardized alias identity/persistence requirements not expressed in RDK XML. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.LastChange` (`WAN_MANAGER_UNIFICATION_ENABLED`) | `Device.XPON.ONU.{i}.ANI.{i}.LastChange` | Both unsignedInt, read-only. BBF specifies elapsed seconds since the current operational state began; RDK XML supplies no unit/reset definition. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.LowerLayers` (`WAN_MANAGER_UNIFICATION_ENABLED`) | None at ANI | RDK string(1024), read-write. BBF ANI deliberately has no `LowerLayers`. `Device.XPON.ONU.{i}.EthernetUNI.{i}.LowerLayers` belongs to a different logical interface and is not a substitute. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Upstream` (`WAN_MANAGER_UNIFICATION_ENABLED`) | None at ANI | RDK boolean, read-only. `Device.XPON.ONU.{i}.EthernetUNI.{i}.Upstream` exists but describes the UNI, not the ANI/physical-media entry. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevel` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.RxPower` | Flatten/rename; both int, read-only. BBF unit is 0.1 dBm; RDK XML gives no scale. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelLowerThreshold`, `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelUpperThreshold` | None | RDK-only int, read-write. BBF XPON has no corresponding RX optical-power threshold parameters. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevel` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.TxPower` | Flatten/rename; both int, read-only. BBF unit is 0.1 dBm; RDK XML gives no scale. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelLowerThreshold`, `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelUpperThreshold` | None | RDK-only int, read-write. BBF XPON has no corresponding TX optical-power threshold parameters. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Voltage.VoltageLevel` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Voltage` | Flatten/rename; RDK int versus BBF unsignedInt, both read-only. BBF unit is mV; RDK XML gives no scale or invalid-value convention. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Bias.CurrentBias` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Bias` | Flatten/rename; both unsignedInt, read-only. BBF unit is microamperes; RDK XML gives no scale. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Temperature.CurrentTemp` | `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Temperature` | Flatten/rename; both int, read-only. BBF specifies degrees Celsius, minimum -274, and -274 for no valid reading. RDK XML specifies neither unit nor sentinel. |

## Performance Thresholds and Alarms

These are BER thresholds, not replacements for the RDK RX/TX optical-power thresholds.

| RDK parameter | BBF parameter | Mapping and differences |
| --- | --- | --- |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.PerformanceThreshold.SignalFail` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalFail` | Both unsignedInt, read-only. BBF restricts the exponent to 3..8 and defines the downstream BER threshold as $10^{-n}$. RDK XML supplies no range or exponent semantics. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.PerformanceThreshold.SignalDegrade` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalDegrade` | Both unsignedInt, read-only. BBF restricts the exponent to 4..10; it must exceed the SignalFail exponent so the SD BER threshold is lower. RDK XML expresses neither constraint. |

All RDK alarms below are read-only mapped strings `Active(0), Inactive(1)`; all BBF alarms are read-only booleans. Translate **`Active(0) -> true`, `Inactive(1) -> false`**. Casting the RDK numeric enum directly to a boolean would invert its meaning. Alarm ownership also moves from a physical-media entry to an ANI's TC layer.

| RDK parameter | BBF parameter | Meaning / difference |
| --- | --- | --- |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.RDI` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.RDI` | Remote defect indication; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.PEE` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.PEE` | Receipt of a PEE message; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LOS` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOS` | Loss of signal; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LOF` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOF` | Loss of frame; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.DACT` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DACT` | Deactivate ONU-ID; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.DIS` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DIS` | Disabled ONU; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.MIS` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MIS` | Link mismatching; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.MEM` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MEM` | Message error message; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SUF` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SUF` | Start-up failure; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SF` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SF` | Signal failed; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.SD` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SD` | Signal degraded; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LCDG` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LCDG` | Loss of GEM channel delineation; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.TF` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.TF` | Transmitter failure; enum-to-boolean conversion. |
| `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.ROGUE` | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.ROGUE` | Non-conforming upstream transmission; enum-to-boolean conversion. |
| None | `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LODS` | BBF-only loss of downstream synchronization alarm. |

## GTC, PLOAM, and OMCI

All parameters in these tables are read-only. `StatsCounter32` and `StatsCounter64` are BBF 32-bit and 64-bit unsigned statistics types, not simply names for ordinary RDK integers. RDK's GTC, PLOAM, and OMCI singletons do not establish which ONU/ANI owns their values.

| RDK parameter | BBF parameter | Mapping and differences |
| --- | --- | --- |
| `Device.X_RDK_ONT.Gtc.CorrectedFecBytes` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECBytes` | Rename; unsignedInt -> StatsCounter64. BBF counts bytes corrected by FEC. |
| `Device.X_RDK_ONT.Gtc.CorrectedFecCodeWords` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECCodewords` | Rename/case change; unsignedInt -> StatsCounter64. BBF counts erroneous codewords successfully corrected. |
| `Device.X_RDK_ONT.Gtc.UnCorrectedFecCodeWords` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UncorrectableFECCodewords` | Candidate rename; unsignedInt -> StatsCounter64. Verify that RDK counts uncorrectable codewords, not every codeword left unmodified. |
| `Device.X_RDK_ONT.Gtc.TotalFecCodeWords` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.TotalFECCodewords` | Rename/case change; unsignedInt -> StatsCounter64. BBF counts total received codewords. |
| `Device.X_RDK_ONT.Gtc.PSBdHecErrors` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.PSBdHECErrorCount` | Candidate rename; unsignedInt -> StatsCounter32. BBF specifically counts HEC errors in PSBd fields. |
| `Device.X_RDK_ONT.Gtc.HecErrorCount`, `Device.X_RDK_ONT.Gtc.FrameHecErrors` | Possible `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.HeaderHECErrorCount` and/or `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FrameHeaderHECErrors` | **Ambiguous, not two confirmed one-to-one mappings.** RDK only declares unsignedInt counters. BBF distinguishes downstream XGTC/FS header HEC errors from (X)GEM frame-header HEC errors; both are StatsCounter32. Counter definitions from the backend are needed to assign either RDK field. |
| `Device.X_RDK_ONT.Gtc.FramesLost` | None established | RDK unsignedInt. No BBF XPON `FramesLost` parameter; do not equate lost frames with a header-HEC error counter. |

| RDK parameter | BBF parameter | Mapping and differences |
| --- | --- | --- |
| `Device.X_RDK_ONT.Ploam.OnuId` | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUID` | Case/name change; both unsignedInt 0..1022. BBF assigns the ID to an ANI during OLT activation. |
| `Device.X_RDK_ONT.Ploam.VendorId` | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.VendorID` | string(128) -> string(4). BBF expects the ONU vendor ID, not a free-form vendor name. |
| `Device.X_RDK_ONT.Ploam.SerialNumber` | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.SerialNumber` | string(128) -> string(12). BBF explicitly combines Vendor-ID and vendor-specific serial number in human-readable form. RDK XML does not specify the format. |
| `Device.X_RDK_ONT.Ploam.RegistrationState` | `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUState` | RDK mapped O1..O9 (codes 0..8) -> BBF strings O1..O9 **plus `O2-3`**. Translate by state meaning; RDK has no combined O2-3 state. |
| `Device.X_RDK_ONT.Ploam.ActivationCounter` | None | RDK-only unsignedInt. No activation-count parameter inside BBF XPON. |
| `Device.X_RDK_ONT.Ploam.TxMessageCount` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.UpstreamMessageCount` | Candidate; unsignedInt -> StatsCounter64. BBF excludes Acknowledgement messages from its upstream count; RDK XML does not state that exclusion. |
| `Device.X_RDK_ONT.Ploam.RxMessageCount` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.DownstreamMessageCount` | Candidate; unsignedInt -> StatsCounter64. BBF counts received broadcast messages and messages directed to the ONU-ID; validate RDK filtering. |
| `Device.X_RDK_ONT.Ploam.MicErrors` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.MICErrors` | Case change; unsignedInt -> StatsCounter32. BBF counts received PLOAM messages with MIC errors. |
| `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO1`, `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO2` | None | RDK-only unsignedInt timers. BBF `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.RangingTime` is a message counter, **not** a registration timer or duration. |

| RDK parameter | BBF parameter | Mapping and differences |
| --- | --- | --- |
| `Device.X_RDK_ONT.Omci.RxBaseLineMessageCountValid` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.BaselineMessagesReceived` | Candidate rename; signed int -> unsigned StatsCounter64. RDK name qualifies the count as valid; verify accepted/error-message counting and any negative sentinels. |
| `Device.X_RDK_ONT.Omci.RxExtendedMessageCountValid` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.ExtendedMessagesReceived` | Candidate rename; signed int -> unsigned StatsCounter64. Same validity/sentinel caveat. |
| `Device.X_RDK_ONT.Omci.MicErrors` | `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.MICErrors` | Case change; unsignedInt -> StatsCounter32. BBF counts received OMCI messages with MIC errors. |

## GEM Ports and Ethernet UNI / VEIP

All parameters in these two tables are read-only in both models.

| RDK parameter | BBF parameter | Mapping and differences |
| --- | --- | --- |
| `Device.X_RDK_ONT.Gem.{i}.PortId` | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortID` | Case change; unsignedInt on both, but BBF constrains it to 0..65534. Port ID is not the table instance number. |
| `Device.X_RDK_ONT.Gem.{i}.TrafficType` | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.Direction` | RDK `Unidirectional(0), Bidirectional(1)` versus BBF `UNI-to-ANI, ANI-to-UNI, bidirectional`. Bidirectional can be translated; unidirectional lacks enough information to choose a BBF direction. |
| `Device.X_RDK_ONT.Gem.{i}.TransmittedFrames` | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesSent` | Candidate rename/move; unsignedInt -> StatsCounter64. BBF counts (X)GEM frames for this port, not Ethernet frames or ANI-wide totals. |
| `Device.X_RDK_ONT.Gem.{i}.ReceivedFrames` | `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesReceived` | Candidate rename/move; unsignedInt -> StatsCounter64. Preserve per-port scope and verify the counted frame layer. |

| RDK parameter | BBF parameter | Mapping and differences |
| --- | --- | --- |
| `Device.X_RDK_ONT.Veip.{i}.MeId` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainID` | Transform, not rename: unsignedInt -> string(256). For OMCI-managed VEIP service, BBF recommends `(VEIP,1025)`-style service-type/MEID formatting. The string also supports other service types. |
| `Device.X_RDK_ONT.Veip.{i}.AdministrativeState` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Enable` | RDK mapped `Lock(0), Unlock(1)` -> boolean: Lock is false; Unlock is true. BBF Enable reflects OLT-controlled service state and is read-only, unlike ANI Enable. BBF refers to OMCI administrative-state 0 as unlocked; do not confuse raw OMCI codes with the RDK enum codes. |
| `Device.X_RDK_ONT.Veip.{i}.OperationalState` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Status` | Rename; both enumerate `Up, Down, Unknown, Dormant, NotPresent, LowerLayerDown, Error`. BBF marks Error optional and says LowerLayerDown should never be used for this layer-1 interface. Confirm traffic-provisioning semantics, not merely numeric enum order. |
| `Device.X_RDK_ONT.Veip.{i}.InterDomainName` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainName` | Case change only in the path; both string(25). BBF explicitly refers to the associated VEIP ME's interdomain name. |
| `Device.X_RDK_ONT.Veip.{i}.InterfaceName` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Name` | Candidate rename; string(256) -> string(64). BBF specifies a CPE-assigned textual interface name. |

## RDK VLAN Flows and TR-69 Fields Without XPON Equivalents

Every parameter in this table is absent from the supplied BBF XPON subtree. This does not assert that VLAN or management-server functionality is absent elsewhere in TR-181; other BBF branches were not compared.

| RDK parameter(s) | Access / type | Difference |
| --- | --- | --- |
| `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.Tagged` | read-only mapped string | `Single(0), Double(1)`; no XPON ingress tagging selector. |
| `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Vid`, `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Pcp`, `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Dei` | read-only unsignedInt | No XPON ingress service-VLAN fields. RDK XML declares no VLAN-specific numeric ranges. |
| `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Vid`, `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Pcp`, `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Dei` | read-only unsignedInt | No XPON ingress customer-VLAN fields. |
| `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.Tagged` | read-only mapped string | `Single(0), Double(1)`; no XPON egress tagging selector. |
| `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Vid`, `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Pcp`, `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Dei` | read-only unsignedInt | No XPON egress service-VLAN fields. |
| `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Vid`, `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Pcp`, `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Dei` | read-only unsignedInt | No XPON egress customer-VLAN fields. |
| `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Tagged` | read-write mapped string | `Untagged(0), Single(1), Double(2)`; no XPON equivalent. Its numeric encoding differs from GEM Tagged. |
| `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Vid` | read-write unsignedInt | No XPON equivalent for the writable ingress VLAN ID. |
| `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Pcp`, `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Dei` | read-only unsignedInt | No XPON ingress priority/drop-eligibility fields. |
| `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Tagged` | read-write mapped string | `Untagged(0), Single(1), Double(2)`; no XPON equivalent. |
| `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Vid` | read-write unsignedInt | No XPON equivalent for the writable egress VLAN ID. |
| `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Pcp`, `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Dei` | read-only unsignedInt | No XPON egress priority/drop-eligibility fields. |
| `Device.X_RDK_ONT.TR69.url` | read-only string(256) | No XPON TR-69 URL parameter. Preserve the lowercase RDK spelling `url`. |
| `Device.X_RDK_ONT.TR69.AssociatedTag` | read-only unsignedInt | No corresponding XPON parameter. RDK XML does not define the tag's semantics. |

## Additional BBF Fields Without Direct RDK Equivalents

The BBF-only LODS alarm is listed in the alarm table. The fields below account for the remaining BBF additions or fields whose only RDK relationship is a non-equivalent candidate.

| BBF parameter(s) | Type / access | Difference from RDK |
| --- | --- | --- |
| `Device.XPON.ONUNumberOfEntries` | unsignedInt, read-only | Explicit ONU table count; no RDK ONU table. |
| `Device.XPON.ONU.{i}.Enable` | boolean, read-write | Whole-ONU enable/disable, including traffic suppression, notification suppression, and power reduction. Not the same as `WAN_MANAGER_UNIFICATION_ENABLED` physical-media Enable. |
| `Device.XPON.ONU.{i}.Name` | string(64), read-only | ONU identity, not a VEIP interface or transceiver part name. |
| `Device.XPON.ONU.{i}.Version` | string(14), read-only | ONU vendor-defined version; not established as equivalent to the RDK module revision. |
| `Device.XPON.ONU.{i}.EquipmentID` | string(20), read-only | ONU equipment/type identifier. |
| `Device.XPON.ONU.{i}.PowerCapability` | string enumeration list, read-only | Added in BBF 2.20: `On, Off`, optionally `LowPower`. |
| `Device.XPON.ONU.{i}.PowerStatus` | string, read-only | Added in BBF 2.20: current power state referencing PowerCapability, plus `Error`. |
| `Device.XPON.ONU.{i}.SoftwareImageNumberOfEntries`, `Device.XPON.ONU.{i}.EthernetUNINumberOfEntries`, `Device.XPON.ONU.{i}.ANINumberOfEntries` | unsignedInt, read-only | Explicit child-table counts. RDK table callbacks are not equivalent exposed parameters. |
| `Device.XPON.ONU.{i}.SoftwareImage.{i}.ID` | unsignedInt 0..1, read-only | Software-image identifier within the up-to-2-entry table. |
| `Device.XPON.ONU.{i}.SoftwareImage.{i}.Version` | string(14), read-only | Per-image version. RDK module firmware version is only a candidate source, not a complete software-image model. |
| `Device.XPON.ONU.{i}.SoftwareImage.{i}.IsCommitted`, `Device.XPON.ONU.{i}.SoftwareImage.{i}.IsActive`, `Device.XPON.ONU.{i}.SoftwareImage.{i}.IsValid` | boolean, read-only | Per-image commitment, active, and validity flags absent from RDK. |
| `Device.XPON.ONU.{i}.ANI.{i}.Name` | string(64), read-only | ANI name; no physical-media name field with this meaning in RDK. |
| `Device.XPON.ONU.{i}.ANI.{i}.TransceiverNumberOfEntries` | unsignedInt, read-only | Explicit transceiver count for an ANI. |
| `Device.XPON.ONU.{i}.ANI.{i}.SFPReferenceList` | list of strong path references; string items max 256, read-only | Added in BBF 2.19. References associated SFP cage rows. RDK Cage's BoB/SFP enum does not identify cage instances. The referenced external SFP tree is out of scope. |
| `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.ID` | unsignedInt 0..1, read-only | Explicit transceiver identifier, separate from table instance and SFF Identifier. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Alias` | _AliasCWMP, read-write | UNI alias; `WAN_MANAGER_UNIFICATION_ENABLED` physical-media Alias is at a different owner. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Description` | string(256), read-write | Added in BBF 2.21; user-friendly UNI description. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.LastChange` | unsignedInt, seconds, read-only | UNI state age, not ANI/physical-media state age. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.LowerLayers` | strong interface-reference list, max length 1024, read-write | UNI interface stacking; expected unused because this is a physical or virtualized layer-1 interface. Not a replacement for RDK physical-media LowerLayers. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.Upstream` | boolean, read-only | Added in BBF 2.19; direction of the UNI. Not the RDK physical-media flag. |
| `Device.XPON.ONU.{i}.EthernetUNI.{i}.ANIs` | strong ANI-reference list, max length 1024, read-only | Explicit UNI-to-ANI association. The list does not itself establish that forwarding is provisioned. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.Password` | secured string(72), read-write | Added in BBF 2.17. Password/registration-ID provisioning; effective ASCII/hex lengths depend on PON mode. No RDK XPON authentication field. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.HexadecimalPassword` | boolean, read-write | Selects ASCII versus hexadecimal password representation. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UnknownProfile` | StatsCounter32, read-only | Transmission blocked by an unknown burst profile. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesSent`, `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesReceived` | StatsCounter64, read-only | ANI-wide (X)GEM totals. RDK per-port counters are not direct replacements; summation requires checking inclusion and lifecycle rules. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.KeyErrors` | StatsCounter32, read-only | XGEM frames discarded for unknown/invalid encryption keys; included in the BBF received-frame count. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.RangingTime` | StatsCounter64, read-only | Counts Ranging_Time PLOAM messages; not elapsed time and not TO1/TO2. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.PortNumberOfEntries` | unsignedInt, read-only | Exposed GEM port count; RDK has only table enumeration callbacks. |
| `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortType` | string enum, read-only | `unicast, multicast, broadcast`; separate from GEM direction/TrafficType. |

### Throughput Statistics

RDK exposes neither of these throughput-statistics subtrees. These are interface byte/Ethernet-packet counters, not replacements for GEM frame counters. All counters below are read-only. BBF defines transmit as towards the PON and receive as from the PON for both objects, and specifies reset behavior on administrative enable/disable transitions in the object descriptions.

| BBF ANI parameter | BBF EthernetUNI parameter | Type / difference |
| --- | --- | --- |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesSent` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesSent` | StatsCounter64; transmitted bytes including framing characters. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesReceived` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesReceived` | StatsCounter64; received bytes including framing characters. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsSent` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsSent` | StatsCounter64; transmitted packets/Ethernet frames. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsReceived` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsReceived` | StatsCounter64; received packets/Ethernet frames. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsSent` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsSent` | StatsCounter32; outbound packet/frame errors. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsReceived` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsReceived` | StatsCounter32; inbound packet/frame errors. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsSent` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsSent` | StatsCounter64; unicast transmission requests, including discarded/not sent. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsReceived` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsReceived` | StatsCounter64; delivered/forwarded received unicast frames. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsSent` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsSent` | StatsCounter32; outbound discards without transmission errors. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsReceived` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsReceived` | StatsCounter32; inbound discards without reception errors. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsSent` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsSent` | StatsCounter64; multicast transmission requests. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsReceived` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsReceived` | StatsCounter64; delivered/forwarded received multicast frames. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsSent` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsSent` | StatsCounter64; broadcast transmission requests. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsReceived` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsReceived` | StatsCounter64; delivered/forwarded received broadcast frames. |
| `Device.XPON.ONU.{i}.ANI.{i}.Stats.UnknownProtoPacketsReceived` | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnknownProtoPacketsReceived` | StatsCounter32; discards due to unknown/unsupported protocol. |
| None | `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.Reset` | Boolean, read-write command, added in BBF 2.19; set true to reset the UNI counters. There is no corresponding ANI Stats.Reset in this XML. |

### ONU Power Management

The entire `Device.XPON.ONU.{i}.PowerManagement.` object is additional in BBF 2.20 and has no RDK counterpart. All its parameters are read-only.

| BBF parameter | Type / meaning | Difference |
| --- | --- | --- |
| `Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementCapability` | Enum list: `WatchfulSleepMode, DozeMode, CyclicSleepMode` | Reports supported sleep modes; empty means none. |
| `Device.XPON.ONU.{i}.PowerManagement.OLTSleepAllow` | boolean | Indicates OLT permission for power reduction. |
| `Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementStatus` | Enum: `Active, WatchfulSleepMode, DozeMode, CyclicSleepMode` | Reports the current power-reduction state. |
| `Device.XPON.ONU.{i}.PowerManagement.ITransInit` | unsignedInt | Complete transceiver initialization time, in 125-microsecond PHY frames. |
| `Device.XPON.ONU.{i}.PowerManagement.ITxInit` | unsignedInt | Transmitter initialization time, in 125-microsecond PHY frames. |
| `Device.XPON.ONU.{i}.PowerManagement.MaxSleepInterval` | unsignedInt | Maximum asleep/listen/low-power interval, in 125-microsecond PHY frames. |
| `Device.XPON.ONU.{i}.PowerManagement.MinAwareInterval` | unsignedInt | Minimum aware interval, in 125-microsecond PHY frames. |
| `Device.XPON.ONU.{i}.PowerManagement.MinActiveHeldInterval` | unsignedInt | Minimum active-held interval, in 125-microsecond PHY frames. |
| `Device.XPON.ONU.{i}.PowerManagement.MaxSleepIntervalExtension` | unsignedLong | Encodes separate 4-byte doze and cyclic-sleep interval values in 125-microsecond PHY-frame units; not just a wider scalar interval. Overrides MaxSleepInterval when supported; ignored for watchful sleep. |

## Main Migration Implications

1. Establish ONU, ANI, transceiver, GEM-port, and UNI instance relationships before remapping the flatter RDK tree.
2. Translate alarm booleans and PON/status/activation/direction enums explicitly. New BBF values cannot always be derived from current RDK values.
3. Source genuinely 64-bit counters where BBF requires them. Casting a 32-bit value does not recover previously lost wrap information; confirm counter scope, validity, and reset behavior.
4. Confirm optical units, signedness, invalid-reading values, string lengths, and ambiguous HEC/module mappings in the backend before implementation.
5. Keep RDK-only VLAN controls, optical thresholds, redundancy state, registration timers, activation count, and TR-69 fields as separate extensions if still needed; they cannot be represented by a simple XPON path rename.

Validation: paths and coverage are checked against XML-parsed inventories of the two scoped trees. No other BBF object tree, source implementation, or runtime behavior is evaluated by this document.