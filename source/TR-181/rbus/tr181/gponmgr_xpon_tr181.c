#include "gponmgr_xpon_tr181.h"
#include "gponmgr_dml_data.h"
#include "gponmgr_dml_hal.h"
#include <rbus.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum
{
    XPON_NONE, XPON_PHY,
    XPON_GTC, XPON_PLOAM, XPON_OMCI, XPON_GEM, XPON_VEIP,
    XPON_SOURCE_COUNT
} XponSource;

typedef struct
{
    const char *name;
    const char *parameter;
    uint32_t instances[3];
    GPON_DML_DATA *data;
    rbusProperty_t property;
    rbusValue_t value;
} XponRequest;

#define XPON_MAX_ROWS (2 + GPON_DATA_MAX_PM + GPON_DATA_MAX_GEM + GPON_DATA_MAX_VEIP)
#define XPON_ANI "Device.XPON.ONU.1.ANI.1."

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static rbusHandle_t g_handle;
static bool g_ready;
static time_t g_fetched[XPON_SOURCE_COUNT];
static char g_rows[XPON_MAX_ROWS][RBUS_MAX_NAME_LENGTH];
static size_t g_rowCount;

static rbusError_t xpon_base_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_unsupported_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_veip_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_ani_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_activation_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_gtc_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_ploam_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_omci_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_gem_count_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_gem_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_threshold_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_alarm_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_transceiver_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options);
static rbusError_t xpon_ani_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t *options);
static rbusError_t xpon_unsupported_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t *options);
static rbusError_t xpon_add(rbusHandle_t handle, const char *name, const char *alias, uint32_t *instance);
static rbusError_t xpon_remove(rbusHandle_t handle, const char *name);
static rbusError_t xpon_subscribe(rbusHandle_t handle, rbusEventSubAction_t action, const char *name, rbusFilter_t filter, int32_t interval, bool *autoPublish);

static rbusDataElement_t g_elements[] = {
    {"Device.XPON.ONU.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, xpon_add, xpon_remove, NULL, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, xpon_add, xpon_remove, NULL, NULL}},
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, xpon_add, xpon_remove, NULL, NULL}}, /* Not supported yet: HAL software-image inventory is unavailable; no rows are registered. */
    {"Device.XPON.ONU.{i}.ANI.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, xpon_add, xpon_remove, NULL, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, xpon_add, xpon_remove, NULL, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, xpon_add, xpon_remove, NULL, NULL}},

    {"Device.XPON.ONUNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_base_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerCapability", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerStatus", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.Version", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EquipmentID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImageNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_base_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNINumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_base_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANINumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_base_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Alias", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Description", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.LastChange", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.LowerLayers", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Upstream", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.ANIs", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.InterdomainName", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_veip_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.Reset", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BytesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.PacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.ErrorsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.DiscardPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnicastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.MulticastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.BroadcastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.EthernetUNI.{i}.Stats.UnknownProtoPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.ID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.Version", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.IsCommitted", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.IsActive", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.SoftwareImage.{i}.IsValid", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, xpon_ani_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported without WAN_MANAGER_UNIFICATION_ENABLED; access logs a warning. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Alias", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, xpon_ani_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported without WAN_MANAGER_UNIFICATION_ENABLED; access logs a warning. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.LastChange", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported without WAN_MANAGER_UNIFICATION_ENABLED; access logs a warning. */
    {"Device.XPON.ONU.{i}.ANI.{i}.PONMode", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TransceiverNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ani_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.SFPReferenceList", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.BytesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.PacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.ErrorsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.UnicastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.DiscardPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.MulticastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.BroadcastPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.Stats.UnknownProtoPacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUState", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_activation_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.VendorID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_activation_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.SerialNumber", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_activation_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.ONUActivation.ONUID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_activation_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.Password", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Authentication.HexadecimalPassword", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, xpon_unsupported_set_handler, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECBytes", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.CorrectedFECCodewords", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UncorrectableFECCodewords", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.TotalFECCodewords", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.PSBdHECErrorCount", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.HeaderHECErrorCount", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PHY.UnknownProfile", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesSent", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FramesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.FrameHeaderHECErrors", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gtc_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.GEM.KeyErrors", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.MICErrors", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ploam_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.DownstreamMessageCount", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ploam_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.RangingTime", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.PLOAM.UpstreamMessageCount", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_ploam_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.BaselineMessagesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_omci_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.ExtendedMessagesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_omci_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.PM.OMCI.MICErrors", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_omci_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.PortNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_count_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.Direction", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_gem_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Unidirectional mapping is not supported yet: HAL omits the traffic direction. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.GEM.Port.{i}.PortType", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */

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
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.LODS", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.ANI.{i}.TC.Alarms.ROGUE", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_alarm_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},

    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.ID", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_transceiver_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}},
    {"Device.XPON.ONU.{i}.ANI.{i}.Transceiver.{i}.Identifier", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
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

    {"Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementCapability", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.OLTSleepAllow", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.PowerReductionManagementStatus", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.ITransInit", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.ITxInit", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.MaxSleepInterval", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.MinAwareInterval", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.MinActiveHeldInterval", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
    {"Device.XPON.ONU.{i}.PowerManagement.MaxSleepIntervalExtension", RBUS_ELEMENT_TYPE_PROPERTY, {xpon_unsupported_get_handler, NULL, NULL, NULL, xpon_subscribe, NULL}}, /* Not supported yet: no equivalent unified HAL field. */
};

static const size_t g_elementCount = sizeof(g_elements) / sizeof(g_elements[0]);

static rbusError_t xpon_unsupported(const char *name, const char *reason)
{
    CcspTraceWarning(("XPON: BBF TR-181 %s not supported yet (%s)\n", name, reason));
    return RBUS_ERROR_INVALID_OPERATION;
}

static bool xpon_match(const char *pattern, const char *name, uint32_t instances[3])
{
    size_t count = 0;
    memset(instances, 0, 3 * sizeof(*instances));
    while (*pattern)
    {
        if (strncmp(pattern, "{i}", 3) == 0)
        {
            uint64_t number = 0;
            if (count == 3 || *name < '1' || *name > '9')
                return false;
            do
            {
                number = number * 10 + (unsigned int)(*name++ - '0');
                if (number > UINT32_MAX)
                    return false;
            } while (*name >= '0' && *name <= '9');
            instances[count++] = (uint32_t)number;
            pattern += 3;
        }
        else if (*pattern++ != *name++)
            return false;
    }
    return *name == '\0';
}

static const rbusDataElement_t *xpon_find(const char *name, uint32_t instances[3])
{
    size_t index;
    if (!name || strlen(name) >= RBUS_MAX_NAME_LENGTH)
        return NULL;
    for (index = 0; index < g_elementCount; ++index)
        if (g_elements[index].type == RBUS_ELEMENT_TYPE_PROPERTY &&
            xpon_match(g_elements[index].name, name, instances))
            return &g_elements[index];
    return NULL;
}

static rbusError_t xpon_refresh(GPON_DML_DATA *data, XponSource source)
{
    struct timespec now;
    ANSC_STATUS result;
    if (source == XPON_NONE)
        return RBUS_ERROR_SUCCESS;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return RBUS_ERROR_BUS_ERROR;
    if (g_fetched[source] != 0 && now.tv_sec == g_fetched[source])
        return RBUS_ERROR_SUCCESS;
    switch (source)
    {
        case XPON_PHY: result = GponHal_get_pm(&data->gpon.PhysicalMedia); break;
        case XPON_GTC: result = GponHal_get_gtc(&data->gpon.Gtc); break;
        case XPON_PLOAM: result = GponHal_get_ploam(&data->gpon.Ploam); break;
        case XPON_OMCI: result = GponHal_get_omci(&data->gpon.Omci); break;
        case XPON_GEM: result = GponHal_get_gem(&data->gpon.Gem); break;
        case XPON_VEIP: result = GponHal_get_veip(&data->gpon.Veip); break;
        default: return RBUS_ERROR_SUCCESS;
    }
    if (result != ANSC_STATUS_SUCCESS)
    {
        CcspTraceWarning(("XPON: HAL refresh failed for source %d\n", source));
        return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
    }
    g_fetched[source] = now.tv_sec;
    return RBUS_ERROR_SUCCESS;
}

static DML_PHY_MEDIA *xpon_phy(GPON_DML_DATA *data, uint32_t instance)
{
    size_t index;
    for (index = 0; index < GPON_DATA_MAX_PM; ++index)
        if (data->gpon.PhysicalMedia.pdata[index] &&
            data->gpon.PhysicalMedia.pdata[index]->dml.uInstanceNumber == instance)
            return &data->gpon.PhysicalMedia.pdata[index]->dml;
    return NULL;
}

static DML_GEM *xpon_gem(GPON_DML_DATA *data, uint32_t instance)
{
    size_t index;
    for (index = 0; index < GPON_DATA_MAX_GEM; ++index)
        if (data->gpon.Gem.pdata[index] && data->gpon.Gem.pdata[index]->dml.uInstanceNumber == instance)
            return &data->gpon.Gem.pdata[index]->dml;
    return NULL;
}

static DML_VEIP *xpon_veip(GPON_DML_DATA *data, uint32_t instance)
{
    size_t index;
    for (index = 0; index < GPON_DATA_MAX_VEIP; ++index)
        if (data->gpon.Veip.pdata[index] && data->gpon.Veip.pdata[index]->dml.uInstanceNumber == instance)
            return &data->gpon.Veip.pdata[index]->dml;
    return NULL;
}

static DML_PHY_MEDIA *xpon_active_phy(GPON_DML_DATA *data)
{
    DML_PHY_MEDIA *only = NULL, *active = NULL;
    size_t index, count = 0, activeCount = 0;
    for (index = 0; index < GPON_DATA_MAX_PM; ++index)
    {
        if (!data->gpon.PhysicalMedia.pdata[index])
            continue;
        only = &data->gpon.PhysicalMedia.pdata[index]->dml;
        ++count;
        if (only->RedundancyState == Active)
        {
            active = only;
            ++activeCount;
        }
    }
    return count == 1 ? only : activeCount == 1 ? active : NULL;
}

static rbusError_t xpon_finish_get(XponRequest *request, rbusError_t result)
{
    if (result == RBUS_ERROR_SUCCESS)
        rbusProperty_SetValue(request->property, request->value);
    if (request->value)
        rbusValue_Release(request->value);
    GponMgrDml_GetData_release(request->data);
    pthread_mutex_unlock(&g_mutex);
    return result;
}

static rbusError_t xpon_begin_get(rbusProperty_t property, XponSource source, XponRequest *request)
{
    rbusError_t result;

    memset(request, 0, sizeof(*request));
    if (!property)
        return RBUS_ERROR_INVALID_INPUT;
    request->name = rbusProperty_GetName(property);
    if (!xpon_find(request->name, request->instances))
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if ((request->instances[0] && request->instances[0] != 1) ||
        (strstr(request->name, ".ANI.") && request->instances[1] != 1))
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    request->parameter = strrchr(request->name, '.') + 1;
    request->property = property;

    pthread_mutex_lock(&g_mutex);
    if (!g_ready)
    {
        pthread_mutex_unlock(&g_mutex);
        return RBUS_ERROR_NOT_INITIALIZED;
    }
    request->data = GponMgrDml_GetData_locked();
    if (!request->data)
    {
        pthread_mutex_unlock(&g_mutex);
        return RBUS_ERROR_NOT_INITIALIZED;
    }

    result = xpon_refresh(request->data, source);
    if (result == RBUS_ERROR_SUCCESS && strstr(request->name, ".EthernetUNI."))
    {
        result = xpon_refresh(request->data, XPON_VEIP);
        if (result == RBUS_ERROR_SUCCESS && !xpon_veip(request->data, request->instances[1]))
            result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    if (result == RBUS_ERROR_SUCCESS && strstr(request->name, ".Transceiver."))
    {
        result = xpon_refresh(request->data, XPON_PHY);
        if (result == RBUS_ERROR_SUCCESS && !xpon_phy(request->data, request->instances[2]))
            result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    if (result == RBUS_ERROR_SUCCESS && strstr(request->name, ".TC.GEM.Port."))
    {
        result = xpon_refresh(request->data, XPON_GEM);
        if (result == RBUS_ERROR_SUCCESS && !xpon_gem(request->data, request->instances[2]))
            result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    if (result == RBUS_ERROR_SUCCESS && !rbusValue_Init(&request->value))
        result = RBUS_ERROR_OUT_OF_RESOURCES;
    if (result != RBUS_ERROR_SUCCESS)
        return xpon_finish_get(request, result);
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t xpon_uint32(rbusValue_t value, uint64_t number)
{
    if (number > UINT32_MAX)
        return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
    rbusValue_SetUInt32(value, (uint32_t)number);
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t xpon_string(rbusValue_t value, const char *text, size_t capacity, size_t maxLength)
{
    size_t length = strnlen(text, capacity);
    if (length == capacity || length > maxLength)
        return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
    rbusValue_SetString(value, text);
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t xpon_enum(rbusValue_t value, unsigned int number,
                             const char *const *names, size_t count)
{
    if (number >= count)
        return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
    rbusValue_SetString(value, names[number]);
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t xpon_alarm(rbusValue_t value, ALARM_ENUM alarm)
{
    if (alarm != ACTIVE && alarm != INACTIVE)
        return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
    rbusValue_SetBoolean(value, alarm == ACTIVE);
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t xpon_transceiver_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_PHY_MEDIA *media;
    size_t index;
    rbusError_t result;
    static const char *const modes[] = {"G-PON", "XG-PON", "NG-PON2", "XGS-PON"};
    static const char *const connectors[] = {"LC", "ST", "FC", "SC", "MT-RJ"};
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_PHY, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    media = xpon_phy(request.data, request.instances[2]);

    if (strcmp(request.parameter, "ID") == 0)
    {
        result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
        for (index = 0; index < GPON_DATA_MAX_PM; ++index)
        {
            if (request.data->gpon.PhysicalMedia.pdata[index] &&
                &request.data->gpon.PhysicalMedia.pdata[index]->dml == media)
            {
                rbusValue_SetUInt32(request.value, (uint32_t)index);
                result = RBUS_ERROR_SUCCESS;
                break;
            }
        }
    }
    else if (strcmp(request.parameter, "VendorName") == 0)
        result = xpon_string(request.value, media->ModuleVendor, sizeof(media->ModuleVendor), 256);
    else if (strcmp(request.parameter, "VendorPartNumber") == 0)
        result = xpon_string(request.value, media->ModuleName, sizeof(media->ModuleName), 256);
    else if (strcmp(request.parameter, "VendorRevision") == 0)
        result = xpon_string(request.value, media->ModuleVersion, sizeof(media->ModuleVersion), 256);
    else if (strcmp(request.parameter, "NominalBitRateDownstream") == 0)
        rbusValue_SetUInt32(request.value, media->NominalBitRateDownstream);
    else if (strcmp(request.parameter, "NominalBitRateUpstream") == 0)
        rbusValue_SetUInt32(request.value, media->NominalBitRateUpstream);
    else if (strcmp(request.parameter, "Bias") == 0)
        rbusValue_SetUInt32(request.value, media->Bias.CurrentBias);
    else if (strcmp(request.parameter, "RxPower") == 0)
        rbusValue_SetInt32(request.value, media->RxPower.SignalLevel);
    else if (strcmp(request.parameter, "TxPower") == 0)
        rbusValue_SetInt32(request.value, media->TxPower.SignalLevel);
    else if (strcmp(request.parameter, "Temperature") == 0)
    {
        if (media->Temperature.CurrentTemp < -274)
            result = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        else
            rbusValue_SetInt32(request.value, media->Temperature.CurrentTemp);
    }
    else if (strcmp(request.parameter, "Voltage") == 0)
    {
        if (media->Voltage.VoltageLevel < 0)
            result = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        else
            rbusValue_SetUInt32(request.value, (uint32_t)media->Voltage.VoltageLevel);
    }
    else if (strcmp(request.parameter, "PONMode") == 0)
        result = xpon_enum(request.value, media->PonMode, modes, sizeof(modes) / sizeof(modes[0]));
    else if (strcmp(request.parameter, "Connector") == 0)
        result = xpon_enum(request.value, media->Connector, connectors, sizeof(connectors) / sizeof(connectors[0]));
    else
        result = xpon_unsupported(request.name, "no equivalent transceiver field");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_ani_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_PHY_MEDIA *media;
    rbusError_t result;
    size_t index;
    uint32_t count = 0;
    static const char *const states[] = {"Up", "Down", "Unknown", "Dormant", "NotPresent", "LowerLayerDown", "Error"};
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_PHY, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    if (strcmp(request.parameter, "TransceiverNumberOfEntries") == 0)
    {
        for (index = 0; index < GPON_DATA_MAX_PM; ++index)
            if (request.data->gpon.PhysicalMedia.pdata[index])
                ++count;
        rbusValue_SetUInt32(request.value, count);
        return xpon_finish_get(&request, RBUS_ERROR_SUCCESS);
    }
    media = xpon_active_phy(request.data);
    if (!media)
        return xpon_finish_get(&request, xpon_unsupported(request.name, "no uniquely active physical medium"));

    if (strcmp(request.parameter, "Status") == 0)
    {
        if (media->Status == LowerLayerDown)
            result = xpon_unsupported(request.name, "LowerLayerDown has no BBF ANI equivalent");
        else
            result = xpon_enum(request.value, media->Status, states, sizeof(states) / sizeof(states[0]));
    }
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    else if (strcmp(request.parameter, "Alias") == 0)
        result = xpon_string(request.value, media->Alias, sizeof(media->Alias), 64);
    else if (strcmp(request.parameter, "LastChange") == 0)
        rbusValue_SetUInt32(request.value, media->LastChange);
    else if (strcmp(request.parameter, "Enable") == 0)
        rbusValue_SetBoolean(request.value, media->Enable != 0);
#endif
    else
        result = xpon_unsupported(request.name, "requires WAN_MANAGER_UNIFICATION_ENABLED");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_threshold_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_PHY_MEDIA *media;
    rbusError_t result;
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_PHY, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    media = xpon_active_phy(request.data);
    if (!media)
        return xpon_finish_get(&request, xpon_unsupported(request.name, "no uniquely active physical medium"));

    if (strcmp(request.parameter, "SignalFail") == 0)
    {
        if (media->PerformThreshold.SignalFail < 3 || media->PerformThreshold.SignalFail > 8)
            result = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        else
            rbusValue_SetUInt32(request.value, (uint32_t)media->PerformThreshold.SignalFail);
    }
    else if (strcmp(request.parameter, "SignalDegrade") == 0)
    {
        if (media->PerformThreshold.SignalDegrade < 4 || media->PerformThreshold.SignalDegrade > 10 ||
            media->PerformThreshold.SignalDegrade <= media->PerformThreshold.SignalFail)
            result = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        else
            rbusValue_SetUInt32(request.value, (uint32_t)media->PerformThreshold.SignalDegrade);
    }
    else
        result = xpon_unsupported(request.name, "no equivalent threshold field");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_alarm_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_PHY_MEDIA *media;
    ALARM_ENUM alarm;
    rbusError_t result;
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_PHY, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    media = xpon_active_phy(request.data);
    if (!media)
        return xpon_finish_get(&request, xpon_unsupported(request.name, "no uniquely active physical medium"));

    if (strcmp(request.parameter, "RDI") == 0)
        alarm = media->Alarm.RDI;
    else if (strcmp(request.parameter, "PEE") == 0)
        alarm = media->Alarm.PEE;
    else if (strcmp(request.parameter, "LOS") == 0)
        alarm = media->Alarm.LOS;
    else if (strcmp(request.parameter, "LOF") == 0)
        alarm = media->Alarm.LOF;
    else if (strcmp(request.parameter, "DACT") == 0)
        alarm = media->Alarm.DACT;
    else if (strcmp(request.parameter, "DIS") == 0)
        alarm = media->Alarm.DIS;
    else if (strcmp(request.parameter, "MIS") == 0)
        alarm = media->Alarm.MIS;
    else if (strcmp(request.parameter, "MEM") == 0)
        alarm = media->Alarm.MEM;
    else if (strcmp(request.parameter, "SUF") == 0)
        alarm = media->Alarm.SUF;
    else if (strcmp(request.parameter, "SF") == 0)
        alarm = media->Alarm.SF;
    else if (strcmp(request.parameter, "SD") == 0)
        alarm = media->Alarm.SD;
    else if (strcmp(request.parameter, "LCDG") == 0)
        alarm = media->Alarm.LCDG;
    else if (strcmp(request.parameter, "TF") == 0)
        alarm = media->Alarm.TF;
    else if (strcmp(request.parameter, "ROGUE") == 0)
        alarm = media->Alarm.ROGUE;
    else
        return xpon_finish_get(&request, xpon_unsupported(request.name, "no equivalent alarm field"));
    return xpon_finish_get(&request, xpon_alarm(request.value, alarm));
}

static int xpon_hex(unsigned char character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

static rbusError_t xpon_serial(const char *name, DML_PLOAM *ploam, rbusValue_t value)
{
    char serial[13];
    size_t index, length = strnlen(ploam->SerialNumber, sizeof(ploam->SerialNumber));
    if (length == 16)
    {
        for (index = 0; index < 4; ++index)
        {
            int high = xpon_hex((unsigned char)ploam->SerialNumber[index * 2]);
            int low = xpon_hex((unsigned char)ploam->SerialNumber[index * 2 + 1]);
            if (high < 0 || low < 0 || high * 16 + low < 33 || high * 16 + low > 126)
                return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
            serial[index] = (char)(high * 16 + low);
        }
        for (index = 0; index < 8; ++index)
        {
            if (xpon_hex((unsigned char)ploam->SerialNumber[index + 8]) < 0)
                return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
            serial[index + 4] = (char)toupper((unsigned char)ploam->SerialNumber[index + 8]);
        }
        serial[12] = '\0';
    }
    else if (length == 12)
    {
        memcpy(serial, ploam->SerialNumber, sizeof(serial));
        for (index = 0; index < 4; ++index)
            if ((unsigned char)serial[index] < 33 || (unsigned char)serial[index] > 126)
                return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        for (index = 4; index < 12; ++index)
            if (xpon_hex((unsigned char)serial[index]) < 0)
                return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
    }
    else
        return xpon_unsupported(name, "serial must be 8-byte hex or canonical 12-character form");
    return xpon_string(value, serial, sizeof(serial), 12);
}

static rbusError_t xpon_base_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    rbusError_t result;
    size_t index;
    uint32_t count = 0;
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_NONE, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    if (strcmp(request.parameter, "ONUNumberOfEntries") == 0 ||
        strcmp(request.parameter, "ANINumberOfEntries") == 0)
        rbusValue_SetUInt32(request.value, 1);
    else if (strcmp(request.parameter, "SoftwareImageNumberOfEntries") == 0)
        rbusValue_SetUInt32(request.value, 0);
    else if (strcmp(request.parameter, "EthernetUNINumberOfEntries") == 0)
    {
        result = xpon_refresh(request.data, XPON_VEIP);
        if (result == RBUS_ERROR_SUCCESS)
        {
            for (index = 0; index < GPON_DATA_MAX_VEIP; ++index)
                if (request.data->gpon.Veip.pdata[index])
                    ++count;
            rbusValue_SetUInt32(request.value, count);
        }
    }
    else
        result = xpon_unsupported(request.name, "no equivalent ONU field");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_gtc_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_GTC *gtc;
    rbusError_t result;
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_GTC, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    gtc = &request.data->gpon.Gtc;
    if (strcmp(request.parameter, "CorrectedFECBytes") == 0)
        rbusValue_SetUInt64(request.value, gtc->CorrectedFecBytes);
    else if (strcmp(request.parameter, "CorrectedFECCodewords") == 0)
        rbusValue_SetUInt64(request.value, gtc->CorrectedFecCodeWords);
    else if (strcmp(request.parameter, "UncorrectableFECCodewords") == 0)
        rbusValue_SetUInt64(request.value, gtc->UnCorrectedFecCodeWords);
    else if (strcmp(request.parameter, "TotalFECCodewords") == 0)
        rbusValue_SetUInt64(request.value, gtc->TotalFecCodeWords);
    else if (strcmp(request.parameter, "PSBdHECErrorCount") == 0)
        rbusValue_SetUInt32(request.value, (uint32_t)gtc->PSBdHecErrors);
    else if (strcmp(request.parameter, "HeaderHECErrorCount") == 0)
        rbusValue_SetUInt32(request.value, (uint32_t)gtc->HecErrorCount);
    else if (strcmp(request.parameter, "FrameHeaderHECErrors") == 0)
        rbusValue_SetUInt32(request.value, (uint32_t)gtc->FrameHecErrors);
    else
        result = xpon_unsupported(request.name, "no equivalent GTC counter");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_activation_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_PLOAM *ploam;
    rbusError_t result;
    static const char *const states[] = {"O1", "O2", "O3", "O4", "O5", "O6", "O7", "O8", "O9"};
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_PLOAM, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    ploam = &request.data->gpon.Ploam;
    if (strcmp(request.parameter, "ONUID") == 0)
    {
        if (ploam->OnuId > 1022)
            result = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        else
            rbusValue_SetUInt32(request.value, ploam->OnuId);
    }
    else if (strcmp(request.parameter, "VendorID") == 0)
        result = xpon_string(request.value, ploam->VendorId, sizeof(ploam->VendorId), 4);
    else if (strcmp(request.parameter, "SerialNumber") == 0)
        result = xpon_serial(request.name, ploam, request.value);
    else if (strcmp(request.parameter, "ONUState") == 0)
        result = xpon_enum(request.value, ploam->RegistrationState, states, sizeof(states) / sizeof(states[0]));
    else
        result = xpon_unsupported(request.name, "no equivalent ONU activation field");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_ploam_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_PLOAM *ploam;
    rbusError_t result;
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_PLOAM, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    ploam = &request.data->gpon.Ploam;
    if (strcmp(request.parameter, "UpstreamMessageCount") == 0)
        rbusValue_SetUInt64(request.value, ploam->TxMessageCount);
    else if (strcmp(request.parameter, "DownstreamMessageCount") == 0)
        rbusValue_SetUInt64(request.value, ploam->RxMessageCount);
    else if (strcmp(request.parameter, "MICErrors") == 0)
        result = xpon_uint32(request.value, ploam->MicErrors);
    else
        result = xpon_unsupported(request.name, "no equivalent PLOAM counter");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_omci_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_OMCI *omci;
    rbusError_t result;
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_OMCI, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    omci = &request.data->gpon.Omci;
    if (strcmp(request.parameter, "BaselineMessagesReceived") == 0)
        rbusValue_SetUInt64(request.value, omci->RxBaseLineMessageCountValid);
    else if (strcmp(request.parameter, "ExtendedMessagesReceived") == 0)
        rbusValue_SetUInt64(request.value, omci->RxExtendedMessageCountValid);
    else if (strcmp(request.parameter, "MICErrors") == 0)
        result = xpon_uint32(request.value, omci->MicErrors);
    else
        result = xpon_unsupported(request.name, "no equivalent OMCI counter");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_gem_count_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    rbusError_t result;
    size_t index;
    uint32_t count = 0;
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_GEM, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    for (index = 0; index < GPON_DATA_MAX_GEM; ++index)
        if (request.data->gpon.Gem.pdata[index])
            ++count;
    rbusValue_SetUInt32(request.value, count);
    return xpon_finish_get(&request, RBUS_ERROR_SUCCESS);
}

static rbusError_t xpon_gem_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_GEM *gem;
    rbusError_t result;
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_GEM, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    gem = xpon_gem(request.data, request.instances[2]);
    if (strcmp(request.parameter, "PortID") == 0)
    {
        if (gem->PortId > 65534)
            result = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        else
            rbusValue_SetUInt32(request.value, (uint32_t)gem->PortId);
    }
    else if (strcmp(request.parameter, "FramesSent") == 0)
        rbusValue_SetUInt64(request.value, gem->TransmittedFrames);
    else if (strcmp(request.parameter, "FramesReceived") == 0)
        rbusValue_SetUInt64(request.value, gem->ReceivedFrames);
    else if (strcmp(request.parameter, "Direction") == 0)
    {
        if (gem->TrafficType != Bidirectional)
            result = xpon_unsupported(request.name, "HAL does not specify unidirectional orientation");
        else
            rbusValue_SetString(request.value, "bidirectional");
    }
    else
        result = xpon_unsupported(request.name, "no equivalent GEM field");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_veip_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    DML_VEIP *veip;
    rbusError_t result;
    static const char *const states[] = {"Up", "Down", "Unknown", "Dormant", "NotPresent", "LowerLayerDown", "Error"};
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_VEIP, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    veip = xpon_veip(request.data, request.instances[1]);
    if (strcmp(request.parameter, "Name") == 0)
        result = xpon_string(request.value, veip->InterfaceName, sizeof(veip->InterfaceName), 64);
    else if (strcmp(request.parameter, "InterdomainName") == 0)
        result = xpon_string(request.value, veip->InterDomainName, sizeof(veip->InterDomainName), 25);
    else if (strcmp(request.parameter, "Enable") == 0)
    {
        if (veip->AdministrativeState != Lock && veip->AdministrativeState != Unlock)
            result = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        else
            rbusValue_SetBoolean(request.value, veip->AdministrativeState == Unlock);
    }
    else if (strcmp(request.parameter, "Status") == 0)
        result = xpon_enum(request.value, veip->OperationalState, states, sizeof(states) / sizeof(states[0]));
    else if (strcmp(request.parameter, "InterdomainID") == 0)
    {
        char identity[48];
        snprintf(identity, sizeof(identity), "(VEIP,%lu)", (unsigned long)veip->MeId);
        rbusValue_SetString(request.value, identity);
    }
    else if (strcmp(request.parameter, "ANIs") == 0)
        rbusValue_SetString(request.value, "Device.XPON.ONU.1.ANI.1");
    else
        result = xpon_unsupported(request.name, "no equivalent VEIP field");
    return xpon_finish_get(&request, result);
}

static rbusError_t xpon_unsupported_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    XponRequest request;
    rbusError_t result;
    (void)handle;
    (void)options;

    result = xpon_begin_get(property, XPON_NONE, &request);
    if (result != RBUS_ERROR_SUCCESS)
        return result;
    return xpon_finish_get(&request, xpon_unsupported(request.name, "no equivalent unified HAL field"));
}

static rbusError_t xpon_ani_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t *options)
{
    const rbusDataElement_t *element;
    const char *name;
    const char *parameter;
    uint32_t instances[3];
    bool alias;
    rbusValue_t value;
    rbusError_t result;
    (void)handle;
    if (!property)
        return RBUS_ERROR_INVALID_INPUT;
    name = rbusProperty_GetName(property);
    element = xpon_find(name, instances);
    if (!element)
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if (element->cbTable.setHandler != xpon_ani_set_handler)
        return RBUS_ERROR_NOT_WRITABLE;
    parameter = strrchr(name, '.') + 1;
    alias = strcmp(parameter, "Alias") == 0;
    value = rbusProperty_GetValue(property);
    if (!value || rbusValue_GetType(value) != (alias ? RBUS_STRING : RBUS_BOOLEAN))
        return RBUS_ERROR_INVALID_PARAMETER_TYPE;
    if (instances[0] != 1 || instances[1] != 1)
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if (options && (options->sessionId || !options->commit))
        return xpon_unsupported(name, "transactional writes are unavailable");
    pthread_mutex_lock(&g_mutex);
    if (!g_ready)
        result = RBUS_ERROR_NOT_INITIALIZED;
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    else
    {
        GPON_DML_DATA *data = GponMgrDml_GetData_locked();
        if (!data)
            result = RBUS_ERROR_NOT_INITIALIZED;
        else
        {
            DML_PHY_MEDIA *media;
            result = xpon_refresh(data, XPON_PHY);
            media = result == RBUS_ERROR_SUCCESS ? xpon_active_phy(data) : NULL;
            if (result == RBUS_ERROR_SUCCESS && !media)
                result = xpon_unsupported(name, "no uniquely active physical medium");
            if (media)
            {
                char halName[RBUS_MAX_NAME_LENGTH];
                char halValue[65];
                const char *text = alias ? rbusValue_GetString(value, NULL) : NULL;
                if (alias && (!text || !isalpha((unsigned char)text[0]) || strlen(text) >= sizeof(media->Alias)))
                    result = RBUS_ERROR_INVALID_PARAMETER_VALUE;
                else
                {
                    snprintf(halName, sizeof(halName), "Device.X_RDK_ONT.PhysicalMedia.%lu.%s",
                             (unsigned long)media->uInstanceNumber, parameter);
                    snprintf(halValue, sizeof(halValue), "%s", alias ? text : rbusValue_GetBoolean(value) ? "true" : "false");
                    if (GponHal_setParam(halName, alias ? PARAM_STRING : PARAM_BOOLEAN, halValue) != ANSC_STATUS_SUCCESS)
                        result = RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
                    else
                    {
                        if (alias) snprintf(media->Alias, sizeof(media->Alias), "%s", text);
                        else media->Enable = rbusValue_GetBoolean(value);
                        g_fetched[XPON_PHY] = 0;
                        result = RBUS_ERROR_SUCCESS;
                    }
                }
            }
            GponMgrDml_GetData_release(data);
        }
    }
#else
    else
        result = xpon_unsupported(name, "requires WAN_MANAGER_UNIFICATION_ENABLED");
#endif
    pthread_mutex_unlock(&g_mutex);
    return result;
}

static rbusError_t xpon_unsupported_set_handler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t *options)
{
    const rbusDataElement_t *element;
    const char *name;
    const char *parameter;
    uint32_t instances[3];
    rbusValue_t value;
    rbusValueType_t type;
    rbusError_t result;
    (void)handle;

    if (!property)
        return RBUS_ERROR_INVALID_INPUT;
    name = rbusProperty_GetName(property);
    element = xpon_find(name, instances);
    if (!element)
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if (element->cbTable.setHandler != xpon_unsupported_set_handler)
        return RBUS_ERROR_NOT_WRITABLE;
    parameter = strrchr(name, '.') + 1;
    if (strcmp(parameter, "Enable") == 0 || strcmp(parameter, "Reset") == 0 ||
        strcmp(parameter, "HexadecimalPassword") == 0)
        type = RBUS_BOOLEAN;
    else
        type = RBUS_STRING;
    value = rbusProperty_GetValue(property);
    if (!value || rbusValue_GetType(value) != type)
        return RBUS_ERROR_INVALID_PARAMETER_TYPE;
    if (instances[0] != 1 || (strstr(name, ".ANI.") && instances[1] != 1))
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if (options && (options->sessionId || !options->commit))
        return xpon_unsupported(name, "transactional writes are unavailable");

    pthread_mutex_lock(&g_mutex);
    result = g_ready ? xpon_unsupported(name, "no equivalent writable HAL field") : RBUS_ERROR_NOT_INITIALIZED;
    pthread_mutex_unlock(&g_mutex);
    return result;
}

static bool xpon_has_row(char rows[][RBUS_MAX_NAME_LENGTH], size_t count, const char *name)
{
    size_t index;
    for (index = 0; index < count; ++index)
        if (strcmp(rows[index], name) == 0) return true;
    return false;
}

static rbusError_t xpon_sync_locked(void)
{
    char desired[XPON_MAX_ROWS][RBUS_MAX_NAME_LENGTH];
    size_t count = 2, index;
    GPON_DML_DATA *data = GponMgrDml_GetData_locked();
    rbusError_t result;
    if (!data) return RBUS_ERROR_NOT_INITIALIZED;
    result = xpon_refresh(data, XPON_PHY);
    if (result == RBUS_ERROR_SUCCESS) result = xpon_refresh(data, XPON_GEM);
    if (result == RBUS_ERROR_SUCCESS) result = xpon_refresh(data, XPON_VEIP);
    if (result != RBUS_ERROR_SUCCESS)
    {
        GponMgrDml_GetData_release(data);
        return result;
    }
    snprintf(desired[0], sizeof(desired[0]), "Device.XPON.ONU.1");
    snprintf(desired[1], sizeof(desired[1]), "Device.XPON.ONU.1.ANI.1");
    for (index = 0; index < GPON_DATA_MAX_PM; ++index)
        if (data->gpon.PhysicalMedia.pdata[index])
            snprintf(desired[count++], RBUS_MAX_NAME_LENGTH, XPON_ANI "Transceiver.%lu",
                     (unsigned long)data->gpon.PhysicalMedia.pdata[index]->dml.uInstanceNumber);
    for (index = 0; index < GPON_DATA_MAX_GEM; ++index)
        if (data->gpon.Gem.pdata[index])
            snprintf(desired[count++], RBUS_MAX_NAME_LENGTH, XPON_ANI "TC.GEM.Port.%lu",
                     (unsigned long)data->gpon.Gem.pdata[index]->dml.uInstanceNumber);
    for (index = 0; index < GPON_DATA_MAX_VEIP; ++index)
        if (data->gpon.Veip.pdata[index])
            snprintf(desired[count++], RBUS_MAX_NAME_LENGTH, "Device.XPON.ONU.1.EthernetUNI.%lu",
                     (unsigned long)data->gpon.Veip.pdata[index]->dml.uInstanceNumber);
    GponMgrDml_GetData_release(data);
    for (index = g_rowCount; index > 0; --index)
    {
        size_t slot = index - 1;
        if (!xpon_has_row(desired, count, g_rows[slot]))
        {
            result = rbusTable_unregisterRow(g_handle, g_rows[slot]);
            if (result != RBUS_ERROR_SUCCESS) return result;
            memmove(g_rows[slot], g_rows[slot + 1], (g_rowCount - slot - 1) * sizeof(g_rows[0]));
            --g_rowCount;
        }
    }
    for (index = 0; index < count; ++index)
    {
        if (!xpon_has_row(g_rows, g_rowCount, desired[index]))
        {
            char table[RBUS_MAX_NAME_LENGTH];
            char *last;
            unsigned long instance;
            snprintf(table, sizeof(table), "%s", desired[index]);
            last = strrchr(table, '.');
            instance = strtoul(last + 1, NULL, 10);
            if (!instance || instance > UINT32_MAX)
                return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
            last[1] = '\0';
            result = rbusTable_registerRow(g_handle, table, (uint32_t)instance, NULL);
            if (result != RBUS_ERROR_SUCCESS) return result;
            snprintf(g_rows[g_rowCount++], RBUS_MAX_NAME_LENGTH, "%s", desired[index]);
        }
    }
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t xpon_sync(rbusHandle_t handle, const char *tableName)
{
    rbusError_t result;
    (void)handle;
    (void)tableName;
    pthread_mutex_lock(&g_mutex);
    result = g_ready ? xpon_sync_locked() : RBUS_ERROR_NOT_INITIALIZED;
    pthread_mutex_unlock(&g_mutex);
    return result;
}

static rbusError_t xpon_add(rbusHandle_t handle, const char *name, const char *alias, uint32_t *instance)
{
    (void)handle; (void)name; (void)alias; (void)instance;
    return RBUS_ERROR_NOT_WRITABLE;
}

static rbusError_t xpon_remove(rbusHandle_t handle, const char *name)
{
    (void)handle; (void)name;
    return RBUS_ERROR_NOT_WRITABLE;
}

static rbusError_t xpon_subscribe(rbusHandle_t handle, rbusEventSubAction_t action,
                                 const char *name, rbusFilter_t filter, int32_t interval, bool *autoPublish)
{
    const rbusDataElement_t *element;
    uint32_t instances[3];
    (void)handle; (void)filter; (void)interval;
    if (!autoPublish) return RBUS_ERROR_INVALID_INPUT;
    *autoPublish = false;
    if (action == RBUS_EVENT_ACTION_UNSUBSCRIBE) return RBUS_ERROR_SUCCESS;
    element = xpon_find(name, instances);
    if (!element) return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if (element->cbTable.getHandler == xpon_unsupported_get_handler)
        return xpon_unsupported(name, "events for an unsupported parameter");
    *autoPublish = true;
    return RBUS_ERROR_SUCCESS;
}

int GponMgrXponTr181_Init(rbusHandle_t handle)
{
    size_t index;
    size_t parameterCount = 0;
    rbusError_t result;
    if (!handle)
        return RBUS_ERROR_INVALID_HANDLE;
    pthread_mutex_lock(&g_mutex);
    if (g_ready)
    {
        pthread_mutex_unlock(&g_mutex);
        return RBUS_ERROR_SUCCESS;
    }
    g_handle = handle;
    memset(g_fetched, 0, sizeof(g_fetched));
    g_rowCount = 0;
    result = rbus_regDataElements(g_handle, (int)g_elementCount, g_elements);
    if (result != RBUS_ERROR_SUCCESS) goto failed;
    for (index = 0; index < g_elementCount; ++index)
    {
        if (g_elements[index].type == RBUS_ELEMENT_TYPE_TABLE)
        {
            result = rbus_registerDynamicTableSyncHandler(g_handle, g_elements[index].name, xpon_sync);
            if (result != RBUS_ERROR_SUCCESS) goto failed;
        }
        else
            ++parameterCount;
    }
    result = xpon_sync_locked();
    if (result != RBUS_ERROR_SUCCESS) goto failed;
    g_ready = true;
    CcspTraceInfo(("XPON: registered %zu BBF parameters alongside Device.X_RDK_ONT\n", parameterCount));
    goto done;
failed:
    g_handle = NULL;
    g_rowCount = 0;
done:
    pthread_mutex_unlock(&g_mutex);
    return result;
}

void GponMgrXponTr181_Close(void)
{
    pthread_mutex_lock(&g_mutex);
    g_ready = false;
    g_handle = NULL;
    g_rowCount = 0;
    pthread_mutex_unlock(&g_mutex);
}