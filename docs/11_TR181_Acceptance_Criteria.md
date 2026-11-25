# GPON Manager — TR-181 Data Model Acceptance Criteria

User story: GPON Manager exposes the GPON/XGS-PON ONU state, statistics and configuration through the BBF TR-181 `Device.XPON` data model and the RDK `Device.X_RDK_ONT` extension.

**MoSCoW keys:** **MUST**: required for acceptance. **SHOULD**: expected, but a documented exception is acceptable. **COULD**: optional. **WON'T**: explicitly out of scope for this story.

**Scope:** only the supported parameters are covered: 89 BBF `Device.XPON` parameters and 35 of the 37 `Device.X_RDK_ONT` parameters that have no `Device.XPON` equivalent. The other 58 `Device.X_RDK_ONT` parameters carry the same data as a `Device.XPON` parameter and stay available as they are today. The `Device.X_RDK_ONT.TR69` parameters are not part of this story. Unsupported `Device.XPON` parameters are out of scope.

**Field meanings:** *Factory Default Value* is the value GPON Manager reports before the HAL provides data.

---

1. GPON Manager MUST implement the BBF TR-181 `Device.XPON` data model and the RDK `Device.X_RDK_ONT` extension as described in the GPON Manager TR-181 & Telemetry Reference (`docs/08_TR181_Telemetry_Reference.md`).

1.1. GPON Manager MUST register the complete BBF TR-181 2.21 `Device.XPON` tree as an RBUS data provider (`GponManager.XPON`) at start-up.  
1.1.1. Registration failure MUST NOT stop GPON Manager: `Device.X_RDK_ONT` MUST stay available and `XPON_ERROR_RBUS_REGISTER_FAILED` MUST be raised.  

1.2. GPON Manager MUST read the vendor HAL through the JSON-HAL client and cache each HAL table for at most `XPON_CACHE_TTL_SEC` (default 30 s).  
1.2.1. A HAL push event (PhysicalMedia status or alarm, VEIP state, ONU state) MUST invalidate the cache, so the next get returns the new value.  

1.3. GPON Manager WON'T allow writes to the `Device.XPON` parameters that BBF defines as writable. `ONU.{i}.Enable`, `ANI.{i}.Enable`, `ANI.{i}.Alias`, `EthernetUNI.{i}.Alias`, `EthernetUNI.{i}.Description` and `EthernetUNI.{i}.LowerLayers` are read-only, and a set MUST return `RBUS_ERROR_NOT_WRITABLE`.  

2. GPON Manager MUST implement the following BBF `Device.XPON` parameters: XPON Root, ONU & EthernetUNI.

2.1. GPON Manager MUST report `ONUNumberOfEntries`: number of entries in the ONU table.  
2.1.1. Data Model Name: `Device.XPON.ONUNumberOfEntries`  
2.1.2. Type: unsignedInt  
2.1.3. Access: Read  
2.1.4. Persistence: No  
2.1.5. Factory Default Value: 1  
2.1.6. Valid Range: Fixed value  
2.1.7. Example: 1  
2.1.8. Usage: Number of entries in the ONU table. Always 1.  

2.2. GPON Manager MUST report `SoftwareImageNumberOfEntries`: number of entries in the SoftwareImage table.  
2.2.1. Data Model Name: `Device.XPON.ONU.{i}.SoftwareImageNumberOfEntries`  
2.2.2. Type: unsignedInt  
2.2.3. Access: Read  
2.2.4. Persistence: No  
2.2.5. Factory Default Value: 0  
2.2.6. Valid Range: Fixed value  
2.2.7. Example: 0  
2.2.8. Usage: Number of entries in the SoftwareImage table. Always 0 — no software-image rows are registered.  

2.3. GPON Manager MUST report `EthernetUNINumberOfEntries`: number of entries in the EthernetUNI table.  
2.3.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNINumberOfEntries`  
2.3.2. Type: unsignedInt  
2.3.3. Access: Read  
2.3.4. Persistence: No  
2.3.5. Factory Default Value: 0  
2.3.6. Valid Range: 0 – 2  
2.3.7. Example: 1  
2.3.8. Usage: Number of entries in the EthernetUNI table. Number of VEIP interfaces.  

2.4. GPON Manager MUST report `ANINumberOfEntries`: number of entries in the ANI table.  
2.4.1. Data Model Name: `Device.XPON.ONU.{i}.ANINumberOfEntries`  
2.4.2. Type: unsignedInt  
2.4.3. Access: Read  
2.4.4. Persistence: No  
2.4.5. Factory Default Value: 1  
2.4.6. Valid Range: Fixed value  
2.4.7. Example: 1  
2.4.8. Usage: Number of entries in the ANI table. Always 1.  

2.5. GPON Manager MUST report `Enable`: enables or disables this ONU.  
2.5.1. Data Model Name: `Device.XPON.ONU.{i}.Enable`  
2.5.2. Type: boolean  
2.5.3. Access: Read (BBF defines Read/Write; a set returns RBUS_ERROR_NOT_WRITABLE)  
2.5.4. Persistence: No  
2.5.5. Factory Default Value: false (until the JSON-HAL client connects)  
2.5.6. Valid Range: true | false  
2.5.7. Example: true  
2.5.8. Usage: Enables or disables this ONU. Read-only (BBF: Read/Write). true while GPON Manager is connected to the PON stack, otherwise false.  

2.6. GPON Manager MUST report `Name`: the textual name of the ONU as assigned by the CPE.  
2.6.1. Data Model Name: `Device.XPON.ONU.{i}.Name`  
2.6.2. Type: string  
2.6.3. Access: Read  
2.6.4. Persistence: No  
2.6.5. Factory Default Value: ONU1  
2.6.6. Valid Range: Fixed value  
2.6.7. Example: ONU1  
2.6.8. Usage: The textual name of the ONU as assigned by the CPE. Always ONU<i>.  

2.7. GPON Manager MUST report `Enable`: indicates whether this interface is enabled or not.  
2.7.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Enable`  
2.7.2. Type: boolean  
2.7.3. Access: Read  
2.7.4. Persistence: No  
2.7.5. Factory Default Value: false (VEIP AdministrativeState Lock)  
2.7.6. Valid Range: true (Unlock) | false (Lock)  
2.7.7. Example: true  
2.7.8. Usage: Indicates whether this interface is enabled or not. Unlock → true, Lock → false. Telemetry: Every change raises XPON_INTF_ADMIN_LOCK / _UNLOCK.  

2.8. GPON Manager MUST report `Status`: the current operational state of the interface.  
2.8.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Status`  
2.8.2. Type: string  
2.8.3. Access: Read  
2.8.4. Persistence: No  
2.8.5. Factory Default Value: Unknown  
2.8.6. Valid Range: Up, Down, Unknown, Dormant, NotPresent, LowerLayerDown, Error  
2.8.7. Example: Up  
2.8.8. Usage: The current operational state of the interface. Telemetry: Up/Down changes raise the XPON_INTF_LINK_* / XPON_PHY_STATUS_* events.  

2.9. GPON Manager MUST report `Name`: the textual name of the interface as assigned by the CPE.  
2.9.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Name`  
2.9.2. Type: string  
2.9.3. Access: Read  
2.9.4. Persistence: No  
2.9.5. Factory Default Value: Empty string  
2.9.6. Valid Range: ≤ 64 chars  
2.9.7. Example: veip0  
2.9.8. Usage: The textual name of the interface as assigned by the CPE. Telemetry: Carried as Interface=<name> in XPON_INTF_LINK_*.  

2.10. GPON Manager MUST report `ANIs`: references all associated ANI interfaces.  
2.10.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.ANIs`  
2.10.2. Type: list<string>  
2.10.3. Access: Read  
2.10.4. Persistence: No  
2.10.5. Factory Default Value: Device.XPON.ONU.1.ANI.1  
2.10.6. Valid Range: Fixed value  
2.10.7. Example: Device.XPON.ONU.1.ANI.1  
2.10.8. Usage: References all associated ANI interfaces. Always Device.XPON.ONU.1.ANI.1.  

2.11. GPON Manager MUST report `InterdomainID`: string to identify the associated service in the ONU management domain to the TR-181 management domain.  
2.11.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainID`  
2.11.2. Type: string  
2.11.3. Access: Read  
2.11.4. Persistence: No  
2.11.5. Factory Default Value: (VEIP,0)  
2.11.6. Valid Range: ≤ 256 chars, formatted (VEIP,<MeId>)  
2.11.7. Example: (VEIP,1025)  
2.11.8. Usage: String to identify the associated service in the ONU management domain to the TR-181 management domain. Formatted as (VEIP,<MeId>).  

2.12. GPON Manager MUST report `InterdomainName`: string that provides an additional way to identify the associated service to the TR-181 management domain.  
2.12.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainName`  
2.12.2. Type: string  
2.12.3. Access: Read  
2.12.4. Persistence: No  
2.12.5. Factory Default Value: Empty string  
2.12.6. Valid Range: ≤ 25 chars  
2.12.7. Example: VEIP_1  
2.12.8. Usage: String that provides an additional way to identify the associated service to the TR-181 management domain.  

2.13. GPON Manager MUST report `Alias`: unique, non-volatile alias for this instance.  
2.13.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Alias`  
2.13.2. Type: _AliasCWMP  
2.13.3. Access: Read (BBF defines Read/Write; a set returns RBUS_ERROR_NOT_WRITABLE)  
2.13.4. Persistence: No  
2.13.5. Factory Default Value: cpe-VEIP<i> (VEIP interface name not yet known)  
2.13.6. Valid Range: Maximum string length: 64 bytes  
2.13.7. Example: cpe-VEIP0  
2.13.8. Usage: Unique, non-volatile alias for this instance. cpe- + upper-case VEIP interface name; cpe-VEIP<i> if the name is empty. Read-only (BBF: Read/Write).  

2.14. GPON Manager MUST report `Description`: a user-friendly description of the interface.  
2.14.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Description`  
2.14.2. Type: string  
2.14.3. Access: Read (BBF defines Read/Write; a set returns RBUS_ERROR_NOT_WRITABLE)  
2.14.4. Persistence: No  
2.14.5. Factory Default Value: GPON VEIP WAN interface cpe-VEIP<i>  
2.14.6. Valid Range: Maximum string length: 256 bytes  
2.14.7. Example: GPON VEIP WAN interface cpe-VEIP0  
2.14.8. Usage: A user-friendly description of the interface. GPON VEIP WAN interface <Alias>. Read-only (BBF: Read/Write).  

2.15. GPON Manager MUST report `LastChange`: the accumulated time in seconds since the interface entered its current operational state.  
2.15.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.LastChange`  
2.15.2. Type: unsignedInt  
2.15.3. Access: Read  
2.15.4. Persistence: No  
2.15.5. Factory Default Value: 0  
2.15.6. Valid Range: Seconds, 0 – 2^32−1  
2.15.7. Example: 3600  
2.15.8. Usage: The accumulated time in seconds since the interface entered its current operational state. Seconds since Status last changed, or since the row was created. Starts again from 0 when GPON Manager restarts.  

2.16. GPON Manager MUST report `LowerLayers`: comma-separated list of references to the interface objects stacked immediately below this one.  
2.16.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.LowerLayers`  
2.16.2. Type: list<string>  
2.16.3. Access: Read (BBF defines Read/Write; a set returns RBUS_ERROR_NOT_WRITABLE)  
2.16.4. Persistence: No  
2.16.5. Factory Default Value: Empty string  
2.16.6. Valid Range: Fixed value  
2.16.7. Example: Empty string  
2.16.8. Usage: Comma-separated list of references to the interface objects stacked immediately below this one. Always empty: BBF expects LowerLayers not to be used for EthernetUNI. Read-only (BBF: Read/Write).  

2.17. GPON Manager MUST report `Upstream`: indicates whether the interface points towards the Internet (true) or towards End Devices (false).  
2.17.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Upstream`  
2.17.2. Type: boolean  
2.17.3. Access: Read  
2.17.4. Persistence: No  
2.17.5. Factory Default Value: true  
2.17.6. Valid Range: Fixed value  
2.17.7. Example: true  
2.17.8. Usage: Indicates whether the interface points towards the Internet (true) or towards End Devices (false). Always true: the VEIP is the gateway WAN interface.  

2.18. GPON Manager MUST report `BytesSent`: the total number of bytes transmitted out of the interface, including framing characters.  
2.18.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesSent`  
2.18.2. Type: StatsCounter64  
2.18.3. Access: Read  
2.18.4. Persistence: No  
2.18.5. Factory Default Value: None. The get is rejected until the VEIP interface exists in Linux  
2.18.6. Valid Range: 0 – 2^64−1 (cumulative)  
2.18.7. Example: 123456789  
2.18.8. Usage: The total number of bytes transmitted out of the interface, including framing characters. Linux counters exclude the Ethernet preamble and FCS, so values are slightly lower than a framing-inclusive count.  

2.19. GPON Manager MUST report `BytesReceived`: the total number of bytes received on the interface, including framing characters.  
2.19.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesReceived`  
2.19.2. Type: StatsCounter64  
2.19.3. Access: Read  
2.19.4. Persistence: No  
2.19.5. Factory Default Value: None. The get is rejected until the VEIP interface exists in Linux  
2.19.6. Valid Range: 0 – 2^64−1 (cumulative)  
2.19.7. Example: 987654321  
2.19.8. Usage: The total number of bytes received on the interface, including framing characters. Linux counters exclude the Ethernet preamble and FCS, so values are slightly lower than a framing-inclusive count.  

2.20. GPON Manager MUST report `PacketsSent`: the total number of packets transmitted out of the interface.  
2.20.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsSent`  
2.20.2. Type: StatsCounter64  
2.20.3. Access: Read  
2.20.4. Persistence: No  
2.20.5. Factory Default Value: None. The get is rejected until the VEIP interface exists in Linux  
2.20.6. Valid Range: 0 – 2^64−1 (cumulative)  
2.20.7. Example: 120000  
2.20.8. Usage: The total number of packets transmitted out of the interface.  

2.21. GPON Manager MUST report `PacketsReceived`: the total number of packets received on the interface.  
2.21.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsReceived`  
2.21.2. Type: StatsCounter64  
2.21.3. Access: Read  
2.21.4. Persistence: No  
2.21.5. Factory Default Value: None. The get is rejected until the VEIP interface exists in Linux  
2.21.6. Valid Range: 0 – 2^64−1 (cumulative)  
2.21.7. Example: 450000  
2.21.8. Usage: The total number of packets received on the interface.  

2.22. GPON Manager MUST report `ErrorsSent`: the total number of outbound packets that could not be transmitted because of errors.  
2.22.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsSent`  
2.22.2. Type: StatsCounter32  
2.22.3. Access: Read  
2.22.4. Persistence: No  
2.22.5. Factory Default Value: None. The get is rejected until the VEIP interface exists in Linux  
2.22.6. Valid Range: 0 – 2^32−1 (cumulative, wraps)  
2.22.7. Example: 0  
2.22.8. Usage: The total number of outbound packets that could not be transmitted because of errors. 32-bit counter; wraps.  

2.23. GPON Manager MUST report `ErrorsReceived`: the total number of inbound packets that contained errors preventing them from being delivered to a higher-layer protocol.  
2.23.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsReceived`  
2.23.2. Type: StatsCounter32  
2.23.3. Access: Read  
2.23.4. Persistence: No  
2.23.5. Factory Default Value: None. The get is rejected until the VEIP interface exists in Linux  
2.23.6. Valid Range: 0 – 2^32−1 (cumulative, wraps)  
2.23.7. Example: 0  
2.23.8. Usage: The total number of inbound packets that contained errors preventing them from being delivered to a higher-layer protocol. 32-bit counter; wraps.  

2.24. GPON Manager MUST report `DiscardPacketsSent`: the total number of outbound packets which were chosen to be discarded even though no errors had been detected to prevent their being transmitted.  
2.24.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsSent`  
2.24.2. Type: StatsCounter32  
2.24.3. Access: Read  
2.24.4. Persistence: No  
2.24.5. Factory Default Value: None. The get is rejected until the VEIP interface exists in Linux  
2.24.6. Valid Range: 0 – 2^32−1 (cumulative, wraps)  
2.24.7. Example: 0  
2.24.8. Usage: The total number of outbound packets which were chosen to be discarded even though no errors had been detected to prevent their being transmitted. 32-bit counter; wraps.  

2.25. GPON Manager MUST report `DiscardPacketsReceived`: the total number of inbound packets which were chosen to be discarded even though no errors had been detected to prevent their being delivered.  
2.25.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsReceived`  
2.25.2. Type: StatsCounter32  
2.25.3. Access: Read  
2.25.4. Persistence: No  
2.25.5. Factory Default Value: None. The get is rejected until the VEIP interface exists in Linux  
2.25.6. Valid Range: 0 – 2^32−1 (cumulative, wraps)  
2.25.7. Example: 0  
2.25.8. Usage: The total number of inbound packets which were chosen to be discarded even though no errors had been detected to prevent their being delivered. 32-bit counter; wraps.  

2.26. GPON Manager MUST report `MulticastPacketsReceived`: the total number of received packets, delivered by this layer to a higher layer, which were addressed to a multicast address at this layer.  
2.26.1. Data Model Name: `Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsReceived`  
2.26.2. Type: StatsCounter64  
2.26.3. Access: Read  
2.26.4. Persistence: No  
2.26.5. Factory Default Value: None. The get is rejected until the VEIP interface exists in Linux  
2.26.6. Valid Range: 0 – 2^64−1 (cumulative)  
2.26.7. Example: 1500  
2.26.8. Usage: The total number of received packets, delivered by this layer to a higher layer, which were addressed to a multicast address at this layer.  

3. GPON Manager MUST implement the following BBF `Device.XPON` parameters: ANI.

3.1. GPON Manager MUST report `Enable`: enables or disables the interface.  
3.1.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Enable`  
3.1.2. Type: boolean  
3.1.3. Access: Read (BBF defines Read/Write; a set returns RBUS_ERROR_NOT_WRITABLE)  
3.1.4. Persistence: No  
3.1.5. Factory Default Value: false  
3.1.6. Valid Range: true | false  
3.1.7. Example: true  
3.1.8. Usage: Enables or disables the interface. If disabled, the device should force the ONU state of this ANI to O1 (Initial state). Read-only (BBF: Read/Write) because writing false would switch the PON link off.  

3.2. GPON Manager MUST report `Status`: the current operational state of the interface.  
3.2.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Status`  
3.2.2. Type: string  
3.2.3. Access: Read  
3.2.4. Persistence: No  
3.2.5. Factory Default Value: NotPresent  
3.2.6. Valid Range: Up, Down, Unknown, Dormant, NotPresent, Error, NotAllowed. NotAllowed is never reported; a HAL LowerLayerDown fails the get.  
3.2.7. Example: Up  
3.2.8. Usage: The current operational state of the interface. Taken from the active optical module.  

3.3. GPON Manager MUST report `Alias`: unique, non-volatile alias for this instance.  
3.3.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Alias`  
3.3.2. Type: _AliasCWMP  
3.3.3. Access: Read (BBF defines Read/Write; a set returns RBUS_ERROR_NOT_WRITABLE)  
3.3.4. Persistence: No  
3.3.5. Factory Default Value: Empty string  
3.3.6. Valid Range: ≤ 64 chars  
3.3.7. Example: cpe-ani-1  
3.3.8. Usage: Unique, non-volatile alias for this instance. Read-only (BBF: writeOnceReadOnly).  

3.4. GPON Manager MUST report `LastChange`: the accumulated time in seconds since the interface entered its current operational state.  
3.4.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.LastChange`  
3.4.2. Type: unsignedInt  
3.4.3. Access: Read  
3.4.4. Persistence: No  
3.4.5. Factory Default Value: 0  
3.4.6. Valid Range: Seconds, 0 – 2^32−1  
3.4.7. Example: 3600  
3.4.8. Usage: The accumulated time in seconds since the interface entered its current operational state. Seconds since Status last changed, or since the row was created. Starts again from 0 when GPON Manager restarts.  

3.5. GPON Manager MUST report `TransceiverNumberOfEntries`: number of entries in the Transceiver table.  
3.5.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TransceiverNumberOfEntries`  
3.5.2. Type: unsignedInt  
3.5.3. Access: Read  
3.5.4. Persistence: No  
3.5.5. Factory Default Value: 0  
3.5.6. Valid Range: 0 – 2  
3.5.7. Example: 1  
3.5.8. Usage: Number of entries in the Transceiver table. Number of optical modules.  

3.6. GPON Manager MUST report `Name`: the textual name of the ANI entry as assigned by the CPE.  
3.6.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Name`  
3.6.2. Type: string  
3.6.3. Access: Read  
3.6.4. Persistence: No  
3.6.5. Factory Default Value: ANI1  
3.6.6. Valid Range: Fixed value  
3.6.7. Example: ANI1  
3.6.8. Usage: The textual name of the ANI entry as assigned by the CPE. Always ANI<i>.  

3.7. GPON Manager MUST report `PONMode`: PON mode, defines the detected or configured PON mode of the ANI.  
3.7.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.PONMode`  
3.7.2. Type: string  
3.7.3. Access: Read  
3.7.4. Persistence: No  
3.7.5. Factory Default Value: Unknown  
3.7.6. Valid Range: Unknown | G-PON | XG-PON | NG-PON2 | XGS-PON  
3.7.7. Example: XGS-PON  
3.7.8. Usage: PON mode, defines the detected or configured PON mode of the ANI. Same mapping as Transceiver.{i}.PONMode, taken from the active optical module; Unknown if there is no single active module or the PON mode is not valid.  

3.8. GPON Manager MUST report `SFPReferenceList`: small-form Factor Pluggable (SFP) entries associated with this interface.  
3.8.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.SFPReferenceList`  
3.8.2. Type: list<string>  
3.8.3. Access: Read  
3.8.4. Persistence: No  
3.8.5. Factory Default Value: Empty string  
3.8.6. Valid Range: Fixed value  
3.8.7. Example: Empty string  
3.8.8. Usage: Small-form Factor Pluggable (SFP) entries associated with this interface. Always empty: Device.SFPs.SFPCage.{i} is not implemented on this platform.  

4. GPON Manager MUST implement the following BBF `Device.XPON` parameters: ANI.TC — ONU Activation.

4.1. GPON Manager MUST report `ONUState`: ONU activation state.  
4.1.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUState`  
4.1.2. Type: string  
4.1.3. Access: Read  
4.1.4. Persistence: No  
4.1.5. Factory Default Value: O1  
4.1.6. Valid Range: O1, O2, O3, O2-3, O4, O5, O6, O7, O8, O9. O2-3 is never reported  
4.1.7. Example: O5 (Operation)  
4.1.8. Usage: ONU activation state. O1–O9 (the BBF combined O2-3 value is never reported). Telemetry: Every change raises XPON_ONU_STATE_CHANGE (From=O<a>,To=O<b>). Milestone transitions (leaving O1, falling back to O1, entering O6/O7, leaving O7, entering/leaving O5) also raise the specific XPON_ONU_* events.  

4.2. GPON Manager MUST report `VendorID`: identifies the vendor of the ONU.  
4.2.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.VendorID`  
4.2.2. Type: string  
4.2.3. Access: Read  
4.2.4. Persistence: No  
4.2.5. Factory Default Value: Empty string  
4.2.6. Valid Range: 4 chars  
4.2.7. Example: ALCL  
4.2.8. Usage: Identifies the vendor of the ONU.  

4.3. GPON Manager MUST report `SerialNumber`: represents the combination of the Vendor-ID and the Vendor-specific serial number (VSSN).  
4.3.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.SerialNumber`  
4.3.2. Type: string  
4.3.3. Access: Read  
4.3.4. Persistence: No  
4.3.5. Factory Default Value: None. The get is rejected until the HAL reports a serial number  
4.3.6. Valid Range: 12 chars (4-char VendorID + 8 hex-digit VSSN)  
4.3.7. Example: ALCLB1234567  
4.3.8. Usage: Represents the combination of the Vendor-ID and the Vendor-specific serial number (VSSN).  

4.4. GPON Manager MUST report `ONUID`: identifier that the OLT assigns to this ANI during the activation.  
4.4.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUID`  
4.4.2. Type: unsignedInt  
4.4.3. Access: Read  
4.4.4. Persistence: No  
4.4.5. Factory Default Value: 0  
4.4.6. Valid Range: 0 – 1022 (G-PON uses 0 – 253)  
4.4.7. Example: 1  
4.4.8. Usage: Identifier that the OLT assigns to this ANI during the activation.  

5. GPON Manager MUST implement the following BBF `Device.XPON` parameters: ANI.TC — Performance Monitoring (PHY / GEM / PLOAM / OMCI).

5.1. GPON Manager MUST report `CorrectedFECBytes`: the number of bytes that were corrected by the FEC function.  
5.1.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECBytes`  
5.1.2. Type: StatsCounter64  
5.1.3. Access: Read  
5.1.4. Persistence: No  
5.1.5. Factory Default Value: 0  
5.1.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.1.7. Example: 1048576  
5.1.8. Usage: The number of bytes that were corrected by the FEC function.  

5.2. GPON Manager MUST report `CorrectedFECCodewords`: count of FEC codewords that contained errors but were corrected by the FEC function.  
5.2.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECCodewords`  
5.2.2. Type: StatsCounter64  
5.2.3. Access: Read  
5.2.4. Persistence: No  
5.2.5. Factory Default Value: 0  
5.2.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.2.7. Example: 1048576  
5.2.8. Usage: Count of FEC codewords that contained errors but were corrected by the FEC function.  

5.3. GPON Manager MUST report `UncorrectableFECCodewords`: count of FEC codewords that contained errors and could not be corrected by the FEC function.  
5.3.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UncorrectableFECCodewords`  
5.3.2. Type: StatsCounter64  
5.3.3. Access: Read  
5.3.4. Persistence: No  
5.3.5. Factory Default Value: 0  
5.3.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.3.7. Example: 1048576  
5.3.8. Usage: Count of FEC codewords that contained errors and could not be corrected by the FEC function.  

5.4. GPON Manager MUST report `TotalFECCodewords`: count of total received FEC codewords.  
5.4.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.TotalFECCodewords`  
5.4.2. Type: StatsCounter64  
5.4.3. Access: Read  
5.4.4. Persistence: No  
5.4.5. Factory Default Value: 0  
5.4.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.4.7. Example: 1048576  
5.4.8. Usage: Count of total received FEC codewords.  

5.5. GPON Manager MUST report `PSBdHECErrorCount`: HEC error in any of the fields of PSBd.  
5.5.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.PSBdHECErrorCount`  
5.5.2. Type: StatsCounter32  
5.5.3. Access: Read  
5.5.4. Persistence: No  
5.5.5. Factory Default Value: 0  
5.5.6. Valid Range: 0 – 2^32−1 (cumulative)  
5.5.7. Example: 12  
5.5.8. Usage: HEC error in any of the fields of PSBd.  

5.6. GPON Manager MUST report `HeaderHECErrorCount`: HEC errors received in the DS XGTC or FS header.  
5.6.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.HeaderHECErrorCount`  
5.6.2. Type: StatsCounter32  
5.6.3. Access: Read  
5.6.4. Persistence: No  
5.6.5. Factory Default Value: 0  
5.6.6. Valid Range: 0 – 2^32−1 (cumulative)  
5.6.7. Example: 12  
5.6.8. Usage: HEC errors received in the DS XGTC or FS header. Best-match mapping.  

5.7. GPON Manager MUST report `FrameHeaderHECErrors`: XGEM frame header HEC errors.  
5.7.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FrameHeaderHECErrors`  
5.7.2. Type: StatsCounter32  
5.7.3. Access: Read  
5.7.4. Persistence: No  
5.7.5. Factory Default Value: 0  
5.7.6. Valid Range: 0 – 2^32−1 (cumulative)  
5.7.7. Example: 12  
5.7.8. Usage: XGEM frame header HEC errors. Number of events involving loss of XGEM channel delineation. Best-match mapping.  

5.8. GPON Manager MUST report `FramesSent`: total number of (X)GEM frames transmitted.  
5.8.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesSent`  
5.8.2. Type: StatsCounter64  
5.8.3. Access: Read  
5.8.4. Persistence: No  
5.8.5. Factory Default Value: 0  
5.8.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.8.7. Example: 120000  
5.8.8. Usage: Total number of (X)GEM frames transmitted. Sum over all GEM ports.  

5.9. GPON Manager MUST report `FramesReceived`: total number of (X)GEM frames received.  
5.9.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesReceived`  
5.9.2. Type: StatsCounter64  
5.9.3. Access: Read  
5.9.4. Persistence: No  
5.9.5. Factory Default Value: 0  
5.9.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.9.7. Example: 450000  
5.9.8. Usage: Total number of (X)GEM frames received. Sum over all GEM ports.  

5.10. GPON Manager MUST report `MICErrors`: PLOAM MIC errors.  
5.10.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.MICErrors`  
5.10.2. Type: StatsCounter32  
5.10.3. Access: Read  
5.10.4. Persistence: No  
5.10.5. Factory Default Value: 0  
5.10.6. Valid Range: 0 – 2^32−1 (cumulative)  
5.10.7. Example: 12  
5.10.8. Usage: PLOAM MIC errors. Counter of received PLOAM messages with MIC errors.  

5.11. GPON Manager MUST report `DownstreamMessageCount`: downstream PLOAM message count.  
5.11.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.DownstreamMessageCount`  
5.11.2. Type: StatsCounter64  
5.11.3. Access: Read  
5.11.4. Persistence: No  
5.11.5. Factory Default Value: 0  
5.11.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.11.7. Example: 1048576  
5.11.8. Usage: Downstream PLOAM message count. Count of PLOAM messages sent by OLT, received by ONU, either broadcast or directed to the specific ONU-ID.  

5.12. GPON Manager MUST report `UpstreamMessageCount`: upstream PLOAM message count.  
5.12.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.UpstreamMessageCount`  
5.12.2. Type: StatsCounter64  
5.12.3. Access: Read  
5.12.4. Persistence: No  
5.12.5. Factory Default Value: 0  
5.12.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.12.7. Example: 1048576  
5.12.8. Usage: Upstream PLOAM message count. Count of messages (other than Acknowledgement) sent by ONU to OLT.  

5.13. GPON Manager MUST report `BaselineMessagesReceived`: OMCI baseline message count.  
5.13.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.BaselineMessagesReceived`  
5.13.2. Type: StatsCounter64  
5.13.3. Access: Read  
5.13.4. Persistence: No  
5.13.5. Factory Default Value: 0  
5.13.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.13.7. Example: 1048576  
5.13.8. Usage: OMCI baseline message count.  

5.14. GPON Manager MUST report `ExtendedMessagesReceived`: OMCI extended message count.  
5.14.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.ExtendedMessagesReceived`  
5.14.2. Type: StatsCounter64  
5.14.3. Access: Read  
5.14.4. Persistence: No  
5.14.5. Factory Default Value: 0  
5.14.6. Valid Range: 0 – 2^64−1 (cumulative)  
5.14.7. Example: 1048576  
5.14.8. Usage: OMCI extended message count.  

5.15. GPON Manager MUST report `MICErrors`: count of received OMCI messages with MIC errors.  
5.15.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.MICErrors`  
5.15.2. Type: StatsCounter32  
5.15.3. Access: Read  
5.15.4. Persistence: No  
5.15.5. Factory Default Value: 0  
5.15.6. Valid Range: 0 – 2^32−1 (cumulative)  
5.15.7. Example: 12  
5.15.8. Usage: Count of received OMCI messages with MIC errors.  

6. GPON Manager MUST implement the following BBF `Device.XPON` parameters: ANI.TC — GEM Ports.

6.1. GPON Manager MUST report `PortNumberOfEntries`: number of entries in the Port table.  
6.1.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.PortNumberOfEntries`  
6.1.2. Type: unsignedInt  
6.1.3. Access: Read  
6.1.4. Persistence: No  
6.1.5. Factory Default Value: 0  
6.1.6. Valid Range: 0 – 32  
6.1.7. Example: 4  
6.1.8. Usage: Number of entries in the Port table. Number of GEM ports.  

6.2. GPON Manager MUST report `PortID`: identifies a GEM port.  
6.2.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortID`  
6.2.2. Type: unsignedInt  
6.2.3. Access: Read  
6.2.4. Persistence: No  
6.2.5. Factory Default Value: 0  
6.2.6. Valid Range: 0 – 65534 (G-PON uses 0 – 4095)  
6.2.7. Example: 1024  
6.2.8. Usage: Identifies a GEM port.  

6.3. GPON Manager MUST report `Direction`: type of connection this GEM port is used for.  
6.3.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.Direction`  
6.3.2. Type: string  
6.3.3. Access: Read  
6.3.4. Persistence: No  
6.3.5. Factory Default Value: unidirectional  
6.3.6. Valid Range: bidirectional | unidirectional  
6.3.7. Example: bidirectional  
6.3.8. Usage: Type of connection this GEM port is used for. bidirectional for a Bidirectional port, unidirectional for a Unidirectional port. unidirectional is not a BBF value; the PON stack does not report the direction.  

6.4. GPON Manager MUST report `PortType`: GEM port type.  
6.4.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortType`  
6.4.2. Type: string  
6.4.3. Access: Read  
6.4.4. Persistence: No  
6.4.5. Factory Default Value: multicast  
6.4.6. Valid Range: unicast | multicast  
6.4.7. Example: unicast  
6.4.8. Usage: GEM port type. unicast for a Bidirectional port, multicast for a Unidirectional (downstream-only) port; broadcast is never reported until the HAL provides the GEM port type.  

6.5. GPON Manager MUST report `FramesSent`: (X)GEM frames transmitted by this (X)GEM port.  
6.5.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesSent`  
6.5.2. Type: StatsCounter64  
6.5.3. Access: Read  
6.5.4. Persistence: No  
6.5.5. Factory Default Value: 0  
6.5.6. Valid Range: 0 – 2^64−1 (cumulative)  
6.5.7. Example: 1048576  
6.5.8. Usage: (X)GEM frames transmitted by this (X)GEM port.  

6.6. GPON Manager MUST report `FramesReceived`: (X)GEM frames received by this (X)GEM port.  
6.6.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesReceived`  
6.6.2. Type: StatsCounter64  
6.6.3. Access: Read  
6.6.4. Persistence: No  
6.6.5. Factory Default Value: 0  
6.6.6. Valid Range: 0 – 2^64−1 (cumulative)  
6.6.7. Example: 1048576  
6.6.8. Usage: (X)GEM frames received by this (X)GEM port.  

7. GPON Manager MUST implement the following BBF `Device.XPON` parameters: ANI.TC — Performance Thresholds.

7.1. GPON Manager MUST report `SignalFail`: signal fail (SF) threshold.  
7.1.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalFail`  
7.1.2. Type: unsignedInt  
7.1.3. Access: Read  
7.1.4. Persistence: No  
7.1.5. Factory Default Value: None. The get is rejected until the HAL reports a value in range  
7.1.6. Valid Range: 3 – 8 (BER 10^−x)  
7.1.7. Example: 5  
7.1.8. Usage: Signal fail (SF) threshold. This parameter specifies the downstream bit error rate (BER) threshold to detect the SF alarm.  

7.2. GPON Manager MUST report `SignalDegrade`: signal degrade (SD) threshold.  
7.2.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalDegrade`  
7.2.2. Type: unsignedInt  
7.2.3. Access: Read  
7.2.4. Persistence: No  
7.2.5. Factory Default Value: None. The get is rejected until the HAL reports a value in range  
7.2.6. Valid Range: 4 – 10 (BER 10^−x), must be > SignalFail  
7.2.7. Example: 9  
7.2.8. Usage: Signal degrade (SD) threshold. This parameter specifies the downstream BER threshold to detect the SD alarm.  

8. GPON Manager MUST implement the following BBF `Device.XPON` parameters: ANI.TC — Alarms.

8.1. GPON Manager MUST report `LOS`: loss of signal.  
8.1.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOS`  
8.1.2. Type: boolean  
8.1.3. Access: Read  
8.1.4. Persistence: No  
8.1.5. Factory Default Value: false  
8.1.6. Valid Range: true (Active) | false (Inactive)  
8.1.7. Example: true  
8.1.8. Usage: Loss of signal. Active = true. Telemetry: Raise/clear sends XPON_ALARM_LOS.  

8.2. GPON Manager MUST report `LOF`: loss of frame.  
8.2.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOF`  
8.2.2. Type: boolean  
8.2.3. Access: Read  
8.2.4. Persistence: No  
8.2.5. Factory Default Value: false  
8.2.6. Valid Range: true (Active) | false (Inactive)  
8.2.7. Example: true  
8.2.8. Usage: Loss of frame. Active = true. Telemetry: Raise/clear sends XPON_ALARM_LOF.  

8.3. GPON Manager MUST report `SF`: signal failed.  
8.3.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SF`  
8.3.2. Type: boolean  
8.3.3. Access: Read  
8.3.4. Persistence: No  
8.3.5. Factory Default Value: false  
8.3.6. Valid Range: true (Active) | false (Inactive)  
8.3.7. Example: true  
8.3.8. Usage: Signal failed. Active = true. Telemetry: Raise/clear sends XPON_ALARM_SF.  

8.4. GPON Manager MUST report `SD`: signal degraded.  
8.4.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SD`  
8.4.2. Type: boolean  
8.4.3. Access: Read  
8.4.4. Persistence: No  
8.4.5. Factory Default Value: false  
8.4.6. Valid Range: true (Active) | false (Inactive)  
8.4.7. Example: true  
8.4.8. Usage: Signal degraded. Active = true. Telemetry: Raise/clear sends XPON_ALARM_SD.  

8.5. GPON Manager MUST report `LCDG`: loss of GEM channel delineation.  
8.5.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LCDG`  
8.5.2. Type: boolean  
8.5.3. Access: Read  
8.5.4. Persistence: No  
8.5.5. Factory Default Value: false  
8.5.6. Valid Range: true (Active) | false (Inactive)  
8.5.7. Example: true  
8.5.8. Usage: Loss of GEM channel delineation. Active = true. Telemetry: Raise/clear sends XPON_ALARM_LCDG.  

8.6. GPON Manager MUST report `TF`: transmitter failure.  
8.6.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.TF`  
8.6.2. Type: boolean  
8.6.3. Access: Read  
8.6.4. Persistence: No  
8.6.5. Factory Default Value: false  
8.6.6. Valid Range: true (Active) | false (Inactive)  
8.6.7. Example: true  
8.6.8. Usage: Transmitter failure. Active = true. Telemetry: Raise/clear sends XPON_ALARM_TF.  

8.7. GPON Manager MUST report `SUF`: start-up failure.  
8.7.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SUF`  
8.7.2. Type: boolean  
8.7.3. Access: Read  
8.7.4. Persistence: No  
8.7.5. Factory Default Value: false  
8.7.6. Valid Range: true (Active) | false (Inactive)  
8.7.7. Example: true  
8.7.8. Usage: Start-up failure. Active = true. Telemetry: Raise/clear sends XPON_ALARM_SUF.  

8.8. GPON Manager MUST report `MEM`: message error message.  
8.8.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MEM`  
8.8.2. Type: boolean  
8.8.3. Access: Read  
8.8.4. Persistence: No  
8.8.5. Factory Default Value: false  
8.8.6. Valid Range: true (Active) | false (Inactive)  
8.8.7. Example: true  
8.8.8. Usage: Message error message. Active = true. Telemetry: Raise/clear sends XPON_ALARM_MEM.  

8.9. GPON Manager MUST report `DACT`: deactivate ONU-ID.  
8.9.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DACT`  
8.9.2. Type: boolean  
8.9.3. Access: Read  
8.9.4. Persistence: No  
8.9.5. Factory Default Value: false  
8.9.6. Valid Range: true (Active) | false (Inactive)  
8.9.7. Example: true  
8.9.8. Usage: Deactivate ONU-ID. Active = true. Telemetry: Raise/clear sends XPON_ALARM_DACT.  

8.10. GPON Manager MUST report `DIS`: disabled ONU.  
8.10.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DIS`  
8.10.2. Type: boolean  
8.10.3. Access: Read  
8.10.4. Persistence: No  
8.10.5. Factory Default Value: false  
8.10.6. Valid Range: true (Active) | false (Inactive)  
8.10.7. Example: true  
8.10.8. Usage: Disabled ONU. Active = true. Telemetry: Raise/clear sends XPON_ALARM_DIS.  

8.11. GPON Manager MUST report `MIS`: link mismatching.  
8.11.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MIS`  
8.11.2. Type: boolean  
8.11.3. Access: Read  
8.11.4. Persistence: No  
8.11.5. Factory Default Value: false  
8.11.6. Valid Range: true (Active) | false (Inactive)  
8.11.7. Example: true  
8.11.8. Usage: Link mismatching. Active = true. Telemetry: Raise/clear sends XPON_ALARM_MIS.  

8.12. GPON Manager MUST report `PEE`: when the ONU receives a PEE message.  
8.12.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.PEE`  
8.12.2. Type: boolean  
8.12.3. Access: Read  
8.12.4. Persistence: No  
8.12.5. Factory Default Value: false  
8.12.6. Valid Range: true (Active) | false (Inactive)  
8.12.7. Example: true  
8.12.8. Usage: When the ONU receives a PEE message. Active = true. Telemetry: Raise/clear sends XPON_ALARM_PEE.  

8.13. GPON Manager MUST report `RDI`: remote defect indication.  
8.13.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.RDI`  
8.13.2. Type: boolean  
8.13.3. Access: Read  
8.13.4. Persistence: No  
8.13.5. Factory Default Value: false  
8.13.6. Valid Range: true (Active) | false (Inactive)  
8.13.7. Example: true  
8.13.8. Usage: Remote defect indication. Active = true. Telemetry: Raise/clear sends XPON_ALARM_RDI.  

8.14. GPON Manager MUST report `ROGUE`: the ANI behaves rogue: it is not transmitting in a manner consistent with parameters specified in the ITU-T PON standards.  
8.14.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.ROGUE`  
8.14.2. Type: boolean  
8.14.3. Access: Read  
8.14.4. Persistence: No  
8.14.5. Factory Default Value: false  
8.14.6. Valid Range: true (Active) | false (Inactive)  
8.14.7. Example: true  
8.14.8. Usage: The ANI behaves rogue: it is not transmitting in a manner consistent with parameters specified in the ITU-T PON standards. Active = true. Telemetry: Raise/clear sends XPON_ALARM_ROGUE.  

9. GPON Manager MUST implement the following BBF `Device.XPON` parameters: ANI.Transceiver.

9.1. GPON Manager MUST report `ID`: the ID as assigned by the CPE to this Transceiver entry.  
9.1.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.ID`  
9.1.2. Type: unsignedInt  
9.1.3. Access: Read  
9.1.4. Persistence: No  
9.1.5. Factory Default Value: 0  
9.1.6. Valid Range: 0 – 1  
9.1.7. Example: 0  
9.1.8. Usage: The ID as assigned by the CPE to this Transceiver entry.  

9.2. GPON Manager MUST report `Identifier`: transceiver type as an SFF-8024 Table 4-1 identifier.  
9.2.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Identifier`  
9.2.2. Type: unsignedInt  
9.2.3. Access: Read  
9.2.4. Persistence: No  
9.2.5. Factory Default Value: None. The get is rejected until the HAL reports Cage = SFP  
9.2.6. Valid Range: Fixed value  
9.2.7. Example: 3  
9.2.8. Usage: Transceiver type as an SFF-8024 Table 4-1 identifier. 3 (SFP/SFP+/SFP28) when the module cage is SFP; not available for bond-on-board (BoB) optics.  

9.3. GPON Manager MUST report `VendorName`: vendor name.  
9.3.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorName`  
9.3.2. Type: string  
9.3.3. Access: Read  
9.3.4. Persistence: No  
9.3.5. Factory Default Value: Empty string  
9.3.6. Valid Range: ≤ 256 chars  
9.3.7. Example: ACME Optics  
9.3.8. Usage: Vendor name.  

9.4. GPON Manager MUST report `VendorPartNumber`: vendor part number.  
9.4.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorPartNumber`  
9.4.2. Type: string  
9.4.3. Access: Read  
9.4.4. Persistence: No  
9.4.5. Factory Default Value: Empty string  
9.4.6. Valid Range: ≤ 256 chars  
9.4.7. Example: XGSPON-SFP-20  
9.4.8. Usage: Vendor part number.  

9.5. GPON Manager MUST report `VendorRevision`: vendor revision.  
9.5.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorRevision`  
9.5.2. Type: string  
9.5.3. Access: Read  
9.5.4. Persistence: No  
9.5.5. Factory Default Value: Empty string  
9.5.6. Valid Range: ≤ 256 chars  
9.5.7. Example: A1  
9.5.8. Usage: Vendor revision.  

9.6. GPON Manager MUST report `PONMode`: PON mode, reports the PON mode of the transceiver.  
9.6.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.PONMode`  
9.6.2. Type: string  
9.6.3. Access: Read  
9.6.4. Persistence: No  
9.6.5. Factory Default Value: Unknown  
9.6.6. Valid Range: Unknown, G-PON, XG-PON, NG-PON2, XGS-PON. Unknown when the HAL reports PonModeValid = false  
9.6.7. Example: XGS-PON  
9.6.8. Usage: PON mode, reports the PON mode of the transceiver. Mapped to G-PON, XG-PON, NG-PON2, XGS-PON. Telemetry: An EPON module raises XPON_SYSTEM_HAL_WRONG_PON_MODE. Unknown when the PON mode is not valid.  

9.7. GPON Manager MUST report `Connector`: connector type.  
9.7.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Connector`  
9.7.2. Type: string  
9.7.3. Access: Read  
9.7.4. Persistence: No  
9.7.5. Factory Default Value: LC  
9.7.6. Valid Range: Unknown, LC, SC, ST, FC, MT-RJ. Unknown is never reported  
9.7.7. Example: SC  
9.7.8. Usage: Connector type.  

9.8. GPON Manager MUST report `NominalBitRateDownstream`: nominal data rate downstream in kbps.  
9.8.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateDownstream`  
9.8.2. Type: unsignedInt  
9.8.3. Access: Read  
9.8.4. Persistence: No  
9.8.5. Factory Default Value: 0  
9.8.6. Valid Range: kbps  
9.8.7. Example: 2488320 (G-PON), 9953280 (XGS-PON)  
9.8.8. Usage: Nominal data rate downstream in kbps.  

9.9. GPON Manager MUST report `NominalBitRateUpstream`: nominal data rate upstream in kbps.  
9.9.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateUpstream`  
9.9.2. Type: unsignedInt  
9.9.3. Access: Read  
9.9.4. Persistence: No  
9.9.5. Factory Default Value: 0  
9.9.6. Valid Range: kbps  
9.9.7. Example: 1244160 (G-PON), 9953280 (XGS-PON)  
9.9.8. Usage: Nominal data rate upstream in kbps.  

9.10. GPON Manager MUST report `RxPower`: measured RX power in 0.1 dBm.  
9.10.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.RxPower`  
9.10.2. Type: int  
9.10.3. Access: Read  
9.10.4. Persistence: No  
9.10.5. Factory Default Value: 0  
9.10.6. Valid Range: 0.1 dBm. Typical −280 – −80  
9.10.7. Example: -185 (−18.5 dBm)  
9.10.8. Usage: Measured RX power in 0.1 dBm.  

9.11. GPON Manager MUST report `TxPower`: measured TX power in 0.1 dBm.  
9.11.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.TxPower`  
9.11.2. Type: int  
9.11.3. Access: Read  
9.11.4. Persistence: No  
9.11.5. Factory Default Value: 0  
9.11.6. Valid Range: 0.1 dBm. Typical 5 – 90  
9.11.7. Example: 25 (2.5 dBm)  
9.11.8. Usage: Measured TX power in 0.1 dBm.  

9.12. GPON Manager MUST report `Voltage`: measured supply voltage in mV.  
9.12.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Voltage`  
9.12.2. Type: unsignedInt  
9.12.3. Access: Read  
9.12.4. Persistence: No  
9.12.5. Factory Default Value: 0  
9.12.6. Valid Range: mV, ≥ 0  
9.12.7. Example: 3300 (3.3 V)  
9.12.8. Usage: Measured supply voltage in mV.  

9.13. GPON Manager MUST report `Bias`: measured bias current in µA.  
9.13.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Bias`  
9.13.2. Type: unsignedInt  
9.13.3. Access: Read  
9.13.4. Persistence: No  
9.13.5. Factory Default Value: 0  
9.13.6. Valid Range: µA  
9.13.7. Example: 12000 (12 mA)  
9.13.8. Usage: Measured bias current in µA.  

9.14. GPON Manager MUST report `Temperature`: measured temperature in °C.  
9.14.1. Data Model Name: `Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Temperature`  
9.14.2. Type: int  
9.14.3. Access: Read  
9.14.4. Persistence: No  
9.14.5. Factory Default Value: 0  
9.14.6. Valid Range: °C, ≥ −274  
9.14.7. Example: 47  
9.14.8. Usage: Measured temperature in °C. Rounded to the nearest whole degree.  

10. GPON Manager MUST implement the following RDK extension `Device.X_RDK_ONT` parameters: PhysicalMedia — Module & Interface.

10.1. GPON Manager MUST report `Cage`: physical cage of the optical module: BoB (bond-on-board) or SFP.  
10.1.1. Data Model Name: `Device.X_RDK_ONT.PhysicalMedia.{i}.Cage`  
10.1.2. Type: string (enum)  
10.1.3. Access: Read  
10.1.4. Persistence: No  
10.1.5. Factory Default Value: BoB  
10.1.6. Valid Range: BoB | SFP  
10.1.7. Example: SFP  
10.1.8. Usage: Physical cage of the optical module: BoB (bond-on-board) or SFP.  

10.2. GPON Manager MUST report `ModuleFirmwareVersion`: optical module firmware version.  
10.2.1. Data Model Name: `Device.X_RDK_ONT.PhysicalMedia.{i}.ModuleFirmwareVersion`  
10.2.2. Type: string(256)  
10.2.3. Access: Read  
10.2.4. Persistence: No  
10.2.5. Factory Default Value: Empty string  
10.2.6. Valid Range: ≤ 255 chars  
10.2.7. Example: 1.0.3  
10.2.8. Usage: Optical module firmware version.  

10.3. GPON Manager MUST report `RedundancyState`: redundancy role of this media: Active or Standby.  
10.3.1. Data Model Name: `Device.X_RDK_ONT.PhysicalMedia.{i}.RedundancyState`  
10.3.2. Type: string (enum)  
10.3.3. Access: Read  
10.3.4. Persistence: No  
10.3.5. Factory Default Value: Active  
10.3.6. Valid Range: Active | Standby  
10.3.7. Example: Active  
10.3.8. Usage: Redundancy role of this media: Active or Standby.  

10.4. GPON Manager MUST report `LowerLayers`: comma-separated list of lower-layer paths.  
10.4.1. Data Model Name: `Device.X_RDK_ONT.PhysicalMedia.{i}.LowerLayers`  
10.4.2. Type: string(1024)  
10.4.3. Access: Read/Write (the value is written to the vendor HAL)  
10.4.4. Persistence: No  
10.4.5. Factory Default Value: Empty string  
10.4.6. Valid Range: ≤ 1023 chars  
10.4.7. Example: Device.Optical.Interface.1  
10.4.8. Usage: Comma-separated list of lower-layer paths.  

10.5. GPON Manager MUST report `Upstream`: whether this interface is upstream-facing.  
10.5.1. Data Model Name: `Device.X_RDK_ONT.PhysicalMedia.{i}.Upstream`  
10.5.2. Type: boolean  
10.5.3. Access: Read  
10.5.4. Persistence: No  
10.5.5. Factory Default Value: false  
10.5.6. Valid Range: true | false  
10.5.7. Example: true  
10.5.8. Usage: Whether this interface is upstream-facing.  

11. GPON Manager MUST implement the following RDK extension `Device.X_RDK_ONT` parameters: PhysicalMedia — Optical Power Thresholds.

11.1. GPON Manager MUST report `SignalLevelLowerThreshold`: lower RX optical power threshold.  
11.1.1. Data Model Name: `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelLowerThreshold`  
11.1.2. Type: int32  
11.1.3. Access: Read/Write (the value is written to the vendor HAL)  
11.1.4. Persistence: No  
11.1.5. Factory Default Value: 0  
11.1.6. Valid Range: 0.1 dBm  
11.1.7. Example: -280 (−28 dBm)  
11.1.8. Usage: Lower RX optical power threshold.  

11.2. GPON Manager MUST report `SignalLevelUpperThreshold`: upper RX optical power threshold.  
11.2.1. Data Model Name: `Device.X_RDK_ONT.PhysicalMedia.{i}.RxPower.SignalLevelUpperThreshold`  
11.2.2. Type: int32  
11.2.3. Access: Read/Write (the value is written to the vendor HAL)  
11.2.4. Persistence: No  
11.2.5. Factory Default Value: 0  
11.2.6. Valid Range: 0.1 dBm  
11.2.7. Example: -80 (−8 dBm)  
11.2.8. Usage: Upper RX optical power threshold.  

11.3. GPON Manager MUST report `SignalLevelLowerThreshold`: lower TX optical power threshold.  
11.3.1. Data Model Name: `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelLowerThreshold`  
11.3.2. Type: int32  
11.3.3. Access: Read/Write (the value is written to the vendor HAL)  
11.3.4. Persistence: No  
11.3.5. Factory Default Value: 0  
11.3.6. Valid Range: 0.1 dBm  
11.3.7. Example: 5 (0.5 dBm)  
11.3.8. Usage: Lower TX optical power threshold.  

11.4. GPON Manager MUST report `SignalLevelUpperThreshold`: upper TX optical power threshold.  
11.4.1. Data Model Name: `Device.X_RDK_ONT.PhysicalMedia.{i}.TxPower.SignalLevelUpperThreshold`  
11.4.2. Type: int32  
11.4.3. Access: Read/Write (the value is written to the vendor HAL)  
11.4.4. Persistence: No  
11.4.5. Factory Default Value: 0  
11.4.6. Valid Range: 0.1 dBm  
11.4.7. Example: 90 (9 dBm)  
11.4.8. Usage: Upper TX optical power threshold.  

12. GPON Manager MUST implement the following RDK extension `Device.X_RDK_ONT` parameters: GTC Counters.

12.1. GPON Manager MUST report `FramesLost`: downstream frames lost.  
12.1.1. Data Model Name: `Device.X_RDK_ONT.Gtc.FramesLost`  
12.1.2. Type: uint32  
12.1.3. Access: Read  
12.1.4. Persistence: No  
12.1.5. Factory Default Value: 0  
12.1.6. Valid Range: 0 – 2^32−1 (cumulative)  
12.1.7. Example: 12  
12.1.8. Usage: Downstream frames lost.  

13. GPON Manager MUST implement the following RDK extension `Device.X_RDK_ONT` parameters: PLOAM.

13.1. GPON Manager MUST report `ActivationCounter`: number of times the ONU has reached the operational state.  
13.1.1. Data Model Name: `Device.X_RDK_ONT.Ploam.ActivationCounter`  
13.1.2. Type: uint32  
13.1.3. Access: Read  
13.1.4. Persistence: No  
13.1.5. Factory Default Value: 0  
13.1.6. Valid Range: 0 – 2^32−1  
13.1.7. Example: 3  
13.1.8. Usage: Number of times the ONU has reached the operational state.  

13.2. GPON Manager MUST report `TO1`: TO1 (serial-number / ranging) timer value.  
13.2.1. Data Model Name: `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO1`  
13.2.2. Type: uint32  
13.2.3. Access: Read  
13.2.4. Persistence: No  
13.2.5. Factory Default Value: 0  
13.2.6. Valid Range: ms  
13.2.7. Example: 10000 (10 s)  
13.2.8. Usage: TO1 (serial-number / ranging) timer value.  

13.3. GPON Manager MUST report `TO2`: TO2 (downstream-loss recovery: GPON POPUP / XGS-PON intermittent LODS) timer value.  
13.3.1. Data Model Name: `Device.X_RDK_ONT.Ploam.RegistrationTimers.TO2`  
13.3.2. Type: uint32  
13.3.3. Access: Read  
13.3.4. Persistence: No  
13.3.5. Factory Default Value: 0  
13.3.6. Valid Range: ms  
13.3.7. Example: 100 (100 ms)  
13.3.8. Usage: TO2 (downstream-loss recovery: GPON POPUP / XGS-PON intermittent LODS) timer value.  

14. GPON Manager MUST implement the following RDK extension `Device.X_RDK_ONT` parameters: GEM Port Ethernet Flow (VLAN Classification).

14.1. GPON Manager MUST report `Tagged`: ingress tagging mode of the flow.  
14.1.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.Tagged`  
14.1.2. Type: string (enum)  
14.1.3. Access: Read  
14.1.4. Persistence: No  
14.1.5. Factory Default Value: Single  
14.1.6. Valid Range: Single | Double  
14.1.7. Example: Single  
14.1.8. Usage: Ingress tagging mode of the flow.  

14.2. GPON Manager MUST report `Vid`: ingress S-VLAN VLAN ID.  
14.2.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Vid`  
14.2.2. Type: uint32  
14.2.3. Access: Read  
14.2.4. Persistence: No  
14.2.5. Factory Default Value: 0  
14.2.6. Valid Range: 0 – 4095 (1 – 4094 usable)  
14.2.7. Example: 100  
14.2.8. Usage: Ingress S-VLAN VLAN ID.  

14.3. GPON Manager MUST report `Pcp`: ingress S-VLAN priority code point.  
14.3.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Pcp`  
14.3.2. Type: uint32  
14.3.3. Access: Read  
14.3.4. Persistence: No  
14.3.5. Factory Default Value: 0  
14.3.6. Valid Range: 0 – 7  
14.3.7. Example: 0  
14.3.8. Usage: Ingress S-VLAN priority code point.  

14.4. GPON Manager MUST report `Dei`: ingress S-VLAN drop eligible indicator.  
14.4.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.S-VLAN.Dei`  
14.4.2. Type: uint32  
14.4.3. Access: Read  
14.4.4. Persistence: No  
14.4.5. Factory Default Value: 0  
14.4.6. Valid Range: 0 | 1  
14.4.7. Example: 0  
14.4.8. Usage: Ingress S-VLAN drop eligible indicator.  

14.5. GPON Manager MUST report `Vid`: ingress C-VLAN VLAN ID.  
14.5.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Vid`  
14.5.2. Type: uint32  
14.5.3. Access: Read  
14.5.4. Persistence: No  
14.5.5. Factory Default Value: 0  
14.5.6. Valid Range: 0 – 4095 (1 – 4094 usable)  
14.5.7. Example: 100  
14.5.8. Usage: Ingress C-VLAN VLAN ID.  

14.6. GPON Manager MUST report `Pcp`: ingress C-VLAN priority code point.  
14.6.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Pcp`  
14.6.2. Type: uint32  
14.6.3. Access: Read  
14.6.4. Persistence: No  
14.6.5. Factory Default Value: 0  
14.6.6. Valid Range: 0 – 7  
14.6.7. Example: 0  
14.6.8. Usage: Ingress C-VLAN priority code point.  

14.7. GPON Manager MUST report `Dei`: ingress C-VLAN drop eligible indicator.  
14.7.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Ingress.C-VLAN.Dei`  
14.7.2. Type: uint32  
14.7.3. Access: Read  
14.7.4. Persistence: No  
14.7.5. Factory Default Value: 0  
14.7.6. Valid Range: 0 | 1  
14.7.7. Example: 0  
14.7.8. Usage: Ingress C-VLAN drop eligible indicator.  

14.8. GPON Manager MUST report `Tagged`: egress tagging mode of the flow.  
14.8.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.Tagged`  
14.8.2. Type: string (enum)  
14.8.3. Access: Read  
14.8.4. Persistence: No  
14.8.5. Factory Default Value: Single  
14.8.6. Valid Range: Single | Double  
14.8.7. Example: Single  
14.8.8. Usage: Egress tagging mode of the flow.  

14.9. GPON Manager MUST report `Vid`: egress S-VLAN VLAN ID.  
14.9.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Vid`  
14.9.2. Type: uint32  
14.9.3. Access: Read  
14.9.4. Persistence: No  
14.9.5. Factory Default Value: 0  
14.9.6. Valid Range: 0 – 4095 (1 – 4094 usable)  
14.9.7. Example: 100  
14.9.8. Usage: Egress S-VLAN VLAN ID.  

14.10. GPON Manager MUST report `Pcp`: egress S-VLAN priority code point.  
14.10.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Pcp`  
14.10.2. Type: uint32  
14.10.3. Access: Read  
14.10.4. Persistence: No  
14.10.5. Factory Default Value: 0  
14.10.6. Valid Range: 0 – 7  
14.10.7. Example: 0  
14.10.8. Usage: Egress S-VLAN priority code point.  

14.11. GPON Manager MUST report `Dei`: egress S-VLAN drop eligible indicator.  
14.11.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.S-VLAN.Dei`  
14.11.2. Type: uint32  
14.11.3. Access: Read  
14.11.4. Persistence: No  
14.11.5. Factory Default Value: 0  
14.11.6. Valid Range: 0 | 1  
14.11.7. Example: 0  
14.11.8. Usage: Egress S-VLAN drop eligible indicator.  

14.12. GPON Manager MUST report `Vid`: egress C-VLAN VLAN ID.  
14.12.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Vid`  
14.12.2. Type: uint32  
14.12.3. Access: Read  
14.12.4. Persistence: No  
14.12.5. Factory Default Value: 0  
14.12.6. Valid Range: 0 – 4095 (1 – 4094 usable)  
14.12.7. Example: 100  
14.12.8. Usage: Egress C-VLAN VLAN ID.  

14.13. GPON Manager MUST report `Pcp`: egress C-VLAN priority code point.  
14.13.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Pcp`  
14.13.2. Type: uint32  
14.13.3. Access: Read  
14.13.4. Persistence: No  
14.13.5. Factory Default Value: 0  
14.13.6. Valid Range: 0 – 7  
14.13.7. Example: 0  
14.13.8. Usage: Egress C-VLAN priority code point.  

14.14. GPON Manager MUST report `Dei`: egress C-VLAN drop eligible indicator.  
14.14.1. Data Model Name: `Device.X_RDK_ONT.Gem.{i}.EthernetFlow.Egress.C-VLAN.Dei`  
14.14.2. Type: uint32  
14.14.3. Access: Read  
14.14.4. Persistence: No  
14.14.5. Factory Default Value: 0  
14.14.6. Valid Range: 0 | 1  
14.14.7. Example: 0  
14.14.8. Usage: Egress C-VLAN drop eligible indicator.  

15. GPON Manager MUST implement the following RDK extension `Device.X_RDK_ONT` parameters: VEIP Ethernet Flow (VLAN Classification).

15.1. GPON Manager MUST report `Tagged`: ingress tagging mode of the flow.  
15.1.1. Data Model Name: `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Tagged`  
15.1.2. Type: string (enum)  
15.1.3. Access: Read/Write (the value is written to the vendor HAL)  
15.1.4. Persistence: No  
15.1.5. Factory Default Value: Untagged  
15.1.6. Valid Range: Untagged | Single | Double  
15.1.7. Example: Untagged  
15.1.8. Usage: Ingress tagging mode of the flow.  

15.2. GPON Manager MUST report `Vid`: ingress Q-VLAN VLAN ID.  
15.2.1. Data Model Name: `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Vid`  
15.2.2. Type: uint32  
15.2.3. Access: Read/Write (the value is written to the vendor HAL)  
15.2.4. Persistence: No  
15.2.5. Factory Default Value: 0  
15.2.6. Valid Range: 0 – 4095 (1 – 4094 usable)  
15.2.7. Example: 100  
15.2.8. Usage: Ingress Q-VLAN VLAN ID.  

15.3. GPON Manager MUST report `Pcp`: ingress Q-VLAN priority code point.  
15.3.1. Data Model Name: `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Pcp`  
15.3.2. Type: uint32  
15.3.3. Access: Read  
15.3.4. Persistence: No  
15.3.5. Factory Default Value: 0  
15.3.6. Valid Range: 0 – 7  
15.3.7. Example: 0  
15.3.8. Usage: Ingress Q-VLAN priority code point.  

15.4. GPON Manager MUST report `Dei`: ingress Q-VLAN drop eligible indicator.  
15.4.1. Data Model Name: `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Ingress.Q-VLAN.Dei`  
15.4.2. Type: uint32  
15.4.3. Access: Read  
15.4.4. Persistence: No  
15.4.5. Factory Default Value: 0  
15.4.6. Valid Range: 0 | 1  
15.4.7. Example: 0  
15.4.8. Usage: Ingress Q-VLAN drop eligible indicator.  

15.5. GPON Manager MUST report `Tagged`: egress tagging mode of the flow.  
15.5.1. Data Model Name: `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Tagged`  
15.5.2. Type: string (enum)  
15.5.3. Access: Read/Write (the value is written to the vendor HAL)  
15.5.4. Persistence: No  
15.5.5. Factory Default Value: Untagged  
15.5.6. Valid Range: Untagged | Single | Double  
15.5.7. Example: Untagged  
15.5.8. Usage: Egress tagging mode of the flow.  

15.6. GPON Manager MUST report `Vid`: egress Q-VLAN VLAN ID.  
15.6.1. Data Model Name: `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Vid`  
15.6.2. Type: uint32  
15.6.3. Access: Read/Write (the value is written to the vendor HAL)  
15.6.4. Persistence: No  
15.6.5. Factory Default Value: 0  
15.6.6. Valid Range: 0 – 4095 (1 – 4094 usable)  
15.6.7. Example: 100  
15.6.8. Usage: Egress Q-VLAN VLAN ID.  

15.7. GPON Manager MUST report `Pcp`: egress Q-VLAN priority code point.  
15.7.1. Data Model Name: `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Pcp`  
15.7.2. Type: uint32  
15.7.3. Access: Read  
15.7.4. Persistence: No  
15.7.5. Factory Default Value: 0  
15.7.6. Valid Range: 0 – 7  
15.7.7. Example: 0  
15.7.8. Usage: Egress Q-VLAN priority code point.  

15.8. GPON Manager MUST report `Dei`: egress Q-VLAN drop eligible indicator.  
15.8.1. Data Model Name: `Device.X_RDK_ONT.Veip.{i}.EthernetFlow.Egress.Q-VLAN.Dei`  
15.8.2. Type: uint32  
15.8.3. Access: Read  
15.8.4. Persistence: No  
15.8.5. Factory Default Value: 0  
15.8.6. Valid Range: 0 | 1  
15.8.7. Example: 0  
15.8.8. Usage: Egress Q-VLAN drop eligible indicator.
