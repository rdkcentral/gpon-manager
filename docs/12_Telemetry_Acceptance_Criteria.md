# GPON Manager — Telemetry (T2) Acceptance Criteria

User story: GPON Manager sends RDK Telemetry 2.0 (T2) events for GPON/XGS-PON ONU activation, WAN interface link, PhysicalMedia alarms, lifecycle and runtime errors, so field issues can be monitored and correlated from the cloud.

**MoSCoW keys:** **MUST**: required for acceptance. **SHOULD**: expected, but a documented exception is acceptable. **COULD**: optional. **WON'T**: explicitly out of scope for this story.

**Scope:** the 38 supported `XPON_*` events. Event names, priorities and formats are defined in the GPON Manager TR-181 & Telemetry Reference (`docs/08_TR181_Telemetry_Reference.md` §2).

**Calls:** events without a value use `t2_event_d("<marker>", 1)`. Events with a value use `t2_event_s("<marker>", "<value>")`.

---

1. GPON Manager MUST send T2 events to monitor ONU activation (PLOAM) state changes. Events MUST be edge-detected in the HAL push event callback only; polled reads MUST NOT raise them.

1.1. GPON Manager MUST send a T2 event when the ONU activation state changes, for every change. This event MUST be sent first, before any milestone event below.  
1.1.1. Event Name: XPON_ONU_STATE_CHANGE  
1.1.2. Event Arguments(STATE): From=O<a>,To=O<b>, where a and b are 1–9 (e.g. From=O4,To=O5)  
1.1.3. Example: `t2_event_s("XPON_ONU_STATE_CHANGE", "From=O4,To=O5")`  
1.1.4. Priority: Info  
1.1.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.Ploam.RegistrationState` push event (`eventcb_PloamRegistrationState`).  

1.2. GPON Manager MUST send a T2 event when the HAL reports loss of downstream signal: the ONU enters O6, or falls back to O1 from O2–O5.  
1.2.1. Event Name: XPON_ONU_LOS  
1.2.2. Event Arguments: None  
1.2.3. Example: `t2_event_d("XPON_ONU_LOS", 1)`  
1.2.4. Priority: Critical  
1.2.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.Ploam.RegistrationState` push event (`eventcb_PloamRegistrationState`).  

1.3. GPON Manager MUST send a T2 event when downstream signal is acquired: the ONU leaves O1 for any other state.  
1.3.1. Event Name: XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED  
1.3.2. Event Arguments: None  
1.3.3. Example: `t2_event_d("XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED", 1)`  
1.3.4. Priority: Info  
1.3.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.Ploam.RegistrationState` push event (`eventcb_PloamRegistrationState`).  

1.4. GPON Manager MUST send a T2 event when the ONU reaches O5 (Operation): ranging is done, the ONU-ID is assigned and the ONU is in service.  
1.4.1. Event Name: XPON_ONU_REGISTRATION  
1.4.2. Event Arguments: None  
1.4.3. Example: `t2_event_d("XPON_ONU_REGISTRATION", 1)`  
1.4.4. Priority: Info  
1.4.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.Ploam.RegistrationState` push event (`eventcb_PloamRegistrationState`).  

1.5. GPON Manager MUST send a T2 event when the ONU leaves O5 (Operation) for any other state.  
1.5.1. Event Name: XPON_ONU_DEREGISTRATION  
1.5.2. Event Arguments: None  
1.5.3. Example: `t2_event_d("XPON_ONU_DEREGISTRATION", 1)`  
1.5.4. Priority: Warning  
1.5.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.Ploam.RegistrationState` push event (`eventcb_PloamRegistrationState`).  

1.6. GPON Manager MUST send a T2 event when the ONU enters O7 (Emergency Stop): the OLT ordered the transmitter off.  
1.6.1. Event Name: XPON_ONU_EMERGENCY_STOP  
1.6.2. Event Arguments: None  
1.6.3. Example: `t2_event_d("XPON_ONU_EMERGENCY_STOP", 1)`  
1.6.4. Priority: Critical  
1.6.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.Ploam.RegistrationState` push event (`eventcb_PloamRegistrationState`).  

1.7. GPON Manager MUST send a T2 event when the ONU leaves O7 (Emergency Stop): the OLT re-enabled the transmitter.  
1.7.1. Event Name: XPON_ONU_EMERGENCY_STOP_CLEARED  
1.7.2. Event Arguments: None  
1.7.3. Example: `t2_event_d("XPON_ONU_EMERGENCY_STOP_CLEARED", 1)`  
1.7.4. Priority: Info  
1.7.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.Ploam.RegistrationState` push event (`eventcb_PloamRegistrationState`).  

2. GPON Manager MUST send T2 events to monitor GPON WAN (VEIP) interface link and administrative status.

2.1. GPON Manager MUST send a T2 event when a VEIP interface comes up and WAN Manager has been told.  
2.1.1. Event Name: XPON_INTF_LINK_UP  
2.1.2. Event Arguments(INTERFACE): Interface=<ifname>, `unknown` when the name is empty (e.g. Interface=veip0)  
2.1.3. Example: `t2_event_s("XPON_INTF_LINK_UP", "Interface=veip0")`  
2.1.4. Priority: Info  
2.1.5. Timing: MUST be raised within 1 second of the link state machine transition, after WAN Manager has been told.  

2.2. GPON Manager MUST send a T2 event when a VEIP interface goes down and WAN Manager has been told.  
2.2.1. Event Name: XPON_INTF_LINK_DOWN  
2.2.2. Event Arguments(INTERFACE): Interface=<ifname>, `unknown` when the name is empty (e.g. Interface=veip0)  
2.2.3. Example: `t2_event_s("XPON_INTF_LINK_DOWN", "Interface=veip0")`  
2.2.4. Priority: Warning  
2.2.5. Timing: MUST be raised within 1 second of the link state machine transition, after WAN Manager has been told.  

2.3. GPON Manager MUST send a T2 event when the first VEIP interface comes up after all were down (0→1 links up).  
2.3.1. Event Name: XPON_PHY_STATUS_UP  
2.3.2. Event Arguments: None  
2.3.3. Example: `t2_event_d("XPON_PHY_STATUS_UP", 1)`  
2.3.4. Priority: Info  
2.3.5. Timing: MUST be raised within 1 second of the link state machine transition, after WAN Manager has been told.  

2.4. GPON Manager MUST send a T2 event when the last VEIP interface goes down (1→0 links up). The ONU has no WAN connectivity.  
2.4.1. Event Name: XPON_PHY_STATUS_DOWN  
2.4.2. Event Arguments: None  
2.4.3. Example: `t2_event_d("XPON_PHY_STATUS_DOWN", 1)`  
2.4.4. Priority: Critical  
2.4.5. Timing: MUST be raised within 1 second of the link state machine transition, after WAN Manager has been told.  

2.5. GPON Manager MUST send a T2 event when the OLT unlocks a VEIP over OMCI (AdministrativeState Lock→Unlock). The first event is compared with the state read from the HAL at start-up, so the first Unlock after a cold boot MUST raise this event, and a GponManager restart with the VEIP already Unlock MUST NOT.  
2.5.1. Event Name: XPON_INTF_ADMIN_UNLOCK  
2.5.2. Event Arguments(INTERFACE,VEIP): Interface=<ifname>,Veip=<n>, where n is the Veip instance and ifname is `unknown` when empty (e.g. Interface=veip0,Veip=1)  
2.5.3. Example: `t2_event_s("XPON_INTF_ADMIN_UNLOCK", "Interface=veip0,Veip=1")`  
2.5.4. Priority: Info  
2.5.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.Veip.{i}.AdministrativeState` push event (`eventcb_VeipAdministrativeState`).  

2.6. GPON Manager MUST send a T2 event when the OLT locks a VEIP over OMCI (AdministrativeState Unlock→Lock).  
2.6.1. Event Name: XPON_INTF_ADMIN_LOCK  
2.6.2. Event Arguments(INTERFACE,VEIP): Interface=<ifname>,Veip=<n>, where n is the Veip instance and ifname is `unknown` when empty (e.g. Interface=veip0,Veip=1)  
2.6.3. Example: `t2_event_s("XPON_INTF_ADMIN_LOCK", "Interface=veip0,Veip=1")`  
2.6.4. Priority: Warning  
2.6.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.Veip.{i}.AdministrativeState` push event (`eventcb_VeipAdministrativeState`).  

3. GPON Manager MUST send T2 events for the ITU-T G.984.3 / G.987.3 PhysicalMedia alarms on both RAISED (Inactive→Active) and CLEARED (Active→Inactive) transitions. Each alarm MUST have its own event, so one alarm can't mask another. An unchanged alarm MUST NOT send an event.

3.1. GPON Manager MUST send a T2 event for Loss Of Signal (no downstream optical signal).  
3.1.1. Event Name: XPON_ALARM_LOS  
3.1.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.1.3. Example: `t2_event_s("XPON_ALARM_LOS", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_LOS", "CLEARED,PM=1")`  
3.1.4. Priority: Critical  
3.1.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.2. GPON Manager MUST send a T2 event for Loss Of Frame (downstream frame delineation lost).  
3.2.1. Event Name: XPON_ALARM_LOF  
3.2.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.2.3. Example: `t2_event_s("XPON_ALARM_LOF", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_LOF", "CLEARED,PM=1")`  
3.2.4. Priority: Critical  
3.2.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.3. GPON Manager MUST send a T2 event for Signal Fail (downstream BER above the SignalFail threshold).  
3.3.1. Event Name: XPON_ALARM_SF  
3.3.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.3.3. Example: `t2_event_s("XPON_ALARM_SF", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_SF", "CLEARED,PM=1")`  
3.3.4. Priority: Error  
3.3.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.4. GPON Manager MUST send a T2 event for Signal Degrade (downstream BER above the SignalDegrade threshold).  
3.4.1. Event Name: XPON_ALARM_SD  
3.4.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.4.3. Example: `t2_event_s("XPON_ALARM_SD", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_SD", "CLEARED,PM=1")`  
3.4.4. Priority: Warning  
3.4.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.5. GPON Manager MUST send a T2 event for Loss of GEM Channel Delineation.  
3.5.1. Event Name: XPON_ALARM_LCDG  
3.5.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.5.3. Example: `t2_event_s("XPON_ALARM_LCDG", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_LCDG", "CLEARED,PM=1")`  
3.5.4. Priority: Error  
3.5.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.6. GPON Manager MUST send a T2 event for Transmitter Failure (ONU laser fault).  
3.6.1. Event Name: XPON_ALARM_TF  
3.6.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.6.3. Example: `t2_event_s("XPON_ALARM_TF", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_TF", "CLEARED,PM=1")`  
3.6.4. Priority: Critical  
3.6.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.7. GPON Manager MUST send a T2 event for Start-Up Failure (ranging or activation failed).  
3.7.1. Event Name: XPON_ALARM_SUF  
3.7.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.7.3. Example: `t2_event_s("XPON_ALARM_SUF", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_SUF", "CLEARED,PM=1")`  
3.7.4. Priority: Error  
3.7.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.8. GPON Manager MUST send a T2 event for Message Error Message (unknown PLOAM message).  
3.8.1. Event Name: XPON_ALARM_MEM  
3.8.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.8.3. Example: `t2_event_s("XPON_ALARM_MEM", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_MEM", "CLEARED,PM=1")`  
3.8.4. Priority: Warning  
3.8.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.9. GPON Manager MUST send a T2 event for Deactivate ONU-ID received from the OLT.  
3.9.1. Event Name: XPON_ALARM_DACT  
3.9.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.9.3. Example: `t2_event_s("XPON_ALARM_DACT", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_DACT", "CLEARED,PM=1")`  
3.9.4. Priority: Error  
3.9.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.10. GPON Manager MUST send a T2 event for Disabled ONU (the OLT disabled this serial number).  
3.10.1. Event Name: XPON_ALARM_DIS  
3.10.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.10.3. Example: `t2_event_s("XPON_ALARM_DIS", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_DIS", "CLEARED,PM=1")`  
3.10.4. Priority: Critical  
3.10.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.11. GPON Manager MUST send a T2 event for Link Mismatch.  
3.11.1. Event Name: XPON_ALARM_MIS  
3.11.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.11.3. Example: `t2_event_s("XPON_ALARM_MIS", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_MIS", "CLEARED,PM=1")`  
3.11.4. Priority: Error  
3.11.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.12. GPON Manager MUST send a T2 event for Physical Equipment Error reported by the OLT.  
3.12.1. Event Name: XPON_ALARM_PEE  
3.12.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.12.3. Example: `t2_event_s("XPON_ALARM_PEE", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_PEE", "CLEARED,PM=1")`  
3.12.4. Priority: Error  
3.12.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.13. GPON Manager MUST send a T2 event for Remote Defect Indication (the OLT sees a defective upstream signal).  
3.13.1. Event Name: XPON_ALARM_RDI  
3.13.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.13.3. Example: `t2_event_s("XPON_ALARM_RDI", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_RDI", "CLEARED,PM=1")`  
3.13.4. Priority: Warning  
3.13.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

3.14. GPON Manager MUST send a T2 event for Rogue ONU (transmitting outside the allocated window).  
3.14.1. Event Name: XPON_ALARM_ROGUE  
3.14.2. Event Arguments(STATE,PM): RAISED,PM=<n> or CLEARED,PM=<n>, where n is the PhysicalMedia instance (e.g. RAISED,PM=1)  
3.14.3. Example: `t2_event_s("XPON_ALARM_ROGUE", "RAISED,PM=1")` and `t2_event_s("XPON_ALARM_ROGUE", "CLEARED,PM=1")`  
3.14.4. Priority: Critical  
3.14.5. Timing: MUST be raised within 1 second of the HAL `Device.X_RDK_ONT.PhysicalMedia.{i}.Alarm.*` push event (`eventcb_PhysicalMediaAlarmsAll`).  

4. GPON Manager MUST send T2 events for system lifecycle transitions.

4.1. GPON Manager MUST send a T2 event once after initialisation completes: DML data, HAL connection, initial HAL data and the internal controller are up. It MUST also be sent when only the `Device.XPON` RBUS registration failed, because `Device.X_RDK_ONT` still works.  
4.1.1. Event Name: XPON_SYSTEM_INIT_SUCCESS  
4.1.2. Event Arguments: None  
4.1.3. Example: `t2_event_d("XPON_SYSTEM_INIT_SUCCESS", 1)`  
4.1.4. Priority: Info  

4.2. GPON Manager MUST send a T2 event when initialisation fails (DML data, HAL connection, initial HAL data read including an EPON module, internal controller, or `Cdm_Init`).  
4.2.1. Event Name: XPON_SYSTEM_INIT_FAILURE  
4.2.2. Event Arguments: None  
4.2.3. Example: `t2_event_d("XPON_SYSTEM_INIT_FAILURE", 1)`  
4.2.4. Priority: Critical  

4.3. GPON Manager MUST send a T2 event when a graceful shutdown is requested through the interactive `q` command loop.  
4.3.1. Event Name: XPON_SYSTEM_SHUTDOWN  
4.3.2. Event Arguments: None  
4.3.3. Example: `t2_event_d("XPON_SYSTEM_SHUTDOWN", 1)`  
4.3.4. Priority: Info  

4.4. GPON Manager MUST send a T2 event when the optical module reports an EPON `PonMode` that GPON Manager can't serve. The event MUST be latched: sent once, and sent again only after a valid ITU PON mode has been reported.  
4.4.1. Event Name: XPON_SYSTEM_HAL_WRONG_PON_MODE  
4.4.2. Event Arguments: None  
4.4.3. Example: `t2_event_d("XPON_SYSTEM_HAL_WRONG_PON_MODE", 1)`  
4.4.4. Priority: Critical  

5. GPON Manager MUST send T2 events for runtime errors.

5.1. GPON Manager MUST send a T2 event when the JSON-HAL client init or connect fails, a HAL `setParam` request can't be sent, or the HAL returns failure.  
5.1.1. Event Name: XPON_ERROR_HAL_CALL_FAILED  
5.1.2. Event Arguments: None  
5.1.3. Example: `t2_event_d("XPON_ERROR_HAL_CALL_FAILED", 1)`  
5.1.4. Priority: Error  

5.2. GPON Manager MUST send a T2 event when subscribing to HAL push events fails.  
5.2.1. Event Name: XPON_ERROR_HAL_EVENT_SUBSCRIBE_FAILED  
5.2.2. Event Arguments: None  
5.2.3. Example: `t2_event_d("XPON_ERROR_HAL_EVENT_SUBSCRIBE_FAILED", 1)`  
5.2.4. Priority: Error  

5.3. GPON Manager MUST send a T2 event when a HAL push event can't be parsed or mapped into the data model.  
5.3.1. Event Name: XPON_ERROR_HAL_EVENT_PARSE_FAILED  
5.3.2. Event Arguments: None  
5.3.3. Example: `t2_event_d("XPON_ERROR_HAL_EVENT_PARSE_FAILED", 1)`  
5.3.4. Priority: Warning  

5.4. GPON Manager MUST send a T2 event when the bulk HAL data read (`GponHal_get_data`) fails. The event MUST be latched: sent once per failure streak and re-armed by the next success.  
5.4.1. Event Name: XPON_ERROR_STATS_COLLECTION_FAILED  
5.4.2. Event Arguments: None  
5.4.3. Example: `t2_event_d("XPON_ERROR_STATS_COLLECTION_FAILED", 1)`  
5.4.4. Priority: Warning  

5.5. GPON Manager MUST send a T2 event when telling WAN Manager about a VEIP PHY status change fails. The event MUST be latched per state machine and re-armed by the next success.  
5.5.1. Event Name: XPON_ERROR_RBUS_PUBLISH_FAILED  
5.5.2. Event Arguments: None  
5.5.3. Example: `t2_event_d("XPON_ERROR_RBUS_PUBLISH_FAILED", 1)`  
5.5.4. Priority: Error  

5.6. GPON Manager MUST send a T2 event when registering the `Device.XPON` RBUS provider fails. GPON Manager MUST keep running and `Device.X_RDK_ONT` MUST stay available.  
5.6.1. Event Name: XPON_ERROR_RBUS_REGISTER_FAILED  
5.6.2. Event Arguments: None  
5.6.3. Example: `t2_event_d("XPON_ERROR_RBUS_REGISTER_FAILED", 1)`  
5.6.4. Priority: Error  

5.7. GPON Manager MUST send a T2 event when the VEIP link state machine can't be started (allocation or thread creation failed).  
5.7.1. Event Name: XPON_ERROR_STATE_MACHINE_START_FAILED  
5.7.2. Event Arguments: None  
5.7.3. Example: `t2_event_d("XPON_ERROR_STATE_MACHINE_START_FAILED", 1)`  
5.7.4. Priority: Critical  

6. GPON Manager MUST raise the ONU milestone events from the `From`/`To` pair of `XPON_ONU_STATE_CHANGE` using the same rules for every PON mode.

6.1. GPON Manager MUST send `XPON_ONU_STATE_CHANGE` first, then the matching milestones in this order: EMERGENCY_STOP_CLEARED, DOWNSTREAM_SIGNAL_DETECTED, LOS, DEREGISTRATION, REGISTRATION, EMERGENCY_STOP.  

6.2. GPON Manager MUST NOT send any ONU event when the reported state is unchanged.  

6.3. GPON Manager MUST map vendor sub-states to their parent state by prefix (for example `O1.2` → O1, `O5.1` → O5), and a literal `O2-3` to O2.  

6.4. GPON Manager MUST raise the following milestones for the GPON (G.984.3) transitions:  
6.4.1. O1 → O2: XPON_ONU_STATE_CHANGE (From=O1,To=O2), XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED.  
6.4.2. O4 → O5: XPON_ONU_STATE_CHANGE (From=O4,To=O5), XPON_ONU_REGISTRATION.  
6.4.3. O2 / O3 / O4 → O1: XPON_ONU_STATE_CHANGE, XPON_ONU_LOS.  
6.4.4. O5 → O2: XPON_ONU_STATE_CHANGE (From=O5,To=O2), XPON_ONU_DEREGISTRATION.  
6.4.5. O5 → O6: XPON_ONU_STATE_CHANGE (From=O5,To=O6), XPON_ONU_LOS, XPON_ONU_DEREGISTRATION.  
6.4.6. O6 → O5: XPON_ONU_STATE_CHANGE (From=O6,To=O5), XPON_ONU_REGISTRATION.  
6.4.7. O2 / O3 / O4 / O6 → O7: XPON_ONU_STATE_CHANGE, XPON_ONU_EMERGENCY_STOP.  
6.4.8. O5 → O7: XPON_ONU_STATE_CHANGE (From=O5,To=O7), XPON_ONU_DEREGISTRATION, XPON_ONU_EMERGENCY_STOP.  
6.4.9. O7 → O1 / O2: XPON_ONU_STATE_CHANGE, XPON_ONU_EMERGENCY_STOP_CLEARED.  
6.4.10. O2 → O3, O3 → O4, O3 → O2, O4 → O2, O6 → O4, O6 → O1: XPON_ONU_STATE_CHANGE only.  

6.5. GPON Manager MUST raise the following milestones for the XGS-PON (G.9807.1) transitions. The combined O2-3 state is reported as O2 or O3, and both MUST give the same milestones:  
6.5.1. O1 → O2-3: XPON_ONU_STATE_CHANGE (From=O1,To=O2 or To=O3), XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED.  
6.5.2. O4 → O5: XPON_ONU_STATE_CHANGE (From=O4,To=O5), XPON_ONU_REGISTRATION.  
6.5.3. O2-3 / O4 → O1: XPON_ONU_STATE_CHANGE, XPON_ONU_LOS.  
6.5.4. O5 → O2-3: XPON_ONU_STATE_CHANGE, XPON_ONU_DEREGISTRATION.  
6.5.5. O5 → O6: XPON_ONU_STATE_CHANGE (From=O5,To=O6), XPON_ONU_LOS, XPON_ONU_DEREGISTRATION.  
6.5.6. O6 → O5: XPON_ONU_STATE_CHANGE (From=O6,To=O5), XPON_ONU_REGISTRATION.  
6.5.7. O2-3 / O4 → O7: XPON_ONU_STATE_CHANGE, XPON_ONU_EMERGENCY_STOP.  
6.5.8. O5 → O7: XPON_ONU_STATE_CHANGE (From=O5,To=O7), XPON_ONU_DEREGISTRATION, XPON_ONU_EMERGENCY_STOP.  
6.5.9. O7 → O1: XPON_ONU_STATE_CHANGE (From=O7,To=O1), XPON_ONU_EMERGENCY_STOP_CLEARED.  
6.5.10. O2-3 → O4, O4 → O2-3, O6 → O1, O2 ↔ O3: XPON_ONU_STATE_CHANGE only.  

6.6. GPON Manager MUST apply the same rules to pairs the standards don't define (for example O1 → O5 when intermediate states were not pushed by the HAL).  

6.7. GPON Manager WON'T report the BBF combined `O2-3` value in `XPON_ONU_STATE_CHANGE`; XGS-PON O2-3 is reported as `O2` or `O3`.  

7. GPON Manager MUST send all T2 events in a consistent and non-flooding way.

7.1. GPON Manager MUST register with T2 as component `GponManager` and use the `XPON_` prefix for every marker.  

7.2. GPON Manager MUST send T2 events only when built with `--enable-telemetry2-0` (`ENABLE_FEATURE_TELEMETRY2_0`). Without it, the API MUST stay linked and only write the log line.  

7.3. GPON Manager MUST log every event as `[T2] <PRIORITY> <marker> [value]` in the GPON Manager log, whether or not T2 is compiled in.  

7.4. GPON Manager MUST raise ONU, alarm and VEIP admin events only from the HAL push event callbacks.  
7.4.1. TR-181 polling (CCSP/RBUS gets, `GponHal_get_data()`) MUST NOT raise these events.  
7.4.2. After an instance is seeded, a change seen first by a poll MUST NOT hide the event: the callbacks compare against their own event-fed state, not against the polled value.  

7.5. GPON Manager MUST NOT send false fault events at start-up. Each PLOAM / PhysicalMedia / VEIP instance is seeded on its first HAL push event from the data-model value held just before it (read from the HAL at start-up; O1, alarms Inactive and VEIP Lock until the HAL reports otherwise). A GponManager restart with the ONU already in O5 MUST NOT re-send REGISTRATION.  

7.6. GPON Manager MUST latch `XPON_ERROR_STATS_COLLECTION_FAILED`, `XPON_ERROR_RBUS_PUBLISH_FAILED` and `XPON_SYSTEM_HAL_WRONG_PON_MODE`: one event per failure streak, re-armed by the next success.  

7.7. GPON Manager MUST drop events raised before the telemetry module is initialised or after it is cleaned up, without crashing.  

7.8. GPON Manager SHOULD NOT send any event while the ONU stays in O5 with no alarm, link or state change.  

7.9. GPON Manager WON'T send periodic statistics (optical levels, counters) as T2 events in this story. These are read from the TR-181 data model.
