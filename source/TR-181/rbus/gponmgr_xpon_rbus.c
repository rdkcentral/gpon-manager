#include "gponmgr_xpon_rbus.h"
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
    XPON_NONE, XPON_META, XPON_PHY, XPON_ACTIVE_PHY,
    XPON_GTC, XPON_PLOAM, XPON_OMCI, XPON_GEM, XPON_VEIP,
    XPON_SOURCE_COUNT
} XponSource;

typedef struct
{
    const char *name;
    rbusValueType_t type;
    bool writable;
    XponSource source;
    const char *member;
    size_t maxLength;
} XponParameter;

static const XponParameter g_parameters[] = {
#define XPON_TABLE(name)
#define XPON_PARAM(name, type, writable, source, member, length) \
    {name, RBUS_##type, writable, XPON_##source, member, length},
#include "gponmgr_xpon_params.def"
#undef XPON_PARAM
#undef XPON_TABLE
};

static const char *g_tables[] = {
#define XPON_TABLE(name) name,
#define XPON_PARAM(name, type, writable, source, member, length)
#include "gponmgr_xpon_params.def"
#undef XPON_PARAM
#undef XPON_TABLE
};

#define XPON_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define XPON_MAX_ROWS (2 + GPON_DATA_MAX_PM + GPON_DATA_MAX_GEM + GPON_DATA_MAX_VEIP)
#define XPON_ANI "Device.XPON.ONU.1.ANI.1."
#define XPON_UNSUPPORTED RBUS_ERROR_INVALID_OPERATION

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static rbusHandle_t g_handle;
static bool g_ready;
static time_t g_fetched[XPON_SOURCE_COUNT];
static char g_rows[XPON_MAX_ROWS][RBUS_MAX_NAME_LENGTH];
static size_t g_rowCount;
static rbusDataElement_t g_elements[XPON_COUNT(g_parameters) + XPON_COUNT(g_tables)];

static rbusError_t xpon_unsupported(const char *name, const char *reason)
{
    CcspTraceWarning(("XPON: %s not supported yet (%s)\n", name, reason));
    return XPON_UNSUPPORTED;
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

static const XponParameter *xpon_find(const char *name, uint32_t instances[3])
{
    size_t index;
    if (!name || strlen(name) >= RBUS_MAX_NAME_LENGTH)
        return NULL;
    for (index = 0; index < XPON_COUNT(g_parameters); ++index)
        if (xpon_match(g_parameters[index].name, name, instances))
            return &g_parameters[index];
    return NULL;
}

static rbusError_t xpon_refresh(GPON_DML_DATA *data, XponSource source)
{
    struct timespec now;
    ANSC_STATUS result;
    if (source == XPON_ACTIVE_PHY)
        source = XPON_PHY;
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

static rbusError_t xpon_uint(const XponParameter *parameter, rbusValue_t value, uint64_t number)
{
    if (parameter->type == RBUS_UINT64)
        rbusValue_SetUInt64(value, number);
    else if (parameter->type == RBUS_UINT32 &&
             (number <= UINT32_MAX || parameter->source == XPON_GTC))
        rbusValue_SetUInt32(value, (uint32_t)number);
    else
        return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t xpon_string(const XponParameter *parameter, rbusValue_t value,
                               const char *text, size_t capacity)
{
    size_t length = strnlen(text, capacity);
    if (length == capacity || (parameter->maxLength && length > parameter->maxLength))
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

#define XPON_READ_UINT(object, field) \
    if (strcmp(parameter->member, #field) == 0) \
        return xpon_uint(parameter, value, (object)->field)
#define XPON_READ_STRING(object, field) \
    if (strcmp(parameter->member, #field) == 0) \
        return xpon_string(parameter, value, (object)->field, sizeof((object)->field))
#define XPON_READ_INT(object, field) \
    if (strcmp(parameter->member, #field) == 0) \
    { rbusValue_SetInt32(value, (object)->field); return RBUS_ERROR_SUCCESS; }
#define XPON_READ_ALARM(field) \
    if (strcmp(parameter->member, "Alarm." #field) == 0) \
    { \
        if (media->Alarm.field != ACTIVE && media->Alarm.field != INACTIVE) \
            return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION; \
        rbusValue_SetBoolean(value, media->Alarm.field == ACTIVE); \
        return RBUS_ERROR_SUCCESS; \
    }

static rbusError_t xpon_read_phy(const XponParameter *parameter, DML_PHY_MEDIA *media, rbusValue_t value)
{
    static const char *const modes[] = {"G-PON", "XG-PON", "NG-PON2", "XGS-PON"};
    static const char *const connectors[] = {"LC", "ST", "FC", "SC", "MT-RJ"};
    static const char *const states[] = {"Up", "Down", "Unknown", "Dormant", "NotPresent", "LowerLayerDown", "Error"};
    XPON_READ_STRING(media, ModuleVendor);
    XPON_READ_STRING(media, ModuleName);
    XPON_READ_STRING(media, ModuleVersion);
    XPON_READ_UINT(media, NominalBitRateDownstream);
    XPON_READ_UINT(media, NominalBitRateUpstream);
    XPON_READ_UINT(media, Bias.CurrentBias);
    XPON_READ_INT(media, RxPower.SignalLevel);
    XPON_READ_INT(media, TxPower.SignalLevel);
    if (strcmp(parameter->member, "Temperature.CurrentTemp") == 0)
    {
        if (media->Temperature.CurrentTemp < -274)
            return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        rbusValue_SetInt32(value, media->Temperature.CurrentTemp);
        return RBUS_ERROR_SUCCESS;
    }
    if (strcmp(parameter->member, "Voltage.VoltageLevel") == 0)
    {
        if (media->Voltage.VoltageLevel < 0)
            return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        return xpon_uint(parameter, value, (uint32_t)media->Voltage.VoltageLevel);
    }
    if (strcmp(parameter->member, "PonMode") == 0)
        return xpon_enum(value, media->PonMode, modes, XPON_COUNT(modes));
    if (strcmp(parameter->member, "Connector") == 0)
        return xpon_enum(value, media->Connector, connectors, XPON_COUNT(connectors));
    if (strcmp(parameter->member, "Status") == 0)
    {
        if (media->Status == LowerLayerDown)
            return xpon_unsupported(parameter->name, "LowerLayerDown has no BBF ANI equivalent");
        return xpon_enum(value, media->Status, states, XPON_COUNT(states));
    }
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    XPON_READ_STRING(media, Alias);
    XPON_READ_UINT(media, LastChange);
    if (strcmp(parameter->member, "Enable") == 0)
    {
        rbusValue_SetBoolean(value, media->Enable != 0);
        return RBUS_ERROR_SUCCESS;
    }
#endif
    if (strcmp(parameter->member, "PerformThreshold.SignalFail") == 0)
    {
        if (media->PerformThreshold.SignalFail < 3 || media->PerformThreshold.SignalFail > 8)
            return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        return xpon_uint(parameter, value, media->PerformThreshold.SignalFail);
    }
    if (strcmp(parameter->member, "PerformThreshold.SignalDegrade") == 0)
    {
        if (media->PerformThreshold.SignalDegrade < 4 || media->PerformThreshold.SignalDegrade > 10 ||
            media->PerformThreshold.SignalDegrade <= media->PerformThreshold.SignalFail)
            return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
        return xpon_uint(parameter, value, media->PerformThreshold.SignalDegrade);
    }
    XPON_READ_ALARM(RDI);
    XPON_READ_ALARM(PEE);
    XPON_READ_ALARM(LOS);
    XPON_READ_ALARM(LOF);
    XPON_READ_ALARM(DACT);
    XPON_READ_ALARM(DIS);
    XPON_READ_ALARM(MIS);
    XPON_READ_ALARM(MEM);
    XPON_READ_ALARM(SUF);
    XPON_READ_ALARM(SF);
    XPON_READ_ALARM(SD);
    XPON_READ_ALARM(LCDG);
    XPON_READ_ALARM(TF);
    XPON_READ_ALARM(ROGUE);
    return xpon_unsupported(parameter->name, "requires WAN_MANAGER_UNIFICATION_ENABLED");
}

static int xpon_hex(unsigned char character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

static rbusError_t xpon_serial(const XponParameter *parameter, DML_PLOAM *ploam, rbusValue_t value)
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
        return xpon_unsupported(parameter->name, "serial must be 8-byte hex or canonical 12-character form");
    return xpon_string(parameter, value, serial, sizeof(serial));
}

static rbusError_t xpon_read_data(const XponParameter *parameter, const uint32_t instances[3],
                                 GPON_DML_DATA *data, rbusValue_t value)
{
    DML_PHY_MEDIA *media;
    DML_GEM *gem;
    DML_VEIP *veip;
    rbusError_t result;
    size_t index;
    uint32_t count = 0;
    static const char *const states[] = {"Up", "Down", "Unknown", "Dormant", "NotPresent", "LowerLayerDown", "Error"};
    static const char *const activation[] = {"O1", "O2", "O3", "O4", "O5", "O6", "O7", "O8", "O9"};

    if ((instances[0] && instances[0] != 1) ||
        (strstr(parameter->name, ".ANI.") && instances[1] != 1))
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if (strstr(parameter->name, ".EthernetUNI."))
    {
        result = xpon_refresh(data, XPON_VEIP);
        if (result != RBUS_ERROR_SUCCESS) return result;
        if (!xpon_veip(data, instances[1])) return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    if (strstr(parameter->name, ".Transceiver."))
    {
        result = xpon_refresh(data, XPON_PHY);
        if (result != RBUS_ERROR_SUCCESS) return result;
        if (!xpon_phy(data, instances[2])) return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    if (strstr(parameter->name, ".Port.{i}."))
    {
        result = xpon_refresh(data, XPON_GEM);
        if (result != RBUS_ERROR_SUCCESS) return result;
        if (!xpon_gem(data, instances[2])) return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    if (parameter->source == XPON_NONE)
        return xpon_unsupported(parameter->name, "no equivalent unified HAL field");
    if (parameter->source == XPON_META)
    {
        if (strcmp(parameter->member, "ONUNumberOfEntries") == 0 ||
            strcmp(parameter->member, "ANINumberOfEntries") == 0)
            return xpon_uint(parameter, value, 1);
        if (strcmp(parameter->member, "SoftwareImageNumberOfEntries") == 0)
            return xpon_uint(parameter, value, 0);
        if (strcmp(parameter->member, "ANIs") == 0)
        {
            rbusValue_SetString(value, "Device.XPON.ONU.1.ANI.1");
            return RBUS_ERROR_SUCCESS;
        }
        if (strcmp(parameter->member, "TransceiverID") == 0)
        {
            for (index = 0; index < GPON_DATA_MAX_PM; ++index)
                if (data->gpon.PhysicalMedia.pdata[index] &&
                    data->gpon.PhysicalMedia.pdata[index]->dml.uInstanceNumber == instances[2])
                    return xpon_uint(parameter, value, index);
        }
        if (strcmp(parameter->member, "TransceiverNumberOfEntries") == 0)
        {
            result = xpon_refresh(data, XPON_PHY);
            if (result != RBUS_ERROR_SUCCESS) return result;
            for (index = 0; index < GPON_DATA_MAX_PM; ++index)
                if (data->gpon.PhysicalMedia.pdata[index]) ++count;
        }
        else if (strcmp(parameter->member, "EthernetUNINumberOfEntries") == 0)
        {
            result = xpon_refresh(data, XPON_VEIP);
            if (result != RBUS_ERROR_SUCCESS) return result;
            for (index = 0; index < GPON_DATA_MAX_VEIP; ++index)
                if (data->gpon.Veip.pdata[index]) ++count;
        }
        else if (strcmp(parameter->member, "PortNumberOfEntries") == 0)
        {
            result = xpon_refresh(data, XPON_GEM);
            if (result != RBUS_ERROR_SUCCESS) return result;
            for (index = 0; index < GPON_DATA_MAX_GEM; ++index)
                if (data->gpon.Gem.pdata[index]) ++count;
        }
        return xpon_uint(parameter, value, count);
    }
    result = xpon_refresh(data, parameter->source);
    if (result != RBUS_ERROR_SUCCESS) return result;
    switch (parameter->source)
    {
        case XPON_PHY:
        case XPON_ACTIVE_PHY:
            media = parameter->source == XPON_PHY ? xpon_phy(data, instances[2]) : xpon_active_phy(data);
            if (!media)
                return xpon_unsupported(parameter->name, "no uniquely active physical medium");
            return xpon_read_phy(parameter, media, value);
        case XPON_GTC:
            XPON_READ_UINT(&data->gpon.Gtc, CorrectedFecBytes);
            XPON_READ_UINT(&data->gpon.Gtc, CorrectedFecCodeWords);
            XPON_READ_UINT(&data->gpon.Gtc, UnCorrectedFecCodeWords);
            XPON_READ_UINT(&data->gpon.Gtc, TotalFecCodeWords);
            XPON_READ_UINT(&data->gpon.Gtc, HecErrorCount);
            XPON_READ_UINT(&data->gpon.Gtc, PSBdHecErrors);
            XPON_READ_UINT(&data->gpon.Gtc, FrameHecErrors);
            break;
        case XPON_PLOAM:
            if (strcmp(parameter->member, "OnuId") == 0 && data->gpon.Ploam.OnuId > 1022)
                return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
            XPON_READ_UINT(&data->gpon.Ploam, OnuId);
            XPON_READ_UINT(&data->gpon.Ploam, TxMessageCount);
            XPON_READ_UINT(&data->gpon.Ploam, RxMessageCount);
            XPON_READ_UINT(&data->gpon.Ploam, MicErrors);
            XPON_READ_STRING(&data->gpon.Ploam, VendorId);
            if (strcmp(parameter->member, "SerialNumber") == 0)
                return xpon_serial(parameter, &data->gpon.Ploam, value);
            if (strcmp(parameter->member, "RegistrationState") == 0)
                return xpon_enum(value, data->gpon.Ploam.RegistrationState, activation, XPON_COUNT(activation));
            break;
        case XPON_OMCI:
            XPON_READ_UINT(&data->gpon.Omci, RxBaseLineMessageCountValid);
            XPON_READ_UINT(&data->gpon.Omci, RxExtendedMessageCountValid);
            XPON_READ_UINT(&data->gpon.Omci, MicErrors);
            break;
        case XPON_GEM:
            gem = xpon_gem(data, instances[2]);
            if (!gem) return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
            if (strcmp(parameter->member, "PortId") == 0 && gem->PortId > 65534)
                return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
            XPON_READ_UINT(gem, PortId);
            XPON_READ_UINT(gem, TransmittedFrames);
            XPON_READ_UINT(gem, ReceivedFrames);
            if (gem->TrafficType != Bidirectional)
                return xpon_unsupported(parameter->name, "HAL does not specify unidirectional orientation");
            rbusValue_SetString(value, "bidirectional");
            return RBUS_ERROR_SUCCESS;
        case XPON_VEIP:
            veip = xpon_veip(data, instances[1]);
            if (!veip) return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
            XPON_READ_STRING(veip, InterfaceName);
            XPON_READ_STRING(veip, InterDomainName);
            if (strcmp(parameter->member, "AdministrativeState") == 0)
            {
                if (veip->AdministrativeState != Lock && veip->AdministrativeState != Unlock)
                    return RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
                rbusValue_SetBoolean(value, veip->AdministrativeState == Unlock);
            }
            else if (strcmp(parameter->member, "OperationalState") == 0)
                return xpon_enum(value, veip->OperationalState, states, XPON_COUNT(states));
            else if (strcmp(parameter->member, "MeId") == 0)
            {
                char identity[48];
                snprintf(identity, sizeof(identity), "(VEIP,%lu)", (unsigned long)veip->MeId);
                rbusValue_SetString(value, identity);
            }
            else
                break;
            return RBUS_ERROR_SUCCESS;
        default: break;
    }
    return xpon_unsupported(parameter->name, "unimplemented mapping");
}

static rbusError_t xpon_get(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t *options)
{
    const XponParameter *parameter;
    uint32_t instances[3];
    GPON_DML_DATA *data;
    rbusValue_t value = NULL;
    rbusError_t result = RBUS_ERROR_NOT_INITIALIZED;
    (void)handle;
    (void)options;
    if (!property) return RBUS_ERROR_INVALID_INPUT;
    parameter = xpon_find(rbusProperty_GetName(property), instances);
    if (!parameter) return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    pthread_mutex_lock(&g_mutex);
    if (g_ready)
    {
        data = GponMgrDml_GetData_locked();
        if (data)
        {
            rbusValue_Init(&value);
            result = value ? xpon_read_data(parameter, instances, data, value) : RBUS_ERROR_OUT_OF_RESOURCES;
            if (result == RBUS_ERROR_SUCCESS)
                rbusProperty_SetValue(property, value);
            if (value) rbusValue_Release(value);
            GponMgrDml_GetData_release(data);
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return result;
}

static rbusError_t xpon_set(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t *options)
{
    const XponParameter *parameter;
    uint32_t instances[3];
    rbusValue_t value;
    rbusError_t result = XPON_UNSUPPORTED;
    (void)handle;
    if (!property) return RBUS_ERROR_INVALID_INPUT;
    parameter = xpon_find(rbusProperty_GetName(property), instances);
    if (!parameter) return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if (!parameter->writable) return RBUS_ERROR_NOT_WRITABLE;
    value = rbusProperty_GetValue(property);
    if (!value || rbusValue_GetType(value) != parameter->type)
        return RBUS_ERROR_INVALID_PARAMETER_TYPE;
    if (instances[0] != 1 || (strstr(parameter->name, ".ANI.") && instances[1] != 1))
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if (options && (options->sessionId || !options->commit))
        return xpon_unsupported(parameter->name, "transactional writes are unavailable");
    pthread_mutex_lock(&g_mutex);
    if (!g_ready)
        result = RBUS_ERROR_NOT_INITIALIZED;
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    else if (parameter->source == XPON_ACTIVE_PHY)
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
                result = xpon_unsupported(parameter->name, "no uniquely active physical medium");
            if (media)
            {
                char halName[RBUS_MAX_NAME_LENGTH];
                char halValue[65];
                bool alias = strcmp(parameter->member, "Alias") == 0;
                const char *text = alias ? rbusValue_GetString(value, NULL) : NULL;
                if (alias && (!text || !isalpha((unsigned char)text[0]) || strlen(text) >= sizeof(media->Alias)))
                    result = RBUS_ERROR_INVALID_PARAMETER_VALUE;
                else
                {
                    snprintf(halName, sizeof(halName), "Device.X_RDK_ONT.PhysicalMedia.%lu.%s",
                             (unsigned long)media->uInstanceNumber, parameter->member);
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
#endif
    else
        result = xpon_unsupported(parameter->name, "no equivalent writable HAL field in this build");
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
    const XponParameter *parameter;
    uint32_t instances[3];
    (void)handle; (void)filter; (void)interval;
    if (!autoPublish) return RBUS_ERROR_INVALID_INPUT;
    *autoPublish = false;
    if (action == RBUS_EVENT_ACTION_UNSUBSCRIBE) return RBUS_ERROR_SUCCESS;
    parameter = xpon_find(name, instances);
    if (!parameter) return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    if (parameter->source == XPON_NONE)
        return xpon_unsupported(name, "events for an unsupported parameter");
    *autoPublish = true;
    return RBUS_ERROR_SUCCESS;
}

int GponMgrXponRbus_Init(void)
{
    size_t index;
    rbusError_t result;
    rbusHandle_t failedHandle = NULL;
    pthread_mutex_lock(&g_mutex);
    if (g_ready)
    {
        pthread_mutex_unlock(&g_mutex);
        return RBUS_ERROR_SUCCESS;
    }
    result = rbus_open(&g_handle, "GponManager.XPON");
    if (result != RBUS_ERROR_SUCCESS) goto done;
    memset(g_elements, 0, sizeof(g_elements));
    memset(g_fetched, 0, sizeof(g_fetched));
    g_rowCount = 0;
    for (index = 0; index < XPON_COUNT(g_tables); ++index)
    {
        g_elements[index].name = (char *)g_tables[index];
        g_elements[index].type = RBUS_ELEMENT_TYPE_TABLE;
        g_elements[index].cbTable.tableAddRowHandler = xpon_add;
        g_elements[index].cbTable.tableRemoveRowHandler = xpon_remove;
    }
    for (index = 0; index < XPON_COUNT(g_parameters); ++index)
    {
        rbusDataElement_t *element = &g_elements[XPON_COUNT(g_tables) + index];
        element->name = (char *)g_parameters[index].name;
        element->type = RBUS_ELEMENT_TYPE_PROPERTY;
        element->cbTable.getHandler = xpon_get;
        element->cbTable.setHandler = g_parameters[index].writable ? xpon_set : NULL;
        element->cbTable.eventSubHandler = xpon_subscribe;
    }
    result = rbus_regDataElements(g_handle, (int)XPON_COUNT(g_elements), g_elements);
    if (result != RBUS_ERROR_SUCCESS) goto failed;
    for (index = 0; index < XPON_COUNT(g_tables); ++index)
    {
        result = rbus_registerDynamicTableSyncHandler(g_handle, g_tables[index], xpon_sync);
        if (result != RBUS_ERROR_SUCCESS) goto failed;
    }
    result = xpon_sync_locked();
    if (result != RBUS_ERROR_SUCCESS) goto failed;
    g_ready = true;
    CcspTraceInfo(("XPON: registered %zu BBF parameters alongside Device.X_RDK_ONT\n", XPON_COUNT(g_parameters)));
    goto done;
failed:
    failedHandle = g_handle;
    g_handle = NULL;
    g_rowCount = 0;
done:
    pthread_mutex_unlock(&g_mutex);
    if (failedHandle) rbus_close(failedHandle);
    return result;
}

void GponMgrXponRbus_Close(void)
{
    rbusHandle_t handle;
    pthread_mutex_lock(&g_mutex);
    g_ready = false;
    handle = g_handle;
    g_handle = NULL;
    g_rowCount = 0;
    pthread_mutex_unlock(&g_mutex);
    if (handle) rbus_close(handle);
}