# 10 — Potential HAL Schema Extensions (based on the Broadcom HAL)

This document proposes additions to the RDK unified GPON HAL schema. Each one would let GPON Manager support more BBF `Device.XPON` parameters, or remove a best-match/deviation listed in [09_XPON_TR181_BestMatch_Deviations.md](09_XPON_TR181_BestMatch_Deviations.md).

**Inputs compared**

| Input | Location |
|---|---|
| RDK unified HAL schema | `hal_schema/gpon_wan_unify_hal_schema.json` |
| Broadcom vendor data model (BCM SDK 6.3) | `soc/broadcom/bcm-sdk-6.3/data-model/cms-dm-rdk-gpon.xml` |
| BBF TR-181 2.21 | `tr-181-2-21-0-cwmp-full.xml` |
| Unsupported `Device.XPON` parameters | [08_TR181_Telemetry_Reference.md](08_TR181_Telemetry_Reference.md), Appendix A |

**Result of the comparison:** the Broadcom DM defines 128 `X_RDK_ONT` parameters. Most are already in the RDK schema. The ones that are not fall into three tiers:

| Tier | Meaning | Effort |
|---|---|---|
| **Tier 1** | The Broadcom HAL already **sends** the value to GPON Manager, but GPON Manager has no DML field for it and **discards** it (`gponmgr_dml_hal_param.c`, `AllocId`/`Type`/`PayloadBytes`/`EncryptionKeyErrors` branch). | Schema entry + GPON Manager only |
| **Tier 2** | In the Broadcom DM, but not in the RDK schema and not used by GPON Manager. | Schema entry + confirm the Broadcom HAL server serves it + GPON Manager |
| **Tier 3** | In neither the Broadcom DM nor the RDK schema. | New vendor HAL work |

## Summary

| ID | Proposed HAL parameter | Tier | `Device.XPON` gain |
|---|---|---|---|
| EXT-1 | `Gem.{i}.Type` | 1 | `TC.GEM.Port.{i}.PortType` becomes exact (adds `broadcast`); `Direction` can report `ANI-to-UNI` |
| EXT-2 | `Gem.{i}.EncryptionKeyErrors` | 1 | `TC.PM.GEM.KeyErrors` (Appendix A → supported) |
| EXT-3 | `Gem.{i}.TransmittedPayloadBytes` / `ReceivedPayloadBytes` | 1 | Candidate best-match source for `ANI.{i}.Stats.BytesSent` / `BytesReceived` |
| EXT-4 | `Gem.{i}.AllocId` | 1 | None in BBF 2.21; `X_RDK_ONT` diagnostics only |
| EXT-5 | `PhysicalMedia.{i}.Alarm.LODS` | 2 | `TC.Alarms.LODS` (Appendix A → supported) |
| EXT-6 | `GponVeipStats.{i}.*` | 2 | Six `EthernetUNI.{i}.Stats` counters (Appendix A → supported); framing-inclusive byte counts |
| EXT-7 | `WanLinkStatus.{i}.*`, `TcLayerStatus.{i}.*` | 2 | None; `X_RDK_ONT` diagnostics only |
| EXT-8 | `TR69.ManagedEntityId`, `AdministrativeState`, `AcsNetworkAddress` | 2 | None; `X_RDK_ONT` diagnostics only |
| EXT-9 | `PhysicalMedia.{i}.VendorPartNumber` / `VendorRevision` (SFF-8472) | 3 | `Transceiver.{i}.VendorPartNumber` / `VendorRevision` become exact |
| EXT-10 | `Gem.{i}.Direction` (G.988) | 3 | `TC.GEM.Port.{i}.Direction` fully BBF-compliant |
| EXT-11 | ONU `Version` / `EquipmentID` (OMCI ONU-G / ONU2-G) | 3 | `ONU.{i}.Version`, `ONU.{i}.EquipmentID` |
| EXT-12 | `SoftwareImage.{i}.*` (OMCI Software image) | 3 | `ONU.{i}.SoftwareImage.{i}.*` (5 parameters) |
| EXT-13 | `PowerManagement.*` (OMCI dynamic power management) | 3 | `ONU.{i}.PowerManagement.*` (9) + `PowerCapability` / `PowerStatus` |
| EXT-14 | `Ploam.RangingTime` | 3 | `TC.PM.PLOAM.RangingTime` |
| EXT-15 | `Gtc.UnknownProfile` | 3 | `TC.PM.PHY.UnknownProfile` |
| EXT-16 | ANI-level GEM / Ethernet counters | 3 | Remaining `ANI.{i}.Stats.*`; monotonic `TC.PM.GEM.FramesSent` / `FramesReceived` |

Not recommended: `PhysicalMedia.{i}.X_BROADCOM_COM_LastChange` is a vendor-private, hidden helper. GPON Manager already computes `ANI.LastChange` itself.

---

## Tier 1 — Already sent by the Broadcom HAL, discarded by GPON Manager

GPON Manager currently accepts these values from the HAL and drops them:

```c
/* source/TR-181/middle_layer_src/gponmgr_dml_hal_param.c */
else if (strstr(ParamName, "AllocId") || strstr(ParamName, "Type") ||
         strstr(ParamName, "PayloadBytes") || strstr(ParamName, "EncryptionKeyErrors"))
{
    /* Reported by the unified HAL; no DML field yet. */
    retStatus = ANSC_STATUS_SUCCESS;
}
```

### EXT-1 `Device.X_RDK_ONT.Gem.{i}.Type`

| | |
|---|---|
| **Broadcom** | string, read-only, default `Unicast`, values `Unicast` / `Multicast` / `Broadcast` ("GEM port type, unicast, multicast, or broadcast"). |
| **BBF target** | `TC.GEM.Port.{i}.PortType` (`unicast` / `multicast` / `broadcast`), as a direct lower-case mapping. |
| **Side benefit** | `TC.GEM.Port.{i}.Direction`: a Unidirectional port of type Multicast or Broadcast is downstream-only, so it can be reported as `ANI-to-UNI` instead of the non-BBF `unidirectional`. |
| **Closes** | Deviations 09 §1.5 and, for multicast/broadcast ports, §2.1. |
| **GPON Manager work** | Add `Type` to `DML_GEM`. Parse it in the Gem branch. Switch `xpon_gem_get_handler` from the `TrafficType` heuristic to `Type`, keeping the heuristic as a fallback when the HAL does not report `Type`. Add `X_RDK_ONT.Gem.{i}.Type` to `RdkGponManager.xml` if it should also be visible there. |

Proposed schema entry (same style as `ontGemTrafficType`):

```json
"ontGemType": {
  "description": "GEM port type (Access = Read-Only)",
  "type": "object",
  "additionalProperties": false,
  "properties": {
    "name":  { "type": "string", "pattern": "^Device\\.X_RDK_ONT\\.Gem\\.\\d+\\.Type$" },
    "type":  { "type": "string", "const": "string" },
    "value": { "type": "string", "enum": ["Unicast", "Multicast", "Broadcast"] }
  }
}
```

### EXT-2 `Device.X_RDK_ONT.Gem.{i}.EncryptionKeyErrors`

| | |
|---|---|
| **Broadcom** | unsignedInt, read-only, per GEM port. |
| **BBF target** | `TC.PM.GEM.KeyErrors` (StatsCounter32): "XGEM key errors. XGEM frames discarded because of unknown or invalid encryption key." |
| **Mapping** | Σ over all GEM ports, the same way as `TC.PM.GEM.FramesSent`. Same caveat as 09 §2.3: the total drops if a port is removed. |
| **GPON Manager work** | Add the field to `DML_GEM`, parse it, and move `KeyErrors` from the unsupported handler to the GEM PM handler. |

### EXT-3 `Device.X_RDK_ONT.Gem.{i}.TransmittedPayloadBytes` / `ReceivedPayloadBytes`

| | |
|---|---|
| **Broadcom** | unsignedLong, read-only, per GEM port. |
| **BBF target** | No per-port byte counter in BBF 2.21. Candidate **best match** for `ANI.{i}.Stats.BytesSent` / `BytesReceived` as Σ over the GEM ports. |
| **Caveat** | Payload bytes exclude GEM/XGEM headers and PHY framing, while BBF `Stats.Bytes*` include framing. That would be a documented best match. |
| **Recommendation** | Expose them as `X_RDK_ONT.Gem.{i}.*PayloadBytes` first. Map them to `ANI.Stats` only if product agrees that a best match is acceptable. |

### EXT-4 `Device.X_RDK_ONT.Gem.{i}.AllocId`

| | |
|---|---|
| **Broadcom** | unsignedInt 0–65535, read-only. |
| **BBF target** | None (BBF 2.21 `TC.GEM.Port.{i}` has only `PortID`, `Direction`, `PortType`). |
| **Use** | `X_RDK_ONT` diagnostics: which T-CONT a GEM port belongs to. Useful for QoS troubleshooting. |

---

## Tier 2 — In the Broadcom DM, missing from the RDK schema

For each item, first confirm that the Broadcom JSON-HAL server actually answers `getParameters` for the path, then add it to the schema.

### EXT-5 `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.LODS`

| | |
|---|---|
| **Broadcom** | Present next to the other 14 alarms. |
| **BBF target** | `TC.Alarms.LODS` (boolean). Currently Appendix A. |
| **GPON Manager work** | Add a schema entry and `subscribeEventSupportedList` entry. Add `LODS` back to the alarm parser, the RBUS handler and the `XPON_ALARM_LODS` telemetry event. GPON Manager removed it earlier because the RDK schema did not define it. |

### EXT-6 `Device.X_RDK_ONT.GponVeipStats.{i}.*` (G.988 §9.3.34, Ethernet frame extended PM)

Broadcom provides one row per VEIP **and direction** (`Direction` = `upstream` / `downstream`), with `IfName`, `DropEvents`, `Octets`, `Frames`, `BroadcastFrames`, `MulticastFrames`, `CrcErroredFrames`, `UndersizeFrames`, `OversizeFrames` and the `Frames64Octets` … `Frames1024to1518Octets` size bins. All counters are unsignedLong.

| BBF `EthernetUNI.{i}.Stats.*` | Proposed source | Status today |
|---|---|---|
| `UnicastPacketsSent` / `UnicastPacketsReceived` | `Frames − BroadcastFrames − MulticastFrames` | Appendix A |
| `MulticastPacketsSent` | `MulticastFrames` (upstream row) | Appendix A |
| `BroadcastPacketsSent` / `BroadcastPacketsReceived` | `BroadcastFrames` | Appendix A |
| `BytesSent` / `BytesReceived` | `Octets` (if it includes the FCS, it is closer to BBF than Linux sysfs) | Linux sysfs (09 §2.4) |
| `ErrorsReceived` | `CrcErroredFrames` (+ `UndersizeFrames` + `OversizeFrames`) | Linux sysfs |
| `DiscardPacketsReceived` | `DropEvents` | Linux sysfs |
| `UnknownProtoPacketsReceived` | No source | Stays in Appendix A |
| `Reset` | No source (G.988 PM is reset by OMCI, not by the ONU host) | Stays in Appendix A |

**Decisions needed before implementing:**

1. **Direction convention.** The current sysfs mapping takes the gateway's view: `Sent` = upstream (`tx_bytes` on the VEIP netdev). The G.988 counters are measured on the ONU, so the `upstream` row must map to `*Sent` to keep the same convention.
2. **Mixing sources.** Either keep Linux sysfs for the counters it already serves and use `GponVeipStats` only for the six missing ones, or move the whole `EthernetUNI.Stats` object to `GponVeipStats` so that all counters come from one source.

### EXT-7 `Device.X_RDK_ONT.WanLinkStatus.{i}.*` and `TcLayerStatus.{i}.*`

| | |
|---|---|
| **Broadcom** | `WanLinkStatus.{i}`: `IfName`, `TcStatus`, `OmciStatus`. `TcLayerStatus.{i}`: `IfType`, `OnuState` (1–9), `OnuId`, `SerialNumber`. |
| **BBF target** | None new. `TcLayerStatus` duplicates `Ploam.*` and `PhysicalMedia.PonMode`, which are already mapped. |
| **Use** | `WanLinkStatus.OmciStatus` would let GPON Manager tell "PON up but OMCI not provisioned" apart from "PON down". That helps the WAN state machine and telemetry, but adds no `Device.XPON` parameter. **Low priority.** |

### EXT-8 `Device.X_RDK_ONT.TR69.ManagedEntityId` / `AdministrativeState` / `AcsNetworkAddress`

| | |
|---|---|
| **Broadcom** | Present. The RDK schema has only `TR69.url` and `TR69.AssociatedTag`. |
| **BBF target** | None in `Device.XPON`. |
| **Use** | `X_RDK_ONT` diagnostics for OMCI-provisioned TR-069 (G.988 TR-069 management server ME). **Low priority.** |

---

## Tier 3 — Not available in the Broadcom HAL (new vendor work)

| ID | Proposed HAL parameter | Standard source | BBF target |
|---|---|---|---|
| EXT-9 | `PhysicalMedia.{i}.VendorPartNumber` (string 16), `PhysicalMedia.{i}.VendorRevision` (string 4) | SFF-8472 A0h bytes 40–55 (Vendor PN) and 56–59 (Vendor rev) | `Transceiver.{i}.VendorPartNumber` / `VendorRevision` become exact. `ModuleName` / `ModuleVersion` stay as model name / hardware version. |
| EXT-10 | `Gem.{i}.Direction` (`UNI-to-ANI` / `ANI-to-UNI` / `bidirectional`) | G.988 §9.2.3 GEM port network CTP, *Direction* attribute (1 / 2 / 3) | `TC.GEM.Port.{i}.Direction` without inference. Better than deriving it from EXT-1. |
| EXT-11 | `Onu.Version` (string 14), `Onu.EquipmentId` (string 20) | G.988 §9.1.1 ONU-G *Version*; §9.1.2 ONU2-G *Equipment id* | `ONU.{i}.Version`, `ONU.{i}.EquipmentID` |
| EXT-12 | `SoftwareImage.{i}.Id`, `Version`, `IsCommitted`, `IsActive`, `IsValid` | G.988 §9.1.4 Software image ME (two instances) | `ONU.{i}.SoftwareImage.{i}.*`; `SoftwareImageNumberOfEntries` becomes 2 |
| EXT-13 | `PowerManagement.*` | G.988 §9.1.14 ONU dynamic power management control ME | `ONU.{i}.PowerManagement.*` (9), `ONU.{i}.PowerCapability`, `ONU.{i}.PowerStatus` |
| EXT-14 | `Ploam.RangingTime` | Equalisation delay assigned in the PLOAM *Ranging_Time* message | `TC.PM.PLOAM.RangingTime` |
| EXT-15 | `Gtc.UnknownProfile` | XG(S)-PON PHY PM, unknown burst-profile counter | `TC.PM.PHY.UnknownProfile` |
| EXT-16 | ANI-level `Gtc.GemFramesSent` / `GemFramesReceived` and Ethernet counters over all GEM ports | G.988 §9.2.13 GEM port network CTP PM, aggregated by the vendor | Monotonic `TC.PM.GEM.FramesSent` / `FramesReceived` (closes 09 §2.3); `ANI.{i}.Stats.*` |

`TC.Authentication.Password` / `HexadecimalPassword` (the PLOAM registration ID) are not proposed as HAL **read** parameters, because exposing the password over TR-181 is a security concern. If needed, they should be a write-only HAL `setParameters` path that GPON Manager stores securely.

---

## Suggested Order

1. **EXT-1, EXT-2.** Tier 1, so only GPON Manager and the schema change. This removes the `PortType` heuristic, most of the `Direction` deviation and one Appendix A item.
2. **EXT-5.** Small. Brings back `TC.Alarms.LODS` and its telemetry event.
3. **EXT-6.** Six more `EthernetUNI.Stats` counters. Needs the direction decision first.
4. **EXT-9, EXT-10, EXT-11.** Vendor requests that make the transceiver, GEM and ONU identity values exact.
5. **EXT-12 – EXT-16.** Larger vendor work, based on product need.
