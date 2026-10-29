/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file gponmgr_telemetry.h
 * @brief GPON (xPON) Manager Telemetry Public API
 *
 * The rest of the GPON Manager code base only ever interacts with telemetry
 * through this header. All marker-name selection, severity mapping, value
 * formatting, PLOAM/alarm transition mapping and T2 dispatch live inside
 * source/telemetry/ and are private to that module.
 *
 * Producer surface:
 *   - xponMgr_telemetry_raise_simple   (id)
 *   - xponMgr_telemetry_raise_intf     (id, ifname)
 *   - xponMgr_telemetry_raise_alarms   (prev, curr, pm_instance)
 *   - xponMgr_telemetry_raise_onu_state(prev, curr)
 *
 * See docs/07_Telemetry_Design.md and docs/08_TR181_Telemetry_Reference.md
 * for details.
 */

#ifndef GPONMGR_TELEMETRY_H
#define GPONMGR_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ansc_platform.h"
#include "gpon_apis.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ *
 * Event-id catalog (1-to-1 with 08_TR181_Telemetry_Reference §2)        *
 * ------------------------------------------------------------------ */

/**
 * @brief Telemetry event identifier.
 *
 * Each enumerator maps to exactly one T2 marker name (resolved inside
 * the telemetry module). Values are stable; new ids must be appended
 * before XPON_TELEM_EVENT_ID_MAX.
 */
typedef enum {
    /* §2.1 ONU Status Events (PLOAM activation state O1..O9) -------- */
    XPON_TELEM_ONU_LOS = 0,
    XPON_TELEM_ONU_DOWNSTREAM_SIGNAL_DETECTED,
    XPON_TELEM_ONU_REGISTRATION,
    XPON_TELEM_ONU_DEREGISTRATION,
    XPON_TELEM_ONU_EMERGENCY_STOP,

    /* §2.2 Interface Link Status Events ----------------------------- */
    XPON_TELEM_INTF_LINK_UP,
    XPON_TELEM_INTF_LINK_DOWN,
    XPON_TELEM_PHY_STATUS_UP,
    XPON_TELEM_PHY_STATUS_DOWN,

    /* §2.3 ITU-T G.984.3 / G.987.3 ONU Alarms (RAISED/CLEARED) ------ */
    XPON_TELEM_ALARM_LOS,
    XPON_TELEM_ALARM_LOF,
    XPON_TELEM_ALARM_SF,
    XPON_TELEM_ALARM_SD,
    XPON_TELEM_ALARM_LCDG,
    XPON_TELEM_ALARM_TF,
    XPON_TELEM_ALARM_SUF,
    XPON_TELEM_ALARM_MEM,
    XPON_TELEM_ALARM_DACT,
    XPON_TELEM_ALARM_DIS,
    XPON_TELEM_ALARM_MIS,
    XPON_TELEM_ALARM_PEE,
    XPON_TELEM_ALARM_RDI,
    XPON_TELEM_ALARM_ROGUE,

    /* §2.4 System Lifecycle ----------------------------------------- */
    XPON_TELEM_SYSTEM_INIT_SUCCESS,
    XPON_TELEM_SYSTEM_INIT_FAILURE,
    XPON_TELEM_SYSTEM_SHUTDOWN,
    XPON_TELEM_SYSTEM_HAL_WRONG_PON_MODE,

    /* §2.5 Error Events --------------------------------------------- */
    XPON_TELEM_ERROR_HAL_CALL_FAILED,
    XPON_TELEM_ERROR_HAL_EVENT_SUBSCRIBE_FAILED,
    XPON_TELEM_ERROR_HAL_EVENT_PARSE_FAILED,
    XPON_TELEM_ERROR_STATS_COLLECTION_FAILED,
    XPON_TELEM_ERROR_RBUS_PUBLISH_FAILED,
    XPON_TELEM_ERROR_RBUS_REGISTER_FAILED,
    XPON_TELEM_ERROR_STATE_MACHINE_START_FAILED,

    XPON_TELEM_EVENT_ID_MAX
} xponMgr_telemetry_event_id_t;

/* ------------------------------------------------------------------ *
 * Lifecycle and event producer API                                    *
 * ------------------------------------------------------------------ */

/**
 * @brief Initialize the telemetry module.
 *
 * Must be called once during application start-up before any
 * xponMgr_telemetry_raise_*() call. Events raised before init are
 * silently dropped.
 *
 * @param component_name Component name reported with every T2 event
 *                       (e.g. "GponManager"). Must not be NULL.
 * @return 0 on success, -1 on failure.
 */
int xponMgr_telemetry_init(const char *component_name);

/**
 * @brief Cleanup the telemetry module. Safe to call when not initialized.
 * @return 0 on success, -1 on failure.
 */
int xponMgr_telemetry_cleanup(void);

/**
 * @brief Raise a telemetry event that needs no caller-supplied context.
 *
 * Use this for aggregate PHY status, lifecycle events and generic
 * error events.
 *
 * @param id Event identifier from #xponMgr_telemetry_event_id_t.
 * @return 0 on success, -1 on failure.
 */
int xponMgr_telemetry_raise_simple(xponMgr_telemetry_event_id_t id);

/**
 * @brief Raise a telemetry event that names a single interface.
 *
 * Used for XPON_TELEM_INTF_LINK_UP and XPON_TELEM_INTF_LINK_DOWN.
 *
 * @param id     Event identifier.
 * @param ifname Interface name (e.g. "veip0"). NULL is reported as "unknown".
 * @return 0 on success, -1 on failure.
 */
int xponMgr_telemetry_raise_intf(xponMgr_telemetry_event_id_t id,
                                 const char *ifname);

/**
 * @brief Raise alarm events for every alarm whose state changed.
 *
 * The telemetry module internally maps each DML_ALARM field to its
 * telemetry event id and encodes the value as "RAISED,PM=<n>" or
 * "CLEARED,PM=<n>". Alarms whose state did not change are not raised,
 * so callers can invoke this after every HAL refresh.
 *
 * @param prev        Alarm snapshot before the HAL update. Must not be NULL.
 * @param curr        Alarm snapshot after the HAL update. Must not be NULL.
 * @param pm_instance X_RDK_ONT.PhysicalMedia instance number (1-based).
 * @return Number of events raised (>= 0), or -1 on invalid input.
 */
int xponMgr_telemetry_raise_alarms(const DML_ALARM *prev,
                                   const DML_ALARM *curr,
                                   ULONG pm_instance);

/**
 * @brief Raise ONU status events for a PLOAM activation-state transition.
 *
 * The telemetry module internally maps the (prev -> curr) O-state
 * transition to zero or more ONU status events (see
 * docs/07_Telemetry_Design.md §6). Calling with prev == curr raises nothing.
 *
 * @param prev Previous PLOAM registration state.
 * @param curr New PLOAM registration state.
 * @return Number of events raised (>= 0).
 */
int xponMgr_telemetry_raise_onu_state(REG_STATE_ENUM prev,
                                      REG_STATE_ENUM curr);

#ifdef __cplusplus
}
#endif

#endif /* GPONMGR_TELEMETRY_H */
