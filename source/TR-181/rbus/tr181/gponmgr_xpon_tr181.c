/**
 * @file gponmgr_xpon_tr181.c
 * @brief BBF TR-181 Device.XPON RBUS provider backed by GPON_DML_DATA.
 *
 * Every get locks the DML data, refreshes the HAL table it needs (cached for
 * XPON_CACHE_TTL_SEC), fills the RBUS value and returns.
 * A few values have no HAL field and come from elsewhere: ONU.Enable from the
 * JSON-HAL client connection state, EthernetUNI.Stats from Linux sysfs, and
 * names/aliases/LowerLayers/Upstream are constants or computed here.
 * Table rows are kept in sync by xpon_sync(), one table at a time.
 */

#include "gponmgr_xpon_tr181.h"
#include "../gponmgr_xpon_rbus.h"
#include "gponmgr_dml_data.h"
#include "gponmgr_dml_hal.h"
#include <rbus.h>
#include <ctype.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define XPON_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define XPON_BAD_VALUE    RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION   /* HAL value outside the BBF range. */
#define XPON_ANI          "Device.XPON.ONU.1.ANI.1." // Only single ONU and ANI considered for now.

#ifndef XPON_CACHE_TTL_SEC
#define XPON_CACHE_TTL_SEC 30       /* HAL query cache lifetime in seconds; override with -D. */
#endif

/** @brief HAL tables that are cached independently. */
typedef enum
{
    XPON_HAL_PM, XPON_HAL_GTC, XPON_HAL_PLOAM, XPON_HAL_OMCI, XPON_HAL_GEM, XPON_HAL_VEIP,
    XPON_HAL_COUNT
} XponHalTable;

/** @brief Cache state of one HAL table. */
typedef struct
{
    bool   valid;       /**< Data in GPON_DML_DATA came from a successful HAL read. */
    time_t timestamp;   /**< CLOCK_MONOTONIC seconds of that read. */
} XponCacheEntry;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;     /* Guards g_handle and the row arrays. */
static rbusHandle_t g_handle;                                   /* NULL when not initialized. */
static pthread_mutex_t g_cacheMutex = PTHREAD_MUTEX_INITIALIZER;  /* Guards g_cache; taken after the DML lock. */
static XponCacheEntry g_cache[XPON_HAL_COUNT];
static uint32_t g_phyRows[GPON_DATA_MAX_PM];                    /* Registered Transceiver instance per slot; 0 = none. */
static uint32_t g_gemRows[GPON_DATA_MAX_GEM];                   /* Registered GEM Port instance per slot; 0 = none. */
static uint32_t g_veipRows[GPON_DATA_MAX_VEIP];                 /* Registered EthernetUNI instance per slot; 0 = none. */

static const char *const g_ifStates[] = {"Up", "Down", "Unknown", "Dormant", "NotPresent", "LowerLayerDown", "Error"};

static rbusError_t xpon_base_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_unsupported_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_onu_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_veip_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_uni_stats_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_ani_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_activation_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_gtc_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_ploam_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_omci_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_gem_totals_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_gem_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_threshold_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_alarm_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_transceiver_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_unsupported_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t *options);
static rbusError_t xpon_readonly_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t *options);
static rbusError_t xpon_subscribe(rbusHandle_t handle, rbusEventSubAction_t action, const char *name, rbusFilter_t filter, int32_t interval, bool *autoPublish);
static rbusError_t xpon_unsupported_subscribe(rbusHandle_t handle, rbusEventSubAction_t action, const char *name, rbusFilter_t filter, int32_t interval, bool *autoPublish);

/* Full BBF 2.21 Device.XPON tree; unsupported entries use the xpon_unsupported_* handlers. */
static rbusDataElement_t g_elements[] = {
    {"Device.XPON.ONU.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}}, /* Not supported yet: HAL software-image inventory is unavailable; no rows are registered. */
    {"Device.XPON.ONU.{i}.ANI.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},

    {"Device.XPON.ONUNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_base_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_onu_get_handler, xpon_readonly_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Read-only here (BBF R/W): true while the JSON-HAL client is connected. */
    {"Device.XPON.ONU.{i}.PowerCapability", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerStatus", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_onu_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* CPE-assigned constant "ONU<i>". */
    {"Device.XPON.ONU.{i}.Version", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EquipmentID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImageNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_base_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNINumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_base_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANINumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_base_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Alias", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, xpon_readonly_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Read-only: "cpe-" + upper-case VEIP InterfaceName. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Description", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, xpon_readonly_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Read-only default text built from Alias. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.LastChange", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Computed by GPON Manager from VEIP OperationalState changes. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.LowerLayers", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, xpon_readonly_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Always empty: BBF expects LowerLayers unused for EthernetUNI. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Upstream", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Always true: the VEIP is the gateway WAN interface. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.ANIs", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainName", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.Reset", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_uni_stats_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Linux sysfs /sys/class/net/<InterfaceName>/statistics/tx_bytes. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_uni_stats_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Linux sysfs /sys/class/net/<InterfaceName>/statistics/rx_bytes. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_uni_stats_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Linux sysfs /sys/class/net/<InterfaceName>/statistics/tx_packets. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_uni_stats_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Linux sysfs /sys/class/net/<InterfaceName>/statistics/rx_packets. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_uni_stats_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Linux sysfs /sys/class/net/<InterfaceName>/statistics/tx_errors. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_uni_stats_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Linux sysfs /sys/class/net/<InterfaceName>/statistics/rx_errors. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_uni_stats_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Linux sysfs /sys/class/net/<InterfaceName>/statistics/tx_dropped. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_uni_stats_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Linux sysfs /sys/class/net/<InterfaceName>/statistics/rx_dropped. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_uni_stats_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Linux sysfs /sys/class/net/<InterfaceName>/statistics/multicast. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnknownProtoPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.ID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.Version", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.IsCommitted", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.IsActive", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.IsValid", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, xpon_readonly_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Read-only (BBF R/W): writing would switch the PON link off. Get needs WAN_MANAGER_UNIFICATION_ENABLED. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Alias", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, xpon_readonly_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Read-only. Get not supported without WAN_MANAGER_UNIFICATION_ENABLED; access logs a warning. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* CPE-assigned constant "ANI<i>". */
    {"Device.XPON.ONU.{i}.ANI.{i}.LastChange", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported without WAN_MANAGER_UNIFICATION_ENABLED; access logs a warning. */
    {"Device.XPON.ONU.{i}.ANI.{i}.PONMode", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* HAL PhysicalMedia.PonMode of the active medium; "Unknown" if not valid. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TransceiverNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.SFPReferenceList", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Always empty: Device.SFPs.SFPCage is not implemented on this platform. */

    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.UnknownProtoPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUState", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_activation_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.VendorID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_activation_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.SerialNumber", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_activation_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_activation_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.Password", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.HexadecimalPassword", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECBytes", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECCodewords", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UncorrectableFECCodewords", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.TotalFECCodewords", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.PSBdHECErrorCount", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.HeaderHECErrorCount", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UnknownProfile", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_totals_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Sum of HAL Gem.{i}.TransmittedFrames. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_totals_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Sum of HAL Gem.{i}.ReceivedFrames. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FrameHeaderHECErrors", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.KeyErrors", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.MICErrors", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ploam_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.DownstreamMessageCount", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ploam_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.RangingTime", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.UpstreamMessageCount", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ploam_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.BaselineMessagesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_omci_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.ExtendedMessagesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_omci_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.MICErrors", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_omci_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.PortNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_totals_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.Direction", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Unidirectional mapping is not supported yet: HAL omits the traffic direction. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortType", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* From HAL Gem.{i}.TrafficType: Bidirectional -> unicast, Unidirectional -> multicast. */

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PM.FramesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalFail", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_threshold_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalDegrade", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_threshold_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOS", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LOF", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SF", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SD", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LCDG", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.TF", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.SUF", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MEM", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DACT", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.DIS", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.MIS", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.PEE", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.RDI", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LODS", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no HAL LODS alarm (LOF is not equivalent). */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.ROGUE", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.ID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Identifier", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* 3 (SFP, SFF-8024) when HAL Cage is SFP; rejected otherwise. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorName", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorPartNumber", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.VendorRevision", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.PONMode", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Connector", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateDownstream", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.NominalBitRateUpstream", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.RxPower", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.TxPower", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Voltage", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Bias", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Temperature", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementCapability", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.OLTSleepAllow", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementStatus", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.ITransInit", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.ITxInit", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.MaxSleepInterval", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.MinAwareInterval", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.MinActiveHeldInterval", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.MaxSleepIntervalExtension", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_unsupported_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
};


static const size_t g_elementCount = sizeof(g_elements) / sizeof(g_elements[0]);

/* ---------------------------------------------------------------------- *
 * Small value helpers                                                     *
 * ---------------------------------------------------------------------- */

/**
 * @brief Set a string value from a HAL buffer after checking its length.
 * @param value     Value to set.
 * @param text      HAL buffer.
 * @param size      Size of @p text in bytes.
 * @param maxLength BBF maximum string length.
 * @return RBUS_ERROR_SUCCESS, or XPON_BAD_VALUE if unterminated or too long.
 */
static rbusError_t xpon_set_string(rbusValue_t value, const char *text, size_t size, size_t maxLength)
{
    size_t length = strnlen(text, size);
    if (length == size || length > maxLength)
        return XPON_BAD_VALUE;
    rbusValue_SetString(value, text);
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Set a string value from a HAL enum index.
 * @param value Value to set.
 * @param index HAL enum value, used as an index into @p names.
 * @param names BBF enumeration strings.
 * @param count Number of entries in @p names.
 * @return RBUS_ERROR_SUCCESS, or XPON_BAD_VALUE if @p index is out of range.
 */
static rbusError_t xpon_set_enum(rbusValue_t value, unsigned int index, const char *const *names, size_t count)
{
    if (index >= count)
        return XPON_BAD_VALUE;
    rbusValue_SetString(value, names[index]);
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Set the 12-character BBF serial number from the PLOAM serial.
 *
 * Accepts 16 hex digits (4 vendor ID bytes + 4-byte VSSN) or the canonical
 * 4 printable characters + 8 hex digits.
 *
 * @param value Value to set.
 * @param sn    PLOAM SerialNumber buffer.
 * @param size  Size of @p sn in bytes.
 * @retval RBUS_ERROR_SUCCESS           Value set, e.g. "ABCD1234ABCD".
 * @retval XPON_BAD_VALUE               Bad hex digit or non-printable vendor ID.
 * @retval RBUS_ERROR_INVALID_OPERATION Length is neither 16 nor 12.
 */
static rbusError_t xpon_set_serial(rbusValue_t value, const char *sn, size_t size)
{
    char serial[13];
    size_t i, length = strnlen(sn, size);

    if (length == 16)
    {
        for (i = 0; i < 16; ++i)
            if (!isxdigit((unsigned char)sn[i]))
                return XPON_BAD_VALUE;
        /* First 8 hex digits are the 4 ASCII vendor ID characters. */
        for (i = 0; i < 4; ++i)
        {
            char hex[3] = {sn[i * 2], sn[i * 2 + 1], '\0'};
            long character = strtol(hex, NULL, 16);
            if (character < 33 || character > 126)
                return XPON_BAD_VALUE;
            serial[i] = (char)character;
        }
        for (i = 0; i < 8; ++i)
            serial[4 + i] = (char)toupper((unsigned char)sn[8 + i]);
    }
    else if (length == 12)
    {
        for (i = 0; i < 12; ++i)
            if (i < 4 ? ((unsigned char)sn[i] < 33 || (unsigned char)sn[i] > 126) : !isxdigit((unsigned char)sn[i]))
                return XPON_BAD_VALUE;
        memcpy(serial, sn, 12);
    }
    else
    {
        CcspTraceWarning(("XPON: unsupported serial number format (length %zu)\n", length));
        return RBUS_ERROR_INVALID_OPERATION;
    }
    serial[12] = '\0';
    rbusValue_SetString(value, serial);
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Pick the PhysicalMedia row that backs ANI-level parameters.
 * @param data Locked DML data.
 * @return The only populated row; else the only Active row; NULL if none or ambiguous.
 */
static DML_PHY_MEDIA *xpon_active_phy(GPON_DML_DATA *data)
{
    DML_PHY_MEDIA *only = NULL, *active = NULL;
    size_t i, count = 0, activeCount = 0;

    for (i = 0; i < GPON_DATA_MAX_PM; ++i)
    {
        if (!data->gpon.PhysicalMedia.pdata[i])
            continue;
        only = &data->gpon.PhysicalMedia.pdata[i]->dml;
        ++count;
        if (only->RedundancyState == Active)
        {
            active = only;
            ++activeCount;
        }
    }
    return count == 1 ? only : activeCount == 1 ? active : NULL;
}

/**
 * @brief Set the BBF PONMode from HAL PhysicalMedia.PonMode.
 * @param value Value to set.
 * @param media PhysicalMedia row, or NULL when no unique medium exists.
 * @return RBUS_ERROR_SUCCESS ("Unknown" if @p media is NULL or PonModeValid is false), or XPON_BAD_VALUE.
 */
static rbusError_t xpon_set_pon_mode(rbusValue_t value, const DML_PHY_MEDIA *media)
{
    static const char *const modes[] = {"G-PON", "XG-PON", "NG-PON2", "XGS-PON"};

    if (!media || !media->PonModeValid)
    {
        rbusValue_SetString(value, "Unknown");
        return RBUS_ERROR_SUCCESS;
    }
    return xpon_set_enum(value, media->PonMode, modes, XPON_COUNT(modes));
}

/**
 * @brief Read the instance number that follows @p table in an RBUS name.
 * @param name  Full parameter name.
 * @param table Table segment including dots, e.g. ".ONU.".
 * @return Instance number, or 0 if not found.
 */
static uint32_t xpon_instance(const char *name, const char *table)
{
    const char *row = strstr(name, table);
    unsigned int instance = 0;

    if (!row || sscanf(row + strlen(table), "%u", &instance) != 1)
        return 0;
    return instance;
}

/* ---------------------------------------------------------------------- *
 * HAL query cache (TTL)                                                  *
 * ---------------------------------------------------------------------- */

/**
 * @brief Read one HAL table into the DML data unless the cached copy is younger than XPON_CACHE_TTL_SEC.
 * @param data  DML data; caller holds the DML lock.
 * @param table HAL table to refresh.
 * @retval RBUS_ERROR_SUCCESS                     Data in @p data is fresh.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE HAL read failed; the cache entry stays invalid.
 */
static rbusError_t xpon_hal_get(GPON_DML_DATA *data, XponHalTable table)
{
    struct timespec now;
    ANSC_STATUS status;
    bool fresh;

    clock_gettime(CLOCK_MONOTONIC, &now);       /* Monotonic: immune to NTP/wall-clock jumps. */
    pthread_mutex_lock(&g_cacheMutex);
    fresh = g_cache[table].valid && now.tv_sec - g_cache[table].timestamp < XPON_CACHE_TTL_SEC;
    pthread_mutex_unlock(&g_cacheMutex);
    if (fresh)
        return RBUS_ERROR_SUCCESS;

    switch (table)
    {
        case XPON_HAL_PM:    status = GponHal_get_pm(&data->gpon.PhysicalMedia); break;
        case XPON_HAL_GTC:   status = GponHal_get_gtc(&data->gpon.Gtc); break;
        case XPON_HAL_PLOAM: status = GponHal_get_ploam(&data->gpon.Ploam); break;
        case XPON_HAL_OMCI:  status = GponHal_get_omci(&data->gpon.Omci); break;
        case XPON_HAL_GEM:   status = GponHal_get_gem(&data->gpon.Gem); break;
        case XPON_HAL_VEIP:  status = GponHal_get_veip(&data->gpon.Veip); break;
        default:             return RBUS_ERROR_INVALID_INPUT;
    }
    if (status != ANSC_STATUS_SUCCESS)
    {
        CcspTraceWarning(("XPON: HAL read failed for table %d\n", table));
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    pthread_mutex_lock(&g_cacheMutex);
    g_cache[table].valid = true;
    g_cache[table].timestamp = now.tv_sec;
    pthread_mutex_unlock(&g_cacheMutex);
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Mark every HAL table stale (e.g. on ONU state change).
 * @note Safe to call with or without the DML lock held.
 */
void GponMgrXponTr181_InvalidateCache(void)
{
    pthread_mutex_lock(&g_cacheMutex);
    memset(g_cache, 0, sizeof(g_cache));
    pthread_mutex_unlock(&g_cacheMutex);
}

/* ---------------------------------------------------------------------- *
 * Get handlers: lock DML -> read HAL -> fill value -> unlock -> return    *
 * ---------------------------------------------------------------------- */

/**
 * @brief Get handler for ONUNumberOfEntries and the ONU-level NumberOfEntries counters.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set (ONU/ANI = 1, SoftwareImage = 0, EthernetUNI = VEIP rows).
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE VEIP HAL read failed.
 */
static rbusError_t xpon_base_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *param = strrchr(rbusProperty_GetName(property), '.') + 1;
    GPON_DML_DATA *data;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    uint32_t count = 0;
    size_t i;
    (void)handle;
    (void)options;

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    rbusValue_Init(&value);

    if (strcmp(param, "EthernetUNINumberOfEntries") == 0)
    {
        if (xpon_hal_get(data, XPON_HAL_VEIP) != RBUS_ERROR_SUCCESS)
            rc = RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
        for (i = 0; i < GPON_DATA_MAX_VEIP; ++i)
            if (data->gpon.Veip.pdata[i])
                ++count;
    }
    else if (strcmp(param, "ONUNumberOfEntries") == 0 || strcmp(param, "ANINumberOfEntries") == 0)
        count = 1;                                  /* Single ONU with a single ANI. */
    /* SoftwareImageNumberOfEntries stays 0: no HAL inventory. */
    rbusValue_SetUInt32(value, count);

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.Enable and ONU.{i}.Name.
 *
 * Enable reflects the JSON-HAL client connection (json_hal_is_client_connected());
 * Name is the CPE-assigned constant "ONU<i>". Neither needs a HAL table read.
 *
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS           Value set.
 * @retval RBUS_ERROR_INVALID_OPERATION Unknown parameter.
 */
static rbusError_t xpon_onu_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *name = rbusProperty_GetName(property);
    const char *param = strrchr(name, '.') + 1;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    char text[16];
    (void)handle;
    (void)options;

    rbusValue_Init(&value);
    if (strcmp(param, "Enable") == 0)
        rbusValue_SetBoolean(value, json_hal_is_client_connected() != 0);
    else if (strcmp(param, "Name") == 0)
    {
        snprintf(text, sizeof(text), "ONU%u", xpon_instance(name, ".ONU."));
        rbusValue_SetString(value, text);
    }
    else
        rc = RBUS_ERROR_INVALID_OPERATION;

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.ANI.{i}.Transceiver.{i}.* parameters.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE PhysicalMedia HAL read failed.
 * @retval RBUS_ERROR_ELEMENT_DOES_NOT_EXIST      No PhysicalMedia row for this instance.
 * @retval XPON_BAD_VALUE                         HAL value outside the BBF range.
 */
static rbusError_t xpon_transceiver_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    static const char *const connectors[] = {"LC", "ST", "FC", "SC", "MT-RJ"};
    const char *name = rbusProperty_GetName(property);
    const char *param = strrchr(name, '.') + 1;
    const char *row = strstr(name, ".Transceiver.");
    GPON_DML_DATA *data;
    DML_PHY_MEDIA *media = NULL;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    uint32_t instance = 0;
    size_t i, slot = 0;
    (void)handle;
    (void)options;

    if (!row || sscanf(row, ".Transceiver.%u", &instance) != 1)
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_PM) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    for (i = 0; i < GPON_DATA_MAX_PM && !media; ++i)
        if (data->gpon.PhysicalMedia.pdata[i] && data->gpon.PhysicalMedia.pdata[i]->dml.uInstanceNumber == instance)
        {
            media = &data->gpon.PhysicalMedia.pdata[i]->dml;
            slot = i;
        }
    if (!media)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    rbusValue_Init(&value);

    if (strcmp(param, "ID") == 0)
        rbusValue_SetUInt32(value, (uint32_t)slot);                 /* Zero-based slot index. */
    else if (strcmp(param, "VendorName") == 0)
        rc = xpon_set_string(value, media->ModuleVendor, sizeof(media->ModuleVendor), 256);
    else if (strcmp(param, "VendorPartNumber") == 0)
        rc = xpon_set_string(value, media->ModuleName, sizeof(media->ModuleName), 256);
    else if (strcmp(param, "VendorRevision") == 0)
        rc = xpon_set_string(value, media->ModuleVersion, sizeof(media->ModuleVersion), 256);
    else if (strcmp(param, "NominalBitRateDownstream") == 0)
        rbusValue_SetUInt32(value, media->NominalBitRateDownstream);
    else if (strcmp(param, "NominalBitRateUpstream") == 0)
        rbusValue_SetUInt32(value, media->NominalBitRateUpstream);
    else if (strcmp(param, "Bias") == 0)
        rbusValue_SetUInt32(value, media->Bias.CurrentBias);
    else if (strcmp(param, "RxPower") == 0)
        rbusValue_SetInt32(value, media->RxPower.SignalLevel);
    else if (strcmp(param, "TxPower") == 0)
        rbusValue_SetInt32(value, media->TxPower.SignalLevel);
    else if (strcmp(param, "Temperature") == 0)
    {
        if (media->Temperature.CurrentTemp < -274)                  /* Below absolute zero: bad HAL data. */
            rc = XPON_BAD_VALUE;
        else
            rbusValue_SetInt32(value, media->Temperature.CurrentTemp);
    }
    else if (strcmp(param, "Voltage") == 0)
    {
        if (media->Voltage.VoltageLevel < 0)
            rc = XPON_BAD_VALUE;
        else
            rbusValue_SetUInt32(value, (uint32_t)media->Voltage.VoltageLevel);
    }
    else if (strcmp(param, "PONMode") == 0)
        rc = xpon_set_pon_mode(value, media);
    else if (strcmp(param, "Identifier") == 0)
    {
        if (media->Cage == SFP)
            rbusValue_SetUInt32(value, 3);                          /* SFF-8024 Table 4-1: 03h = SFP/SFP+/SFP28. */
        else
            rc = RBUS_ERROR_INVALID_OPERATION;                      /* BoB: no pluggable module identifier. */
    }
    else if (strcmp(param, "Connector") == 0)
        rc = xpon_set_enum(value, media->Connector, connectors, XPON_COUNT(connectors));
    else
        rc = RBUS_ERROR_INVALID_OPERATION;

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.ANI.{i}.* (Name, PONMode, SFPReferenceList, Status, TransceiverNumberOfEntries, Enable, Alias, LastChange).
 * @note Name ("ANI<i>") and SFPReferenceList ("") are constants; PONMode comes from HAL PhysicalMedia.PonMode.
 * @note Enable, Alias and LastChange need WAN_MANAGER_UNIFICATION_ENABLED.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE PhysicalMedia HAL read failed.
 * @retval RBUS_ERROR_INVALID_OPERATION           No unique active medium, LowerLayerDown status, or feature disabled.
 * @retval XPON_BAD_VALUE                         HAL value outside the BBF range.
 */
static rbusError_t xpon_ani_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *param = strrchr(rbusProperty_GetName(property), '.') + 1;
    GPON_DML_DATA *data;
    DML_PHY_MEDIA *media;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    uint32_t count = 0;
    size_t i;
    (void)handle;
    (void)options;

    /* CPE-assigned constants: answered without the HAL. */
    if (strcmp(param, "Name") == 0 || strcmp(param, "SFPReferenceList") == 0)
    {
        char text[16] = "";

        if (strcmp(param, "Name") == 0)
            snprintf(text, sizeof(text), "ANI%u", xpon_instance(rbusProperty_GetName(property), ".ANI."));
        rbusValue_Init(&value);
        rbusValue_SetString(value, text);                           /* SFPReferenceList: no Device.SFPs.SFPCage table. */
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        return RBUS_ERROR_SUCCESS;
    }

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_PM) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    media = xpon_active_phy(data);
    rbusValue_Init(&value);

    if (strcmp(param, "TransceiverNumberOfEntries") == 0)
    {
        for (i = 0; i < GPON_DATA_MAX_PM; ++i)
            if (data->gpon.PhysicalMedia.pdata[i])
                ++count;
        rbusValue_SetUInt32(value, count);
    }
    else if (strcmp(param, "PONMode") == 0)
        rc = xpon_set_pon_mode(value, media);                       /* "Unknown" without one active medium. */
    else if (!media)
        rc = RBUS_ERROR_INVALID_OPERATION;                          /* Remaining parameters need one active medium. */
    else if (strcmp(param, "Status") == 0)
        rc = media->Status == LowerLayerDown ? RBUS_ERROR_INVALID_OPERATION   /* No BBF ANI equivalent. */
                                             : xpon_set_enum(value, media->Status, g_ifStates, XPON_COUNT(g_ifStates));
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    else if (strcmp(param, "Alias") == 0)
        rc = xpon_set_string(value, media->Alias, sizeof(media->Alias), 64);
    else if (strcmp(param, "LastChange") == 0)
        rbusValue_SetUInt32(value, media->LastChange);
    else if (strcmp(param, "Enable") == 0)
        rbusValue_SetBoolean(value, media->Enable != 0);
#endif
    else
        rc = RBUS_ERROR_INVALID_OPERATION;

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    else if (rc == RBUS_ERROR_INVALID_OPERATION)
    {
        CcspTraceWarning(("XPON: %s not supported\n", rbusProperty_GetName(property)));
    }
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.ANI.{i}.TC.PerformanceThresholds.SignalFail / SignalDegrade.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE PhysicalMedia HAL read failed.
 * @retval RBUS_ERROR_INVALID_OPERATION           No unique active medium.
 * @retval XPON_BAD_VALUE                         SF not 3-8, or SD not 4-10 and greater than SF.
 */
static rbusError_t xpon_threshold_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *param = strrchr(rbusProperty_GetName(property), '.') + 1;
    GPON_DML_DATA *data;
    DML_PHY_MEDIA *media;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    (void)handle;
    (void)options;

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_PM) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    media = xpon_active_phy(data);
    rbusValue_Init(&value);

    if (!media)
        rc = RBUS_ERROR_INVALID_OPERATION;
    else if (strcmp(param, "SignalFail") == 0)
    {
        if (media->PerformThreshold.SignalFail < 3 || media->PerformThreshold.SignalFail > 8)
            rc = XPON_BAD_VALUE;
        else
            rbusValue_SetUInt32(value, (uint32_t)media->PerformThreshold.SignalFail);
    }
    else
    {
        /* SignalDegrade: 4-10 and greater than SignalFail. */
        if (media->PerformThreshold.SignalDegrade < 4 || media->PerformThreshold.SignalDegrade > 10 ||
            media->PerformThreshold.SignalDegrade <= media->PerformThreshold.SignalFail)
            rc = XPON_BAD_VALUE;
        else
            rbusValue_SetUInt32(value, (uint32_t)media->PerformThreshold.SignalDegrade);
    }

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.ANI.{i}.TC.Alarms.* boolean alarms.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; set to true when the alarm is ACTIVE.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE PhysicalMedia HAL read failed.
 * @retval RBUS_ERROR_INVALID_OPERATION           No unique active medium.
 * @retval XPON_BAD_VALUE                         Alarm is neither ACTIVE nor INACTIVE.
 */
static rbusError_t xpon_alarm_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *param = strrchr(rbusProperty_GetName(property), '.') + 1;
    GPON_DML_DATA *data;
    DML_PHY_MEDIA *media;
    ALARM_ENUM alarm = INACTIVE;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    (void)handle;
    (void)options;

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_PM) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    media = xpon_active_phy(data);
    if (!media)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_INVALID_OPERATION;
    }

    if (strcmp(param, "RDI") == 0) alarm = media->Alarm.RDI;
    else if (strcmp(param, "PEE") == 0) alarm = media->Alarm.PEE;
    else if (strcmp(param, "LOS") == 0) alarm = media->Alarm.LOS;
    else if (strcmp(param, "LOF") == 0) alarm = media->Alarm.LOF;
    else if (strcmp(param, "DACT") == 0) alarm = media->Alarm.DACT;
    else if (strcmp(param, "DIS") == 0) alarm = media->Alarm.DIS;
    else if (strcmp(param, "MIS") == 0) alarm = media->Alarm.MIS;
    else if (strcmp(param, "MEM") == 0) alarm = media->Alarm.MEM;
    else if (strcmp(param, "SUF") == 0) alarm = media->Alarm.SUF;
    else if (strcmp(param, "SF") == 0) alarm = media->Alarm.SF;
    else if (strcmp(param, "SD") == 0) alarm = media->Alarm.SD;
    else if (strcmp(param, "LCDG") == 0) alarm = media->Alarm.LCDG;
    else if (strcmp(param, "TF") == 0) alarm = media->Alarm.TF;
    else if (strcmp(param, "ROGUE") == 0) alarm = media->Alarm.ROGUE;
    else rc = RBUS_ERROR_INVALID_OPERATION;
    GponMgrDml_GetData_release(data);

    if (rc == RBUS_ERROR_SUCCESS && alarm != ACTIVE && alarm != INACTIVE)
        rc = XPON_BAD_VALUE;
    if (rc == RBUS_ERROR_SUCCESS)
    {
        rbusValue_Init(&value);
        rbusValue_SetBoolean(value, alarm == ACTIVE);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
    }
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.ANI.{i}.TC.PM.PHY.* FEC/HEC counters and GEM.FrameHeaderHECErrors.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE GTC HAL read failed.
 */
static rbusError_t xpon_gtc_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *param = strrchr(rbusProperty_GetName(property), '.') + 1;
    GPON_DML_DATA *data;
    DML_GTC *gtc;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    (void)handle;
    (void)options;

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_GTC) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    gtc = &data->gpon.Gtc;
    rbusValue_Init(&value);

    if (strcmp(param, "CorrectedFECBytes") == 0)
        rbusValue_SetUInt64(value, gtc->CorrectedFecBytes);
    else if (strcmp(param, "CorrectedFECCodewords") == 0)
        rbusValue_SetUInt64(value, gtc->CorrectedFecCodeWords);
    else if (strcmp(param, "UncorrectableFECCodewords") == 0)
        rbusValue_SetUInt64(value, gtc->UnCorrectedFecCodeWords);
    else if (strcmp(param, "TotalFECCodewords") == 0)
        rbusValue_SetUInt64(value, gtc->TotalFecCodeWords);
    else if (strcmp(param, "PSBdHECErrorCount") == 0)
        rbusValue_SetUInt32(value, (uint32_t)gtc->PSBdHecErrors);
    else if (strcmp(param, "HeaderHECErrorCount") == 0)
        rbusValue_SetUInt32(value, (uint32_t)gtc->HecErrorCount);
    else if (strcmp(param, "FrameHeaderHECErrors") == 0)
        rbusValue_SetUInt32(value, (uint32_t)gtc->FrameHecErrors);
    else
        rc = RBUS_ERROR_INVALID_OPERATION;

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.ANI.{i}.TC.ONUActivation.* (ONUID, VendorID, SerialNumber, ONUState).
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE PLOAM HAL read failed.
 * @retval RBUS_ERROR_INVALID_OPERATION           Serial number in an unknown format.
 * @retval XPON_BAD_VALUE                         ONUID > 1022, VendorID > 4 chars, bad serial or state.
 */
static rbusError_t xpon_activation_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    static const char *const states[] = {"O1", "O2", "O3", "O4", "O5", "O6", "O7", "O8", "O9"};
    const char *param = strrchr(rbusProperty_GetName(property), '.') + 1;
    GPON_DML_DATA *data;
    DML_PLOAM *ploam;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    (void)handle;
    (void)options;

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_PLOAM) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    ploam = &data->gpon.Ploam;
    rbusValue_Init(&value);

    if (strcmp(param, "ONUID") == 0)
    {
        if (ploam->OnuId > 1022)
            rc = XPON_BAD_VALUE;
        else
            rbusValue_SetUInt32(value, ploam->OnuId);
    }
    else if (strcmp(param, "VendorID") == 0)
        rc = xpon_set_string(value, ploam->VendorId, sizeof(ploam->VendorId), 4);
    else if (strcmp(param, "SerialNumber") == 0)
        rc = xpon_set_serial(value, ploam->SerialNumber, sizeof(ploam->SerialNumber));
    else if (strcmp(param, "ONUState") == 0)
        rc = xpon_set_enum(value, ploam->RegistrationState, states, XPON_COUNT(states));
    else
        rc = RBUS_ERROR_INVALID_OPERATION;

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.ANI.{i}.TC.PM.PLOAM.* message and MIC error counters.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE PLOAM HAL read failed.
 * @retval XPON_BAD_VALUE                         MICErrors does not fit in uint32.
 */
static rbusError_t xpon_ploam_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *param = strrchr(rbusProperty_GetName(property), '.') + 1;
    GPON_DML_DATA *data;
    DML_PLOAM *ploam;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    (void)handle;
    (void)options;

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_PLOAM) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    ploam = &data->gpon.Ploam;
    rbusValue_Init(&value);

    if (strcmp(param, "UpstreamMessageCount") == 0)
        rbusValue_SetUInt64(value, ploam->TxMessageCount);
    else if (strcmp(param, "DownstreamMessageCount") == 0)
        rbusValue_SetUInt64(value, ploam->RxMessageCount);
    else if (strcmp(param, "MICErrors") == 0)
    {
        if ((uint64_t)ploam->MicErrors > UINT32_MAX)
            rc = XPON_BAD_VALUE;
        else
            rbusValue_SetUInt32(value, (uint32_t)ploam->MicErrors);
    }
    else
        rc = RBUS_ERROR_INVALID_OPERATION;

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.ANI.{i}.TC.PM.OMCI.* message and MIC error counters.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE OMCI HAL read failed.
 * @retval XPON_BAD_VALUE                         MICErrors does not fit in uint32.
 */
static rbusError_t xpon_omci_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *param = strrchr(rbusProperty_GetName(property), '.') + 1;
    GPON_DML_DATA *data;
    DML_OMCI *omci;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    (void)handle;
    (void)options;

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_OMCI) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    omci = &data->gpon.Omci;
    rbusValue_Init(&value);

    if (strcmp(param, "BaselineMessagesReceived") == 0)
        rbusValue_SetUInt64(value, omci->RxBaseLineMessageCountValid);
    else if (strcmp(param, "ExtendedMessagesReceived") == 0)
        rbusValue_SetUInt64(value, omci->RxExtendedMessageCountValid);
    else if (strcmp(param, "MICErrors") == 0)
    {
        if ((uint64_t)omci->MicErrors > UINT32_MAX)
            rc = XPON_BAD_VALUE;
        else
            rbusValue_SetUInt32(value, (uint32_t)omci->MicErrors);
    }
    else
        rc = RBUS_ERROR_INVALID_OPERATION;

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Get handler for ANI.{i}.TC.GEM.PortNumberOfEntries and ANI.{i}.TC.PM.GEM.FramesSent/FramesReceived.
 *
 * PortNumberOfEntries counts populated HAL Gem rows; FramesSent/FramesReceived sum
 * HAL Gem.{i}.TransmittedFrames / ReceivedFrames across all rows.
 *
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE GEM HAL read failed.
 */
static rbusError_t xpon_gem_totals_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *param = strrchr(rbusProperty_GetName(property), '.') + 1;
    GPON_DML_DATA *data;
    rbusValue_t value;
    uint64_t sent = 0, received = 0;
    uint32_t count = 0;
    size_t i;
    (void)handle;
    (void)options;

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_GEM) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    for (i = 0; i < GPON_DATA_MAX_GEM; ++i)
        if (data->gpon.Gem.pdata[i])
        {
            ++count;
            sent += data->gpon.Gem.pdata[i]->dml.TransmittedFrames;
            received += data->gpon.Gem.pdata[i]->dml.ReceivedFrames;
        }
    GponMgrDml_GetData_release(data);

    rbusValue_Init(&value);
    if (strcmp(param, "FramesSent") == 0)
        rbusValue_SetUInt64(value, sent);
    else if (strcmp(param, "FramesReceived") == 0)
        rbusValue_SetUInt64(value, received);
    else
        rbusValue_SetUInt32(value, count);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Get handler for ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.* parameters.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE GEM HAL read failed.
 * @retval RBUS_ERROR_ELEMENT_DOES_NOT_EXIST      No GEM row for this instance.
 * @retval RBUS_ERROR_INVALID_OPERATION           Direction is not bidirectional.
 * @retval XPON_BAD_VALUE                         PortID > 65534.
 * @note PortType is derived from HAL TrafficType (Bidirectional -> unicast, Unidirectional -> multicast).
 */
static rbusError_t xpon_gem_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *name = rbusProperty_GetName(property);
    const char *param = strrchr(name, '.') + 1;
    const char *row = strstr(name, ".GEM.Port.");
    GPON_DML_DATA *data;
    DML_GEM *gem = NULL;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    uint32_t instance = 0;
    size_t i;
    (void)handle;
    (void)options;

    if (!row || sscanf(row, ".GEM.Port.%u", &instance) != 1)
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_GEM) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    for (i = 0; i < GPON_DATA_MAX_GEM && !gem; ++i)
        if (data->gpon.Gem.pdata[i] && data->gpon.Gem.pdata[i]->dml.uInstanceNumber == instance)
            gem = &data->gpon.Gem.pdata[i]->dml;
    if (!gem)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    rbusValue_Init(&value);

    if (strcmp(param, "PortID") == 0)
    {
        if (gem->PortId > 65534)
            rc = XPON_BAD_VALUE;
        else
            rbusValue_SetUInt32(value, (uint32_t)gem->PortId);
    }
    else if (strcmp(param, "FramesSent") == 0)
        rbusValue_SetUInt64(value, gem->TransmittedFrames);
    else if (strcmp(param, "FramesReceived") == 0)
        rbusValue_SetUInt64(value, gem->ReceivedFrames);
    else if (strcmp(param, "Direction") == 0)
    {
        if (gem->TrafficType != Bidirectional)                      /* HAL does not give the unidirectional orientation. */
            rc = RBUS_ERROR_INVALID_OPERATION;
        else
            rbusValue_SetString(value, "bidirectional");
    }
    else if (strcmp(param, "PortType") == 0)
        /* HAL TrafficType heuristic: bidirectional GEM ports carry unicast, unidirectional (downstream) ones multicast. */
        rbusValue_SetString(value, gem->TrafficType == Bidirectional ? "unicast" : "multicast");
    else
        rc = RBUS_ERROR_INVALID_OPERATION;

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Build the read-only EthernetUNI alias "cpe-" + upper-case VEIP InterfaceName.
 * @param veip VEIP row.
 * @param buf  Output buffer (at least 65 bytes for the BBF 64-character limit).
 * @param size Size of @p buf.
 * @return RBUS_ERROR_SUCCESS, or XPON_BAD_VALUE if the result exceeds 64 characters.
 * @note Falls back to "cpe-VEIP<i>" when InterfaceName is empty.
 */
static rbusError_t xpon_veip_alias(const DML_VEIP *veip, char *buf, size_t size)
{
    size_t i, length = strnlen(veip->InterfaceName, sizeof(veip->InterfaceName));
    int written;

    if (length == 0)
        written = snprintf(buf, size, "cpe-VEIP%lu", (unsigned long)veip->uInstanceNumber);
    else if (length == sizeof(veip->InterfaceName))
        return XPON_BAD_VALUE;
    else
        written = snprintf(buf, size, "cpe-%s", veip->InterfaceName);
    if (written < 0 || (size_t)written >= size || written > 64)
        return XPON_BAD_VALUE;
    for (i = 4; buf[i]; ++i)
        buf[i] = (char)toupper((unsigned char)buf[i]);
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Get handler for ONU.{i}.EthernetUNI.{i}.* parameters backed by VEIP rows.
 * @note Alias, Description, LowerLayers and Upstream are computed here; LastChange uses
 *       DML_VEIP.LastChangeTime, stamped by the middle layer on OperationalState changes.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE VEIP HAL read failed.
 * @retval RBUS_ERROR_ELEMENT_DOES_NOT_EXIST      No VEIP row for this instance.
 * @retval XPON_BAD_VALUE                         Name > 64 chars, InterdomainName > 25, or bad state.
 */
static rbusError_t xpon_veip_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const char *name = rbusProperty_GetName(property);
    const char *param = strrchr(name, '.') + 1;
    const char *row = strstr(name, ".EthernetUNI.");
    GPON_DML_DATA *data;
    DML_VEIP *veip = NULL;
    rbusValue_t value;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    uint32_t instance = 0;
    size_t i;
    (void)handle;
    (void)options;

    if (!row || sscanf(row, ".EthernetUNI.%u", &instance) != 1)
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_VEIP) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    for (i = 0; i < GPON_DATA_MAX_VEIP && !veip; ++i)
        if (data->gpon.Veip.pdata[i] && data->gpon.Veip.pdata[i]->dml.uInstanceNumber == instance)
            veip = &data->gpon.Veip.pdata[i]->dml;
    if (!veip)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    rbusValue_Init(&value);

    if (strcmp(param, "Name") == 0)
        rc = xpon_set_string(value, veip->InterfaceName, sizeof(veip->InterfaceName), 64);
    else if (strcmp(param, "InterdomainName") == 0)
        rc = xpon_set_string(value, veip->InterDomainName, sizeof(veip->InterDomainName), 25);
    else if (strcmp(param, "Enable") == 0)
    {
        if (veip->AdministrativeState != Lock && veip->AdministrativeState != Unlock)
            rc = XPON_BAD_VALUE;
        else
            rbusValue_SetBoolean(value, veip->AdministrativeState == Unlock);   /* Unlock = enabled. */
    }
    else if (strcmp(param, "Status") == 0)
        rc = xpon_set_enum(value, veip->OperationalState, g_ifStates, XPON_COUNT(g_ifStates));
    else if (strcmp(param, "InterdomainID") == 0)
    {
        char identity[48];
        snprintf(identity, sizeof(identity), "(VEIP,%lu)", (unsigned long)veip->MeId);   /* OMCI ME identity. */
        rbusValue_SetString(value, identity);
    }
    else if (strcmp(param, "ANIs") == 0)
        rbusValue_SetString(value, "Device.XPON.ONU.1.ANI.1");
    else if (strcmp(param, "Alias") == 0 || strcmp(param, "Description") == 0)
    {
        char alias[65], text[96];

        rc = xpon_veip_alias(veip, alias, sizeof(alias));
        if (rc == RBUS_ERROR_SUCCESS)
        {
            if (strcmp(param, "Alias") == 0)
                rbusValue_SetString(value, alias);
            else
            {
                snprintf(text, sizeof(text), "GPON VEIP WAN interface %s", alias);
                rbusValue_SetString(value, text);
            }
        }
    }
    else if (strcmp(param, "LastChange") == 0)
    {
        ULONG now = GponMgrDml_GetMonotonicSeconds();
        rbusValue_SetUInt32(value, now >= veip->LastChangeTime ? (uint32_t)(now - veip->LastChangeTime) : 0);
    }
    else if (strcmp(param, "LowerLayers") == 0)
        rbusValue_SetString(value, "");                             /* BBF: LowerLayers not expected to be used for EthernetUNI. */
    else if (strcmp(param, "Upstream") == 0)
        rbusValue_SetBoolean(value, true);                          /* The VEIP is always the gateway WAN side. */
    else
        rc = RBUS_ERROR_INVALID_OPERATION;

    if (rc == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    GponMgrDml_GetData_release(data);
    return rc;
}

/**
 * @brief Get handler for ONU.{i}.EthernetUNI.{i}.Stats.* backed by Linux sysfs.
 *
 * Reads /sys/class/net/<VEIP InterfaceName>/statistics/<counter>. The interface
 * name is copied under the DML lock; sysfs is read after the lock is released.
 * 32-bit BBF counters (Errors*, Discard*) are truncated and wrap.
 *
 * @param handle   RBUS handle (unused).
 * @param property Property being read; its value is set on success.
 * @param options  Get options (unused).
 * @retval RBUS_ERROR_SUCCESS                     Value set.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE VEIP HAL read failed.
 * @retval RBUS_ERROR_ELEMENT_DOES_NOT_EXIST      No VEIP row for this instance.
 * @retval RBUS_ERROR_INVALID_OPERATION           Unknown counter, invalid interface name, or sysfs not readable.
 */
static rbusError_t xpon_uni_stats_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    static const struct
    {
        const char *param;
        const char *file;
        bool wide;              /* true: unsignedLong (64-bit) BBF counter. */
    } counters[] = {
        {"BytesSent", "tx_bytes", true},          {"BytesReceived", "rx_bytes", true},
        {"PacketsSent", "tx_packets", true},      {"PacketsReceived", "rx_packets", true},
        {"ErrorsSent", "tx_errors", false},       {"ErrorsReceived", "rx_errors", false},
        {"DiscardPacketsSent", "tx_dropped", false}, {"DiscardPacketsReceived", "rx_dropped", false},
        {"MulticastPacketsReceived", "multicast", true},
    };
    const char *name = rbusProperty_GetName(property);
    const char *param = strrchr(name, '.') + 1;
    uint32_t instance = xpon_instance(name, ".EthernetUNI.");
    GPON_DML_DATA *data;
    char ifName[IFNAMSIZ] = "", path[128];
    unsigned long long counter = 0;
    rbusValue_t value;
    size_t i, entry;
    bool found = false;
    FILE *file;
    (void)handle;
    (void)options;

    for (entry = 0; entry < XPON_COUNT(counters); ++entry)
        if (strcmp(param, counters[entry].param) == 0)
            break;
    if (entry == XPON_COUNT(counters))
        return RBUS_ERROR_INVALID_OPERATION;

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, XPON_HAL_VEIP) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    for (i = 0; i < GPON_DATA_MAX_VEIP && !found; ++i)
        if (data->gpon.Veip.pdata[i] && data->gpon.Veip.pdata[i]->dml.uInstanceNumber == instance)
        {
            const char *source = data->gpon.Veip.pdata[i]->dml.InterfaceName;
            size_t length = strnlen(source, sizeof(data->gpon.Veip.pdata[i]->dml.InterfaceName));
            found = true;
            if (length < sizeof(ifName))
            {
                memcpy(ifName, source, length);
                ifName[length] = '\0';
            }
        }
    GponMgrDml_GetData_release(data);
    if (!found)
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;

    /* The name comes from the HAL: reject anything that could escape /sys/class/net. */
    if (ifName[0] == '\0' || strchr(ifName, '/') || strcmp(ifName, ".") == 0 || strcmp(ifName, "..") == 0)
    {
        CcspTraceWarning(("XPON: %s: invalid VEIP interface name\n", name));
        return RBUS_ERROR_INVALID_OPERATION;
    }
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/%s", ifName, counters[entry].file);
    file = fopen(path, "r");
    if (!file)
    {
        CcspTraceWarning(("XPON: %s: cannot open %s\n", name, path));
        return RBUS_ERROR_INVALID_OPERATION;
    }
    found = fscanf(file, "%llu", &counter) == 1;
    fclose(file);
    if (!found)
        return RBUS_ERROR_INVALID_OPERATION;

    rbusValue_Init(&value);
    if (counters[entry].wide)
        rbusValue_SetUInt64(value, (uint64_t)counter);
    else
        rbusValue_SetUInt32(value, (uint32_t)counter);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Get handler for registered BBF parameters with no HAL source.
 * @param handle   RBUS handle (unused).
 * @param property Property being read; never set.
 * @param options  Get options (unused).
 * @return Always RBUS_ERROR_INVALID_OPERATION.
 */
static rbusError_t xpon_unsupported_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    (void)handle;
    (void)options;
    CcspTraceWarning(("XPON: %s not supported yet (no HAL field)\n", rbusProperty_GetName(property)));
    return RBUS_ERROR_INVALID_OPERATION;
}

/* ---------------------------------------------------------------------- *
 * Set handlers                                                            *
 * ---------------------------------------------------------------------- */

/**
 * @brief Set handler for supported parameters that GPON Manager exposes read-only
 *        (ONU.Enable, EthernetUNI.Alias/Description/LowerLayers, ANI.Enable/Alias).
 * @param handle   RBUS handle (unused).
 * @param property Property with the new value (ignored).
 * @param options  Set options (unused).
 * @return Always RBUS_ERROR_NOT_WRITABLE.
 */
static rbusError_t xpon_readonly_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t *options)
{
    (void)handle;
    (void)options;
    CcspTraceWarning(("XPON: %s is read-only\n", rbusProperty_GetName(property)));
    return RBUS_ERROR_NOT_WRITABLE;
}

/**
 * @brief Set handler for writable BBF parameters with no HAL target.
 * @param handle   RBUS handle (unused).
 * @param property Property with the new value (ignored).
 * @param options  Set options (unused).
 * @return Always RBUS_ERROR_INVALID_OPERATION.
 */
static rbusError_t xpon_unsupported_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t *options)
{
    (void)handle;
    (void)options;
    CcspTraceWarning(("XPON: %s not writable yet (no HAL field)\n", rbusProperty_GetName(property)));
    return RBUS_ERROR_INVALID_OPERATION;
}

/* ---------------------------------------------------------------------- *
 * Event subscription                                                      *
 * ---------------------------------------------------------------------- */

/**
 * @brief Subscription handler for supported parameters.
 * @param handle           RBUS handle (unused).
 * @param action           Subscribe or unsubscribe (both accepted).
 * @param name             Parameter name (unused).
 * @param filter           Event filter (unused).
 * @param interval         Interval (unused).
 * @param[out] autoPublish Set to true so RBUS polls the getter and publishes changes.
 * @return RBUS_ERROR_SUCCESS, or RBUS_ERROR_INVALID_INPUT if @p autoPublish is NULL.
 */
static rbusError_t xpon_subscribe(rbusHandle_t handle, rbusEventSubAction_t action,
                                 const char *name, rbusFilter_t filter, int32_t interval, bool *autoPublish)
{
    (void)handle; (void)action; (void)name; (void)filter; (void)interval;
    if (!autoPublish)
        return RBUS_ERROR_INVALID_INPUT;
    *autoPublish = true;
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Subscription handler for unsupported parameters.
 * @param handle           RBUS handle (unused).
 * @param action           Subscribe or unsubscribe.
 * @param name             Parameter name, used for logging.
 * @param filter           Event filter (unused).
 * @param interval         Interval (unused).
 * @param[out] autoPublish Set to false.
 * @return RBUS_ERROR_SUCCESS for unsubscribe; RBUS_ERROR_INVALID_OPERATION for subscribe.
 */
static rbusError_t xpon_unsupported_subscribe(rbusHandle_t handle, rbusEventSubAction_t action,
                                             const char *name, rbusFilter_t filter, int32_t interval, bool *autoPublish)
{
    (void)handle; (void)filter; (void)interval;
    if (autoPublish)
        *autoPublish = false;
    if (action == RBUS_EVENT_ACTION_UNSUBSCRIBE)
        return RBUS_ERROR_SUCCESS;
    CcspTraceWarning(("XPON: events for %s not supported yet\n", name));
    return RBUS_ERROR_INVALID_OPERATION;
}

/* ---------------------------------------------------------------------- *
 * Table row sync                                                          *
 * ---------------------------------------------------------------------- */

/**
 * @brief Make the registered rows of one table match the HAL rows.
 * @param table      Table name ending in ".", e.g. XPON_ANI "Transceiver.".
 * @param registered Registered instance per slot (0 = none); updated in place.
 * @param current    Instance per slot from the HAL (0 = empty slot).
 * @param count      Number of slots.
 * @note Caller holds g_mutex. Unregisters first so a moved instance can be re-registered.
 */
static void xpon_sync_rows(const char *table, uint32_t *registered, const uint32_t *current, size_t count)
{
    char row[RBUS_MAX_NAME_LENGTH];
    size_t i;

    for (i = 0; i < count; ++i)
        if (registered[i] && registered[i] != current[i])
        {
            snprintf(row, sizeof(row), "%s%u", table, registered[i]);
            rbusTable_unregisterRow(g_handle, row);
            registered[i] = 0;
        }
    for (i = 0; i < count; ++i)
        if (current[i] && !registered[i])
        {
            if (rbusTable_registerRow(g_handle, table, current[i], NULL) == RBUS_ERROR_SUCCESS)
                registered[i] = current[i];
            else
            {
                CcspTraceWarning(("XPON: failed to register row %s%u\n", table, current[i]));
            }
        }
}

/**
 * @brief Read one dynamic table (PhysicalMedia, GEM or VEIP) from the HAL and resync its RBUS rows.
 * @param table XPON_HAL_PM, XPON_HAL_GEM or XPON_HAL_VEIP.
 * @retval RBUS_ERROR_SUCCESS                     Rows synced.
 * @retval RBUS_ERROR_INVALID_INPUT               @p table is not a dynamic table.
 * @retval RBUS_ERROR_NOT_INITIALIZED             DML data not available.
 * @retval RBUS_ERROR_DESTINATION_RESPONSE_FAILURE HAL read failed; rows are left unchanged.
 * @note Caller holds g_mutex and g_handle is valid.
 */
static rbusError_t xpon_sync_table(XponHalTable table)
{
    uint32_t current[GPON_DATA_MAX_GEM] = {0};    /* Sized for the largest table (GEM). */
    GPON_DML_DATA *data;
    const char *name;
    uint32_t *registered;
    size_t count, i;

    switch (table)
    {
        case XPON_HAL_PM:   name = XPON_ANI "Transceiver.";            registered = g_phyRows;  count = GPON_DATA_MAX_PM;   break;
        case XPON_HAL_GEM:  name = XPON_ANI "TC.GEM.Port.";            registered = g_gemRows;  count = GPON_DATA_MAX_GEM;  break;
        case XPON_HAL_VEIP: name = "Device.XPON.ONU.1.EthernetUNI.";   registered = g_veipRows; count = GPON_DATA_MAX_VEIP; break;
        default:            return RBUS_ERROR_INVALID_INPUT;
    }

    data = GponMgrDml_GetData_locked();
    if (!data)
        return RBUS_ERROR_NOT_INITIALIZED;
    if (xpon_hal_get(data, table) != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    /* Snapshot instance numbers (HAL index = slot + 1) so RBUS calls run without the DML lock. */
    for (i = 0; i < count; ++i)
    {
        if (table == XPON_HAL_PM && data->gpon.PhysicalMedia.pdata[i])
            current[i] = data->gpon.PhysicalMedia.pdata[i]->dml.uInstanceNumber;
        else if (table == XPON_HAL_GEM && data->gpon.Gem.pdata[i])
            current[i] = data->gpon.Gem.pdata[i]->dml.uInstanceNumber;
        else if (table == XPON_HAL_VEIP && data->gpon.Veip.pdata[i])
            current[i] = data->gpon.Veip.pdata[i]->dml.uInstanceNumber;
    }
    GponMgrDml_GetData_release(data);

    xpon_sync_rows(name, registered, current, count);
    return RBUS_ERROR_SUCCESS;
}

/**
 * @brief Resync all dynamic tables (Transceiver, GEM Port, EthernetUNI).
 * @retval RBUS_ERROR_SUCCESS All tables synced.
 * @retval other              First xpon_sync_table() error; later tables are still synced.
 * @note Caller holds g_mutex and g_handle is valid.
 */
static rbusError_t xpon_sync_locked(void)
{
    rbusError_t rc = xpon_sync_table(XPON_HAL_PM);
    rbusError_t r2 = xpon_sync_table(XPON_HAL_GEM);
    rbusError_t r3 = xpon_sync_table(XPON_HAL_VEIP);

    if (rc == RBUS_ERROR_SUCCESS)
        rc = (r2 != RBUS_ERROR_SUCCESS) ? r2 : r3;
    return rc;
}

/**
 * @brief RBUS dynamic table sync callback; resyncs only the table being resolved.
 *
 * RBUS calls this for every table on a get/discover path, before resolving the
 * row instance, so a VEIP/GEM/PM row that appeared after Init is registered in time.
 *
 * @param handle    RBUS handle (unused).
 * @param tableName Table being resolved (template or instance form).
 * @retval RBUS_ERROR_SUCCESS         Table synced, or a fixed table (ONU, ANI) that needs no sync.
 * @retval RBUS_ERROR_NOT_INITIALIZED Provider not initialized.
 * @retval other                      xpon_sync_table() result.
 */
static rbusError_t xpon_sync(rbusHandle_t handle, const char *tableName)
{
    XponHalTable table;
    rbusError_t rc;
    (void)handle;

    if (!tableName)
        return RBUS_ERROR_INVALID_INPUT;
    if (strstr(tableName, "EthernetUNI"))
        table = XPON_HAL_VEIP;
    else if (strstr(tableName, "Transceiver"))
        table = XPON_HAL_PM;
    else if (strstr(tableName, "GEM.Port"))
        table = XPON_HAL_GEM;
    else
        return RBUS_ERROR_SUCCESS;              /* ONU.1 / ANI.1 are fixed rows. */

    pthread_mutex_lock(&g_mutex);
    rc = g_handle ? xpon_sync_table(table) : RBUS_ERROR_NOT_INITIALIZED;
    pthread_mutex_unlock(&g_mutex);
    return rc;
}

/* ---------------------------------------------------------------------- *
 * Public API                                                              *
 * ---------------------------------------------------------------------- */

/**
 * @brief Register the Device.XPON tree, the fixed ONU.1/ANI.1 rows and the table sync handlers.
 *
 * Returns success at once if already initialized. On failure the state is reset.
 *
 * @param handle Open RBUS handle owned by the caller.
 * @retval RBUS_ERROR_SUCCESS        Provider ready.
 * @retval RBUS_ERROR_INVALID_HANDLE @p handle is NULL.
 * @retval other                     Registration or initial row sync error.
 */
int GponMgrXponTr181_Init(rbusHandle_t handle)
{
    rbusError_t rc;
    size_t i;

    if (!handle)
        return RBUS_ERROR_INVALID_HANDLE;
    pthread_mutex_lock(&g_mutex);
    if (g_handle)
    {
        pthread_mutex_unlock(&g_mutex);
        return RBUS_ERROR_SUCCESS;
    }
    g_handle = handle;
    GponMgrXponTr181_InvalidateCache();

    rc = rbus_regDataElements(g_handle, (int)g_elementCount, g_elements);
    /* RBUS calls xpon_sync() whenever a table is enumerated. */
    for (i = 0; i < g_elementCount && rc == RBUS_ERROR_SUCCESS; ++i)
        if (g_elements[i].type == RBUS_ELEMENT_TYPE_TABLE)
            rc = rbus_registerDynamicTableSyncHandler(g_handle, g_elements[i].name, xpon_sync);
    /* Single ONU with a single ANI: fixed rows. */
    if (rc == RBUS_ERROR_SUCCESS)
        rc = rbusTable_registerRow(g_handle, "Device.XPON.ONU.", 1, NULL);
    if (rc == RBUS_ERROR_SUCCESS)
        rc = rbusTable_registerRow(g_handle, "Device.XPON.ONU.1.ANI.", 1, NULL);
    if (rc == RBUS_ERROR_SUCCESS)
        rc = xpon_sync_locked();

    if (rc == RBUS_ERROR_SUCCESS)
    {
        CcspTraceInfo(("XPON: registered %zu BBF Device.XPON elements\n", g_elementCount));
    }
    else
    {
        CcspTraceError(("XPON: Device.XPON registration failed (%d)\n", rc));
        g_handle = NULL;
        memset(g_phyRows, 0, sizeof(g_phyRows));
        memset(g_gemRows, 0, sizeof(g_gemRows));
        memset(g_veipRows, 0, sizeof(g_veipRows));
    }
    pthread_mutex_unlock(&g_mutex);
    return rc;
}

/**
 * @brief Forget the RBUS handle and the registered rows.
 * @note Does not unregister elements; the caller's rbus_close() removes them.
 */
void GponMgrXponTr181_Close(void)
{
    GponMgrXponTr181_InvalidateCache();
    pthread_mutex_lock(&g_mutex);
    g_handle = NULL;
    memset(g_phyRows, 0, sizeof(g_phyRows));
    memset(g_gemRows, 0, sizeof(g_gemRows));
    memset(g_veipRows, 0, sizeof(g_veipRows));
    pthread_mutex_unlock(&g_mutex);
}
