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
 * @file gponmgr_telemetry.c
 * @brief Self-contained telemetry event module.
 *
 * Sections (all internal symbols are file-static):
 *   1. Event descriptor table  -- marker / priority / value-format
 *   2. Value formatter         -- builds T2 value string from ctx
 *   3. DML alarm / PLOAM mapper-- DML alarm fields and O-state
 *                                 transitions -> event ids
 *   4. T2 backend              -- t2_event_s() / t2_event_d()
 *   5. Dispatcher              -- public producer API entry points
 *
 * Marker names and severities track docs/08_TR181_Telemetry_Reference.md §2.
 */

#include "gponmgr_telemetry.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef ENABLE_FEATURE_TELEMETRY2_0
#include <telemetry_busmessage_sender.h>
#endif

/* ====================================================================== *
 * 1. Event descriptor table                                                *
 * ====================================================================== */

typedef enum {
    PRIO_INFO     = 0,
    PRIO_WARNING  = 1,
    PRIO_ERROR    = 2,
    PRIO_CRITICAL = 3
} priority_t;

typedef enum {
    FMT_NONE = 0,        /* value = ""                                  */
    FMT_INTF,            /* value = "Interface=<ifname>"                */
    FMT_ALARM            /* value = "RAISED"|"CLEARED",PM=<n>           */
} fmt_t;

typedef struct {
    const char *marker;
    priority_t  priority;
    fmt_t       fmt;
} event_desc_t;

static const event_desc_t k_table[XPON_TELEM_EVENT_ID_MAX] = {
    /* §2.1 ONU Status -------------------------------------------------- */
    [XPON_TELEM_ONU_LOS]                          = { "XPON_ONU_LOS",                          PRIO_CRITICAL, FMT_NONE  },
    [XPON_TELEM_ONU_DOWNSTREAM_SIGNAL_DETECTED]   = { "XPON_ONU_DOWNSTREAM_SIGNAL_DETECTED",   PRIO_INFO,     FMT_NONE  },
    [XPON_TELEM_ONU_REGISTRATION]                 = { "XPON_ONU_REGISTRATION",                 PRIO_INFO,     FMT_NONE  },
    [XPON_TELEM_ONU_DEREGISTRATION]               = { "XPON_ONU_DEREGISTRATION",               PRIO_WARNING,  FMT_NONE  },
    [XPON_TELEM_ONU_EMERGENCY_STOP]               = { "XPON_ONU_EMERGENCY_STOP",               PRIO_CRITICAL, FMT_NONE  },

    /* §2.2 Link Status ------------------------------------------------- */
    [XPON_TELEM_INTF_LINK_UP]                     = { "XPON_INTF_LINK_UP",                     PRIO_INFO,     FMT_INTF  },
    [XPON_TELEM_INTF_LINK_DOWN]                   = { "XPON_INTF_LINK_DOWN",                   PRIO_WARNING,  FMT_INTF  },
    [XPON_TELEM_PHY_STATUS_UP]                    = { "XPON_PHY_STATUS_UP",                    PRIO_INFO,     FMT_NONE  },
    [XPON_TELEM_PHY_STATUS_DOWN]                  = { "XPON_PHY_STATUS_DOWN",                  PRIO_CRITICAL, FMT_NONE  },

    /* §2.3 ITU-T ONU alarms -------------------------------------------- */
    [XPON_TELEM_ALARM_LOS]                        = { "XPON_ALARM_LOS",                        PRIO_CRITICAL, FMT_ALARM },
    [XPON_TELEM_ALARM_LOF]                        = { "XPON_ALARM_LOF",                        PRIO_CRITICAL, FMT_ALARM },
    [XPON_TELEM_ALARM_SF]                         = { "XPON_ALARM_SF",                         PRIO_ERROR,    FMT_ALARM },
    [XPON_TELEM_ALARM_SD]                         = { "XPON_ALARM_SD",                         PRIO_WARNING,  FMT_ALARM },
    [XPON_TELEM_ALARM_LCDG]                       = { "XPON_ALARM_LCDG",                       PRIO_ERROR,    FMT_ALARM },
    [XPON_TELEM_ALARM_TF]                         = { "XPON_ALARM_TF",                         PRIO_CRITICAL, FMT_ALARM },
    [XPON_TELEM_ALARM_SUF]                        = { "XPON_ALARM_SUF",                        PRIO_ERROR,    FMT_ALARM },
    [XPON_TELEM_ALARM_MEM]                        = { "XPON_ALARM_MEM",                        PRIO_WARNING,  FMT_ALARM },
    [XPON_TELEM_ALARM_DACT]                       = { "XPON_ALARM_DACT",                       PRIO_ERROR,    FMT_ALARM },
    [XPON_TELEM_ALARM_DIS]                        = { "XPON_ALARM_DIS",                        PRIO_CRITICAL, FMT_ALARM },
    [XPON_TELEM_ALARM_MIS]                        = { "XPON_ALARM_MIS",                        PRIO_ERROR,    FMT_ALARM },
    [XPON_TELEM_ALARM_PEE]                        = { "XPON_ALARM_PEE",                        PRIO_ERROR,    FMT_ALARM },
    [XPON_TELEM_ALARM_RDI]                        = { "XPON_ALARM_RDI",                        PRIO_WARNING,  FMT_ALARM },
    [XPON_TELEM_ALARM_ROGUE]                      = { "XPON_ALARM_ROGUE",                      PRIO_CRITICAL, FMT_ALARM },

    /* §2.4 System lifecycle ------------------------------------------- */
    [XPON_TELEM_SYSTEM_INIT_SUCCESS]              = { "XPON_SYSTEM_INIT_SUCCESS",              PRIO_INFO,     FMT_NONE  },
    [XPON_TELEM_SYSTEM_INIT_FAILURE]              = { "XPON_SYSTEM_INIT_FAILURE",              PRIO_CRITICAL, FMT_NONE  },
    [XPON_TELEM_SYSTEM_SHUTDOWN]                  = { "XPON_SYSTEM_SHUTDOWN",                  PRIO_INFO,     FMT_NONE  },
    [XPON_TELEM_SYSTEM_HAL_WRONG_PON_MODE]        = { "XPON_SYSTEM_HAL_WRONG_PON_MODE",        PRIO_CRITICAL, FMT_NONE  },

    /* §2.5 Error events ---------------------------------------------- */
    [XPON_TELEM_ERROR_HAL_CALL_FAILED]            = { "XPON_ERROR_HAL_CALL_FAILED",            PRIO_ERROR,    FMT_NONE  },
    [XPON_TELEM_ERROR_HAL_EVENT_SUBSCRIBE_FAILED] = { "XPON_ERROR_HAL_EVENT_SUBSCRIBE_FAILED", PRIO_ERROR,    FMT_NONE  },
    [XPON_TELEM_ERROR_HAL_EVENT_PARSE_FAILED]     = { "XPON_ERROR_HAL_EVENT_PARSE_FAILED",     PRIO_WARNING,  FMT_NONE  },
    [XPON_TELEM_ERROR_STATS_COLLECTION_FAILED]    = { "XPON_ERROR_STATS_COLLECTION_FAILED",    PRIO_WARNING,  FMT_NONE  },
    [XPON_TELEM_ERROR_RBUS_PUBLISH_FAILED]        = { "XPON_ERROR_RBUS_PUBLISH_FAILED",        PRIO_ERROR,    FMT_NONE  },
    [XPON_TELEM_ERROR_RBUS_REGISTER_FAILED]       = { "XPON_ERROR_RBUS_REGISTER_FAILED",       PRIO_ERROR,    FMT_NONE  },
    [XPON_TELEM_ERROR_STATE_MACHINE_START_FAILED] = { "XPON_ERROR_STATE_MACHINE_START_FAILED", PRIO_CRITICAL, FMT_NONE  },
};

static const event_desc_t *lookup(xponMgr_telemetry_event_id_t id)
{
    if ((unsigned)id >= XPON_TELEM_EVENT_ID_MAX) return NULL;
    if (k_table[id].marker == NULL) return NULL;
    return &k_table[id];
}

/* ====================================================================== *
 * 2. Value formatter                                                       *
 * ====================================================================== */

typedef struct {
    const char *ifname;
    bool        raised;
    ULONG       pm_instance;
} ctx_t;

static int format_value(const event_desc_t *desc,
                        const ctx_t        *ctx,
                        char               *out,
                        size_t              cap)
{
    int n = 0;

    if (out == NULL || cap == 0) return -1;
    out[0] = '\0';

    switch (desc->fmt) {
    case FMT_NONE:
        return 0;
    case FMT_INTF: {
        const char *ifn = (ctx && ctx->ifname && ctx->ifname[0]) ? ctx->ifname : "unknown";
        n = snprintf(out, cap, "Interface=%s", ifn);
        break;
    }
    case FMT_ALARM: {
        const char *state = (ctx && ctx->raised) ? "RAISED" : "CLEARED";
        n = snprintf(out, cap, "%s,PM=%lu", state,
                     (unsigned long)(ctx ? ctx->pm_instance : 0));
        break;
    }
    default:
        return -1;
    }
    if (n < 0 || (size_t)n >= cap) {
        out[cap - 1] = '\0';
        return -1;
    }
    return n;
}

/* ====================================================================== *
 * 3. DML alarm / PLOAM state mapper                                        *
 * ====================================================================== */

typedef struct {
    size_t                        offset;
    xponMgr_telemetry_event_id_t  id;
} alarm_map_t;

static const alarm_map_t k_alarm_map[] = {
    { offsetof(DML_ALARM, LOS),   XPON_TELEM_ALARM_LOS   },
    { offsetof(DML_ALARM, LOF),   XPON_TELEM_ALARM_LOF   },
    { offsetof(DML_ALARM, SF),    XPON_TELEM_ALARM_SF    },
    { offsetof(DML_ALARM, SD),    XPON_TELEM_ALARM_SD    },
    { offsetof(DML_ALARM, LCDG),  XPON_TELEM_ALARM_LCDG  },
    { offsetof(DML_ALARM, TF),    XPON_TELEM_ALARM_TF    },
    { offsetof(DML_ALARM, SUF),   XPON_TELEM_ALARM_SUF   },
    { offsetof(DML_ALARM, MEM),   XPON_TELEM_ALARM_MEM   },
    { offsetof(DML_ALARM, DACT),  XPON_TELEM_ALARM_DACT  },
    { offsetof(DML_ALARM, DIS),   XPON_TELEM_ALARM_DIS   },
    { offsetof(DML_ALARM, MIS),   XPON_TELEM_ALARM_MIS   },
    { offsetof(DML_ALARM, PEE),   XPON_TELEM_ALARM_PEE   },
    { offsetof(DML_ALARM, RDI),   XPON_TELEM_ALARM_RDI   },
    { offsetof(DML_ALARM, ROGUE), XPON_TELEM_ALARM_ROGUE },
};

static ALARM_ENUM alarm_field(const DML_ALARM *alarm, size_t offset)
{
    return *(const ALARM_ENUM *)((const char *)alarm + offset);
}

static const char *onu_state_str(REG_STATE_ENUM state)
{
    static const char *const names[] = { "O1", "O2", "O3", "O4", "O5", "O6", "O7", "O8", "O9" };
    if ((unsigned)state >= sizeof(names) / sizeof(names[0])) return "O?";
    return names[state];
}

/*
 * PLOAM O-state transition -> ONU status events (G.984.3 / G.987.3 / G.9807.1):
 *   O1 -> any            : downstream synchronisation acquired
 *   any -> O6            : intermittent LODS (downstream lost while operating)
 *   O2..O5 -> O1         : downstream lost before/while ranging
 *   O5 -> any            : ONU left Operation state
 *   any -> O5            : ONU entered Operation state
 *   any -> O7            : OLT disabled the ONU (Disable_Serial_Number)
 */
static size_t map_onu_state(REG_STATE_ENUM prev, REG_STATE_ENUM curr,
                            xponMgr_telemetry_event_id_t out[], size_t cap)
{
    size_t n = 0;

    if (prev == curr) return 0;

    if (prev == O1 && n < cap)
        out[n++] = XPON_TELEM_ONU_DOWNSTREAM_SIGNAL_DETECTED;
    if ((curr == O6 || (curr == O1 && prev >= O2 && prev <= O5)) && n < cap)
        out[n++] = XPON_TELEM_ONU_LOS;
    if (prev == O5 && n < cap)
        out[n++] = XPON_TELEM_ONU_DEREGISTRATION;
    if (curr == O5 && n < cap)
        out[n++] = XPON_TELEM_ONU_REGISTRATION;
    if (curr == O7 && n < cap)
        out[n++] = XPON_TELEM_ONU_EMERGENCY_STOP;
    return n;
}

/* ====================================================================== *
 * 4. T2 backend                                                            *
 * ====================================================================== */

static const char *prio_str(priority_t p)
{
    switch (p) {
    case PRIO_INFO:     return "INFO";
    case PRIO_WARNING:  return "WARNING";
    case PRIO_ERROR:    return "ERROR";
    case PRIO_CRITICAL: return "CRITICAL";
    default:            return "UNKNOWN";
    }
}

static int t2_send(const char *marker, const char *value, priority_t prio)
{
    if (marker == NULL) return 0;
    if (value == NULL) value = "";

    CcspTraceInfo(("[T2] %-9s %s%s%s\n",
                   prio_str(prio),
                   marker,
                   value[0] ? " " : "",
                   value));

#ifdef ENABLE_FEATURE_TELEMETRY2_0
    T2ERROR t2ret;
    if (value[0] == '\0') {
        /* t2_event_s() silently drops an empty string, so FMT_NONE events
         * are sent as a count marker with value 1. */
        t2ret = t2_event_d(marker, 1);
    } else {
        t2ret = t2_event_s(marker, value);
    }
    if (t2ret != T2ERROR_SUCCESS) {
        CcspTraceWarning(("telemetry: t2 send failed for marker %s (ret=%d)\n", marker, (int)t2ret));
        return -1;
    }
#endif
    return 0;
}

/* ====================================================================== *
 * 5. Dispatcher + Public API                                               *
 * ====================================================================== */

static struct {
    pthread_mutex_t mtx;
    bool            initialized;
    char            component[128];
} g_state = {
    .mtx         = PTHREAD_MUTEX_INITIALIZER,
    .initialized = false,
};

static int dispatch(xponMgr_telemetry_event_id_t id, const ctx_t *ctx)
{
    const event_desc_t *desc = lookup(id);
    char value[256];
    bool ready;

    if (desc == NULL) {
        CcspTraceWarning(("telemetry: unknown event id %d\n", (int)id));
        return -1;
    }

    pthread_mutex_lock(&g_state.mtx);
    ready = g_state.initialized;
    pthread_mutex_unlock(&g_state.mtx);

    if (!ready) return 0;

    if (format_value(desc, ctx, value, sizeof(value)) < 0) {
        CcspTraceWarning(("telemetry: failed to format value for %s\n", desc->marker));
        return -1;
    }
    return t2_send(desc->marker, value, desc->priority);
}

int xponMgr_telemetry_init(const char *component_name)
{
    if (component_name == NULL) return -1;

    pthread_mutex_lock(&g_state.mtx);
    if (g_state.initialized) {
        pthread_mutex_unlock(&g_state.mtx);
        return 0;
    }
    snprintf(g_state.component, sizeof(g_state.component), "%s", component_name);
    g_state.initialized = true;
    pthread_mutex_unlock(&g_state.mtx);

#ifdef ENABLE_FEATURE_TELEMETRY2_0
    t2_init(g_state.component);
#endif

    CcspTraceInfo(("telemetry: initialized component=%s\n", component_name));
    return 0;
}

int xponMgr_telemetry_cleanup(void)
{
    pthread_mutex_lock(&g_state.mtx);
    if (!g_state.initialized) {
        pthread_mutex_unlock(&g_state.mtx);
        return 0;
    }
    CcspTraceInfo(("telemetry: cleanup component=%s\n", g_state.component));
    g_state.initialized  = false;
    g_state.component[0] = '\0';
    pthread_mutex_unlock(&g_state.mtx);

#ifdef ENABLE_FEATURE_TELEMETRY2_0
    t2_uninit();
#endif
    return 0;
}

int xponMgr_telemetry_raise_simple(xponMgr_telemetry_event_id_t id)
{
    return dispatch(id, NULL);
}

int xponMgr_telemetry_raise_intf(xponMgr_telemetry_event_id_t id,
                                 const char *ifname)
{
    ctx_t ctx = { .ifname = ifname, .raised = false, .pm_instance = 0 };
    return dispatch(id, &ctx);
}

int xponMgr_telemetry_raise_alarms(const DML_ALARM *prev,
                                   const DML_ALARM *curr,
                                   ULONG pm_instance)
{
    size_t i;
    int raised = 0;

    if (prev == NULL || curr == NULL) return -1;

    for (i = 0; i < sizeof(k_alarm_map) / sizeof(k_alarm_map[0]); ++i) {
        ALARM_ENUM before = alarm_field(prev, k_alarm_map[i].offset);
        ALARM_ENUM after  = alarm_field(curr, k_alarm_map[i].offset);
        if (before == after) continue;

        ctx_t ctx = { .ifname = NULL,
                      .raised = (after == ACTIVE),
                      .pm_instance = pm_instance };
        if (dispatch(k_alarm_map[i].id, &ctx) == 0) ++raised;
    }
    return raised;
}

int xponMgr_telemetry_raise_onu_state(REG_STATE_ENUM prev,
                                      REG_STATE_ENUM curr)
{
    xponMgr_telemetry_event_id_t ids[5];
    size_t count, i;
    int raised = 0;

    if (prev == curr) return 0;

    CcspTraceInfo(("telemetry: PLOAM state %s -> %s\n", onu_state_str(prev), onu_state_str(curr)));
    count = map_onu_state(prev, curr, ids, sizeof(ids) / sizeof(ids[0]));
    for (i = 0; i < count; ++i)
        if (dispatch(ids[i], NULL) == 0) ++raised;
    return raised;
}
