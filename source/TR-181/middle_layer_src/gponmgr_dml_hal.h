/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
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

#ifndef __GPONMGR_DML_HAL_H__
#define __GPONMGR_DML_HAL_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_hal_common.h"
#include "json_hal_client.h"
#include "secure_wrapper.h"

#include "gpon_apis.h"
#include "gponmgr_dml_obj.h"


/**
 * Standard supported functions.
 */
#define GET_PARAMETER_METHOD "getParameters"
#define SET_PARAMETER_METHOD "setParameters"

#define JSON_RPC_PARAM_ARR_INDEX            0
#define JSON_RPC_FIELD_PARAMS               "params"
#define JSON_RPC_FIELD_NAME                 "name"
#define JSON_RPC_FIELD_VALUE                "value"
#define JSON_SUBSCRIBE_ON_CHANGE            "onChange"

#define JSON_STR_TRUE                       "true"
#define JSON_STR_FALSE                      "false"

#define JSON_MAX_VAL_ARR_SIZE               32
#define JSON_MAX_STR_ARR_SIZE               256


#define GPON_HAL_PM_STATUS                  "Device.X_RDK_ONT.PhysicalMedia.%ld.Status"
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
#define GPON_HAL_PM_ENABLE                      "Device.X_RDK_ONT.PhysicalMedia.%ld.Enable"
#define GPON_HAL_PM_ALIAS                   	"Device.X_RDK_ONT.PhysicalMedia.%ld.Alias"
#define GPON_HAL_PM_LOWERLAYERS             	"Device.X_RDK_ONT.PhysicalMedia.%ld.LowerLayers"
#define GPON_HAL_PM_UPSTREAM			        "Device.X_RDK_ONT.PhysicalMedia.%ld.Upstream"
#endif
#define GPON_HAL_PM_RX_POWER_LOWER_THR          "Device.X_RDK_ONT.PhysicalMedia.%ld.RxPower.SignalLevelLowerThreshold"
#define GPON_HAL_PM_RX_POWER_UPPER_THR          "Device.X_RDK_ONT.PhysicalMedia.%ld.RxPower.SignalLevelUpperThreshold"
#define GPON_HAL_PM_TX_POWER_LOWER_THR          "Device.X_RDK_ONT.PhysicalMedia.%ld.TxPower.SignalLevelLowerThreshold"
#define GPON_HAL_PM_TX_POWER_UPPER_THR          "Device.X_RDK_ONT.PhysicalMedia.%ld.TxPower.SignalLevelUpperThreshold"
#define GPON_HAL_PM_ALARM                       "Device.X_RDK_ONT.PhysicalMedia.%ld.Alarm"

#define GPON_HAL_GEM_ETH_INGRESS_CVLAN_VID      "Device.X_RDK_ONT.Gem.%ld.EthernetFlow.Ingress.C-VLAN.Vid"
#define GPON_HAL_GEM_ETH_INGRESS_CVLAN_PCP      "Device.X_RDK_ONT.Gem.%ld.EthernetFlow.Ingress.C-VLAN.Pcp"
#define GPON_HAL_GEM_ETH_INGRESS_CVLAN_DEI      "Device.X_RDK_ONT.Gem.%ld.EthernetFlow.Ingress.C-VLAN.Dei"


#define GPON_HAL_VEIP_NODE                      "Device.X_RDK_ONT.Veip.%d"
#define GPON_HAL_VEIP_ADMIN_STATE               "Device.X_RDK_ONT.Veip.%ld.AdministrativeState"
#define GPON_HAL_VEIP_OPER_STATE                "Device.X_RDK_ONT.Veip.%ld.OperationalState"
#define GPON_HAL_VEIP_ETH_EGRESS_TAGGED        "Device.X_RDK_ONT.Veip.%ld.EthernetFlow.Egress.Tagged"
#define GPON_HAL_VEIP_ETH_EGRESS_QVLAN_VID     "Device.X_RDK_ONT.Veip.%ld.EthernetFlow.Egress.Q-VLAN.Vid"
#define GPON_HAL_VEIP_ETH_INGRESS_TAGGED        "Device.X_RDK_ONT.Veip.%ld.EthernetFlow.Ingress.Tagged"
#define GPON_HAL_VEIP_ETH_INGRESS_QVLAN_VID     "Device.X_RDK_ONT.Veip.%ld.EthernetFlow.Ingress.Q-VLAN.Vid"
#define GPON_HAL_VEIP_ETH_INGRESS_QVLAN_PCP     "Device.X_RDK_ONT.Veip.%ld.EthernetFlow.Ingress.Q-VLAN.Pcp"
#define GPON_HAL_VEIP_ETH_INGRESS_QVLAN_DEI     "Device.X_RDK_ONT.Veip.%ld.EthernetFlow.Ingress.Q-VLAN.Dei"

#define GPON_HAL_PLOAM_REG_STATE               "Device.X_RDK_ONT.Ploam.RegistrationState"

#define GPON_HAL_PM_EVENT                       "Device.X_RDK_ONT.PhysicalMedia."
#define GPON_HAL_VEIP_EVENT                     "Device.X_RDK_ONT.Veip."
#define GPON_HAL_PLOAM_EVENT                    "Device.X_RDK_ONT.Ploam."

/**
 * @brief Initialise the gpon features
 *
 * @return Status of the operation
 * @retval ANSC_STATUS_SUCCESS if successful
 * @retval ANSC_STATUS_FAILURE if any error is detected
 */
ANSC_STATUS GponHal_Init(void);

//ANSC_STATUS GponHal_get_data(void);
ANSC_STATUS GponHal_get_init_data(void);
ANSC_STATUS GponHal_send_config(void);
ANSC_STATUS GponHal_Event_Subscribe(event_callback callback, const char* event_name, const char* event_notification_type);


ANSC_STATUS GponHal_setParam(char *pName, eParamType pType, char *pValue);


ANSC_STATUS GponHal_get_pm(DML_PHY_MEDIA_LIST_T* pPhysicalMediaList);
ANSC_STATUS GponHal_get_gtc(PDML_GTC pGtc);
ANSC_STATUS GponHal_get_ploam(PDML_PLOAM pPloam);
ANSC_STATUS GponHal_get_omci(PDML_OMCI pOmci);
ANSC_STATUS GponHal_get_gem(DML_GEM_LIST_T* pGponGemList);
ANSC_STATUS GponHal_get_veip(DML_VEIP_LIST_T* pGponVeipList);
ANSC_STATUS GponHal_get_tr69(PDML_TR69 pTr69);

//events cb
void eventcb_PhysicalMediaStatus(const char *msg, const int len);
void eventcb_PhysicalMediaAlarmsAll(const char *msg, const int len);
void eventcb_VeipAdministrativeState(const char *msg, const int len);
void eventcb_VeipOperationalState(const char *msg, const int len);
void eventcb_PloamRegistrationState(const char *msg, const int len);


#endif
