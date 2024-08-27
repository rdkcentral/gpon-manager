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
/*****************************************************************************
* STANDARD INCLUDE FILES
*****************************************************************************/
#include <json-c/json.h>
#include <limits.h>
#include <syscfg/syscfg.h>

/*****************************************************************************
* PROJECT-SPECIFIC INCLUDE FILES
*****************************************************************************/
#include "json_hal_client.h"
#include "ansc_platform.h"

#include "gpon_apis.h"
#include "gponmgr_dml_hal.h"
#include "gponmgr_dml_hal_param.h"
#include "gponmgr_link_state_machine.h"

#define HAL_CONNECTION_RETRY_MAX_COUNT 10

#define CHECK(expr)                                                \
    if (!(expr))                                                   \
    {                                                              \
        CcspTraceError(("%s - %d Invalid parameter error \n!!!", __FUNCTION__, __LINE__)); \
        return ANSC_STATUS_FAILURE;                                \
    }

/**
 * gpon_manager_conf.json contains the port number of server as well as schema path.
 * This port number can be used in manager as well as
 * vendor software for rpc communication.
 */
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
#define GPON_MANAGER_CONF_FILE "/etc/rdk/conf/gpon_manager_wan_unify_conf.json"
#else
#define GPON_MANAGER_CONF_FILE "/etc/rdk/conf/gpon_manager_conf.json"
#endif
#define NULL_TYPE 0

#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
#define GPON_QUERY_PM    "Device.X_RDK_ONT.PhysicalMedia.1."
#define GPON_QUERY_GEM   "Device.X_RDK_ONT.GemStats.1."
#define GPON_QUERY_VEIP  "Device.X_RDK_ONT.Veip.1."
#else
#define GPON_QUERY_PM    "Device.X_RDK_ONT.PhysicalMedia."
#define GPON_QUERY_GEM   "Device.X_RDK_ONT.Gem."
#define GPON_QUERY_VEIP  "Device.X_RDK_ONT.Veip."
#endif
#define GPON_QUERY_GTC   "Device.X_RDK_ONT.Gtc."
#define GPON_QUERY_PLOAM "Device.X_RDK_ONT.Ploam."
#define GPON_QUERY_OMCI  "Device.X_RDK_ONT.Omci."
#define GPON_QUERY_TR69  "Device.X_RDK_ONT.TR69."

#define DML_GTC_FETCH_INTERVAL 10


ANSC_STATUS GponHal_get_data(void);



/*******************************************************************************
 *                        LOCAL FUNCTION TYPES
 *******************************************************************************/

/**
 * @brief Utility API to create json request message to send to the interface manager
 *
 * @param request_type (IN)   - Type of request message (GET/SET)
 * @param param_name   (IN)   - String holds the parameter name. This contains the absolute DML path for the parameter.
 * @param type         (IN)   - Type of the parameter needs to be passed to lower layer.
 * @param param_val    (IN)   - String holds the value of the parameter needs to set. This string contains the data based on the type.
 * @return Status of the operation
 * @retval ANSC_STATUS_SUCCESS if successful
 * @retval ANSC_STATUS_FAILURE if any error is detected
 */

/**************************************************************************************************
 * PUBLIC APIS
 **************************************************************************************************/
ANSC_STATUS GponHal_Init()
{
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    if (access("/bin/json_hal_server_gpon", F_OK) == 0)
    {
        v_secure_system("/bin/json_hal_server_gpon  /etc/rdk/conf/gpon_manager_wan_unify_conf.json &");
    }
    else
    {
        v_secure_system("/usr/bin/json_hal_server_gpon  /etc/rdk/conf/gpon_manager_wan_unify_conf.json &");
    }
#endif

    if (json_hal_client_init(GPON_MANAGER_CONF_FILE) != RETURN_OK)
    {
        CcspTraceError(("%s - %d Failed to initialise json hal client library \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if (json_hal_client_run() != RETURN_OK)
    {
        CcspTraceError(("%s - %d Failed to start the json hal client module \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /**
     * Make sure HAL client connected to server.
     * Here waits for 10seconds time to check whether connection established or not.
     * If connection not established within this time, returned error.
     */
    int retry_count = 0;
    int is_client_connected = 0;
    while (retry_count < HAL_CONNECTION_RETRY_MAX_COUNT)
    {
        if (!json_hal_is_client_connected())
        {
            sleep(1);
            retry_count++;
        }
        else
        {
            CcspTraceInfo(("Hal-client connected to the hal server \n"));
            is_client_connected = TRUE;
            break;
        }
    }

    if (is_client_connected != TRUE)
    {
        CcspTraceInfo(("Failed to connect to the hal server. \n"));
        return ANSC_STATUS_FAILURE;
    }
    return ANSC_STATUS_SUCCESS;
}


bool GponHal_check_data_initiliased(void)
{
    bool data_initialised = FALSE;
    int pm_idx = 0;


    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        for(pm_idx = 0; pm_idx < pGponDmlData->gpon.PhysicalMedia.ulQuantity; pm_idx++)
        {
            DML_PHY_MEDIA_CTRL_T* pGponData = pGponDmlData->gpon.PhysicalMedia.pdata[pm_idx];
            if(pGponData!= NULL)
            {
                DML_PHY_MEDIA* pGponPhy = &(pGponData->dml);
                if(strcmp(pGponPhy->ModuleVendor, ""))
                {
                    if(pGponPhy->PonMode == GPON)
                    {
                        data_initialised = TRUE;
                        break;
                    }
                }
            }
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }
    return data_initialised;
}


ANSC_STATUS GponHal_get_init_data(void)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    bool data_initialised = FALSE;
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    int retry_count = 1;
#endif

    while(data_initialised != TRUE)
    {
        retStatus = GponHal_get_data();
        if(retStatus == ANSC_STATUS_SUCCESS)
        {
            data_initialised = GponHal_check_data_initiliased();
        }
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
        //TODO: Sky has partially implemented the data models. So due to this json hal request is failing for many parameters.
        //  GponHal_check_data_initiliased is failing due to that which is causing infinite wait.
        //  So for now we added limit on number of retries and exiting after max retries limit reaches. This need to be removed
        //  once we are able to read all data model using json hal.
        if(retry_count >= 0)
        {
            retry_count--;
        }    
        else
        {
             break;
        }
#endif
    }

    return retStatus;
}


static json_object *create_json_request_message(eActionType request_type, const CHAR *param_name, eParamType type, CHAR *param_val)
{
    json_object *jrequest = NULL;
    hal_param_t stParam;
    memset(&stParam, 0, sizeof(stParam));

    switch (request_type)
    {
        case SET_REQUEST_MESSAGE:
            jrequest = json_hal_client_get_request_header(SET_PARAMETER_METHOD);
            strncpy(stParam.name, param_name, sizeof(stParam.name)-1);
            stParam.type = type;
            switch (type)
            {
                case PARAM_BOOLEAN:
                case PARAM_INTEGER:
                case PARAM_UNSIGNED_INTEGER:
                case PARAM_STRING:
                {
                    strncpy(stParam.value,param_val,sizeof(stParam.value)-1);
                    json_hal_add_param(jrequest, SET_REQUEST_MESSAGE, &stParam);
                    break;
                }
                default:
                {
                    fprintf(stderr,"Invalid input\n");
                    break;
                }
            }
        break;
        case GET_REQUEST_MESSAGE:
            jrequest = json_hal_client_get_request_header(GET_PARAMETER_METHOD);
            strncpy(stParam.name, param_name, sizeof(stParam.name)-1);
            json_hal_add_param(jrequest, GET_REQUEST_MESSAGE, &stParam);
            break;
    }
    return jrequest;
}

ANSC_STATUS GponHal_setParam(char *pName, eParamType pType, char *pValue)
{

    json_object *jreply_msg;
    json_object *jrequest;
    int rc = ANSC_STATUS_FAILURE;
    json_bool status = FALSE;
    jrequest = create_json_request_message(SET_REQUEST_MESSAGE, pName, pType, pValue);
    CHECK(jrequest != NULL);

    if (json_hal_client_send_and_get_reply(jrequest, &jreply_msg) == RETURN_ERR)
    {
        fprintf(stderr,"%s - %d Failed to get reply for the json request \n", __FUNCTION__, __LINE__);
        return rc;
    }

    if (json_hal_get_result_status(jreply_msg, &status) == RETURN_OK)
    {
        if (status)
        {
            rc = ANSC_STATUS_SUCCESS;
        }
        else
        {
            fprintf(stderr,"%s - %d Set request for [%s] is failed\n", __FUNCTION__, __LINE__, pName);
        }
    }
    else
    {
        fprintf(stderr,"%s - %d Failed to get result status from json response, something wrong happened!!! \n", __FUNCTION__, __LINE__);
    }

    if (jrequest)
    {
        json_object_put(jrequest);
        jrequest = NULL;
    }

    if (jreply_msg)
    {
        json_object_put(jreply_msg);
        jreply_msg = NULL;
    }
    return rc;
}



#define FREE_JSON_OBJECT(expr) \
    if(expr)                   \
	{                          \
        json_object_put(expr); \
	}                          \



ANSC_STATUS get_event_param(const char* msg, const int len, char* event_name, char* event_val)
{
    json_object* msg_param = NULL;
    json_object* msg_param_val = NULL;

    if(msg == NULL) {
        return ANSC_STATUS_FAILURE;
    }

    /* Parse message and find event received. */
    json_object *jobj = json_tokener_parse(msg);
    CHECK(jobj);

    if (json_object_object_get_ex(jobj, JSON_RPC_FIELD_PARAMS, &msg_param))
    {
        json_object *jsubs_param_array = json_object_array_get_idx(msg_param, 0);
        if (jsubs_param_array == NULL)
        {
            CcspTraceError(("Failed to get params data from subscription json message \n"));
            FREE_JSON_OBJECT(jobj);
            return ANSC_STATUS_FAILURE;
        }

        if (json_object_object_get_ex(jsubs_param_array, JSON_RPC_FIELD_NAME, &msg_param_val))
        {
            strcpy(event_name, json_object_get_string(msg_param_val));
            CcspTraceInfo(("Event name = %s \n", event_name));
        }
        else
        {
            CcspTraceError(("Failed to get event name data from subscription json message \n"));
            FREE_JSON_OBJECT(jobj);
            return ANSC_STATUS_FAILURE;
        }

        if (json_object_object_get_ex(jsubs_param_array, JSON_RPC_FIELD_VALUE, &msg_param_val))
        {
            strcpy(event_val, json_object_get_string(msg_param_val));
            CcspTraceInfo(("Event value = %s \n", event_val));
        }
        else
        {
            CcspTraceError(("Failed to get event value data from subscription json message \n"));
            FREE_JSON_OBJECT(jobj);
            return ANSC_STATUS_FAILURE;
        }
    }

    FREE_JSON_OBJECT(jobj);

    CcspTraceInfo(("Event got for %s and its value =%s \n", event_name, event_val));

    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS GponHal_SetParamBool(BOOL* SValue, char* HalName, BOOL bValue)
{
    char strValue[8] = {0};
    if (bValue == TRUE)
    {
        strncpy(strValue, JSON_STR_TRUE, strlen(JSON_STR_TRUE)+1);
    }
    else
    {
        strncpy(strValue, JSON_STR_FALSE, strlen(JSON_STR_FALSE)+1);
    }
    if (GponHal_setParam(HalName, PARAM_BOOLEAN, strValue) == ANSC_STATUS_SUCCESS)
    {
        *SValue = bValue;
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

ANSC_STATUS GponHal_SetParamUInt(UINT* SValue, char* HalName, UINT iValue)
{
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};

    snprintf(strValue, JSON_MAX_VAL_ARR_SIZE, "%u", iValue);

    if (GponHal_setParam(HalName, PARAM_UNSIGNED_INTEGER, strValue) == ANSC_STATUS_SUCCESS)
    {
        *SValue = iValue;
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

ANSC_STATUS GponHal_SetParamInt(INT* SValue, char* HalName, INT iValue)
{
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};

    snprintf(strValue, JSON_MAX_VAL_ARR_SIZE, "%d", iValue);

    if (GponHal_setParam(HalName, PARAM_INTEGER, strValue) == ANSC_STATUS_SUCCESS)
    {
        *SValue = iValue;
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

ANSC_STATUS GponHal_SetParamString(char* SValue, char* HalName, char *pString)
{
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};

    snprintf(strValue, JSON_MAX_VAL_ARR_SIZE, "%s", pString);

    if (GponHal_setParam(HalName, PARAM_STRING, strValue) == ANSC_STATUS_SUCCESS)
    {
        AnscCopyString(SValue, pString);
        return ANSC_STATUS_SUCCESS;
    }
    else
    {
        CcspTraceError(("%s %d - Invalid Input\n", __FUNCTION__,__LINE__)); 
        return ANSC_STATUS_FAILURE;
    }
}

ANSC_STATUS GponHal_SetIngressTagged(DML_VEIP_TAGGED_ENUM *SValue, char* HalName, DML_VEIP_TAGGED_ENUM pString)
{
    char strValue[JSON_MAX_VAL_ARR_SIZE] = {0};
    char strValue2[JSON_MAX_VAL_ARR_SIZE] = {0};

    if( pString == veip_Single )
    {
        strcpy(strValue, IEEE_802_1Q_ETHERTYPE_STR_SINGLE);
    }
    else if( pString == veip_Double )
    {
        strcpy(strValue, IEEE_802_1Q_ETHERTYPE_STR_DOUBLE);
    }
    else if ( pString == veip_Untagged )
    {
        strcpy(strValue, IEEE_802_1Q_ETHERTYPE_STR_UNTAGGED);
    }

    if (GponHal_SetParamString(strValue2, HalName, strValue) == ANSC_STATUS_SUCCESS)
    {
        if(strcmp(strValue2, IEEE_802_1Q_ETHERTYPE_STR_SINGLE) == 0)
        {
            *SValue  = veip_Single;
        }
        else if(strcmp(strValue2, IEEE_802_1Q_ETHERTYPE_STR_DOUBLE) == 0)
        {
            *SValue = veip_Double;
        }
        else if (strcmp (strValue2, IEEE_802_1Q_ETHERTYPE_STR_UNTAGGED) == 0)
        {
            *SValue = veip_Untagged;
        }
        return ANSC_STATUS_SUCCESS;
    }
    else
    {
        CcspTraceError(("%s %d - Invalid Input\n", __FUNCTION__,__LINE__));
        return ANSC_STATUS_FAILURE;
    }
}

void eventcb_PhysicalMediaStatus(const char *msg, const int len)
{
    ANSC_STATUS ret = ANSC_STATUS_FAILURE;
    char event_name[256] = {'\0'};
    char event_val[256] = {'\0'};

    ret = get_event_param(msg, len, event_name, event_val);
    if(ret == ANSC_STATUS_SUCCESS)
    {
        if (strstr(event_name, GPON_HAL_PM_EVENT))
        {
            //ret = GponHal_update_data();

            GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
            if(pGponDmlData != NULL)
            {
                DML_PHY_MEDIA_LIST_T* pGponPhyList = &(pGponDmlData->gpon.PhysicalMedia);

                //update data
                ret = Map_hal_dml_pm(pGponPhyList, event_name, event_val);

                //release data
                GponMgrDml_GetData_release(pGponDmlData);
            }
        }
    }

}

void eventcb_PhysicalMediaAlarmsAll(const char *msg, const int len)
{
    ANSC_STATUS ret = ANSC_STATUS_FAILURE;
    char event_name[256] = {'\0'};
    char event_val[256] = {'\0'};

    ret = get_event_param(msg, len, event_name, event_val);
    if(ret == ANSC_STATUS_SUCCESS)
    {
        if (strstr(event_name, GPON_HAL_PM_EVENT))
        {
            //ret = GponHal_update_data();

            GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
            if(pGponDmlData != NULL)
            {
                DML_PHY_MEDIA_LIST_T* pGponPhyList = &(pGponDmlData->gpon.PhysicalMedia);

                //update data
                ret = Map_hal_dml_pm(pGponPhyList, event_name, event_val);

                //release data
                GponMgrDml_GetData_release(pGponDmlData);
            }
        }
    }
}

void eventcb_VeipAdministrativeState(const char *msg, const int len)
{
    ANSC_STATUS ret = ANSC_STATUS_FAILURE;
    char event_name[256] = {'\0'};
    char event_val[256] = {'\0'};
    int hal_index = 0;

    ret = get_event_param(msg, len, event_name, event_val);
    if(ret == ANSC_STATUS_SUCCESS)
    {
        if (strstr(event_name, GPON_HAL_VEIP_EVENT))
        {
            GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
            if(pGponDmlData != NULL)
            {
                DML_VEIP_LIST_T* pGponVeipList = &(pGponDmlData->gpon.Veip);

                //update data
                ret = Map_hal_dml_veip(pGponVeipList, event_name, event_val);

                //check new Veip Interface
                gpon_hal_get_veip_index(event_name, &hal_index);
                if(hal_index > 0)
                {
                    DML_VEIP_CTRL_T* pGponVeipCtrl = pGponDmlData->gpon.Veip.pdata[hal_index-1];
                    if(pGponVeipCtrl!= NULL)
                    {
                        if(pGponVeipCtrl->sm_created == FALSE)
                        {
                            DML_VEIP* pGponVeip = &(pGponVeipCtrl->dml);
                            if(pGponVeip->AdministrativeState == Unlock)
                            {
                                //start SM thread
                                ret = GponMgr_Link_StateMachine_Start(pGponVeip);
                                if(ret == ANSC_STATUS_SUCCESS)
                                {
                                    pGponVeipCtrl->sm_created = TRUE;
                                }
                            }
                        }
                    }
                }

                //release data
                GponMgrDml_GetData_release(pGponDmlData);
            }

        }
    }
}

void eventcb_VeipOperationalState(const char *msg, const int len)
{
    ANSC_STATUS ret = ANSC_STATUS_FAILURE;
    char event_name[256] = {'\0'};
    char event_val[256] = {'\0'};
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    int hal_index = 0;
#endif

    ret = get_event_param(msg, len, event_name, event_val);
    if(ret == ANSC_STATUS_SUCCESS)
    {
        if (strstr(event_name, GPON_HAL_VEIP_EVENT))
        {
            //ret = GponHal_update_data();

            GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
            if(pGponDmlData != NULL)
            {
                DML_VEIP_LIST_T* pGponVeipList = &(pGponDmlData->gpon.Veip);

                //update data
                ret = Map_hal_dml_veip(pGponVeipList, event_name, event_val);

#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
                /*TODO : Need revisit based on outcome of CS00012203760 
		 Change added to check the veip adminstate whenever we receive the operstate
                 callback and if its "Unlock" then start the state machine.*/
                gpon_hal_get_veip_index(event_name, &hal_index);
                if(hal_index > 0)
                {
                    DML_VEIP_CTRL_T* pGponVeipCtrl = pGponDmlData->gpon.Veip.pdata[hal_index-1];

                    if(pGponVeipCtrl!= NULL)
                    {
                        if(pGponVeipCtrl->sm_created == FALSE)
                        {
                            DML_VEIP* pGponVeip = &(pGponVeipCtrl->dml);
                            if(pGponVeip->AdministrativeState == Unlock)
                            {
                                //start SM thread
                                ret = GponMgr_Link_StateMachine_Start(pGponVeip);

                                if(ret == ANSC_STATUS_SUCCESS)
                                {
                                    pGponVeipCtrl->sm_created = TRUE;
                                }
                            }
                        }
                    }
                }
#endif
                //release data
                GponMgrDml_GetData_release(pGponDmlData);
            }
        }
    }
}
void eventcb_PloamRegistrationState (const char *msg, const int len)
{
    ANSC_STATUS ret = ANSC_STATUS_FAILURE;
    char event_name[256] = {'\0'};
    char event_val[256] = {'\0'};

    ret = get_event_param(msg, len, event_name, event_val);
    if(ret == ANSC_STATUS_SUCCESS)
    {
        if (strstr(event_name, GPON_HAL_PLOAM_EVENT))
        {
            //ret = GponHal_update_data();

            GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
            if(pGponDmlData != NULL)
            {
                DML_PLOAM* pGponPloam = &(pGponDmlData->gpon.Ploam);

                //update data
                ret = Map_hal_dml_ploam(pGponPloam, event_name, event_val);

                //release data
                GponMgrDml_GetData_release(pGponDmlData);
            }
        }
    }
}

ANSC_STATUS GponHal_get_tr69(PDML_TR69 pTr69)
{

    ANSC_STATUS rc = ANSC_STATUS_SUCCESS;

    if (pTr69 == NULL)
    {
        fprintf(stderr,"%s - %d Invalid argument \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }

    if ((AnscGetTickInSeconds() - DML_GTC_FETCH_INTERVAL) < pTr69->LastFetchedTime)
    {
        /* Last HAL Get call was less than DML_GTC_FETCH_INTERVAL sec pGtc already has recent data */
        return rc;
    }

    int total_param_count = 0;
    hal_param_t resp_param;
    json_object *jreply_msg = NULL;

    json_object *jrequest = create_json_request_message(GET_REQUEST_MESSAGE, GPON_QUERY_TR69, NULL_TYPE , NULL);

    CHECK(jrequest != NULL);

    if (json_hal_client_send_and_get_reply(jrequest, &jreply_msg) == RETURN_ERR)
    {
        fprintf(stderr,"%s - %d Failed to get reply for the json request \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }

    total_param_count = json_hal_get_total_param_count(jreply_msg);

    for (int i = 0; i < total_param_count; ++i)
    {
        if (json_hal_get_param(jreply_msg, i, GET_RESPONSE_MESSAGE, &resp_param) != RETURN_OK)
        {
            fprintf(stderr,"%s - %d Failed to get the param from response message [index = %d] \n", __FUNCTION__, __LINE__, i);
            continue;
        }
        Map_hal_dml_tr69(pTr69,resp_param.name,resp_param.value);
    }
    if (rc != ANSC_STATUS_SUCCESS)
    {
        fprintf(stderr,"%s - %d Failed to get statistics data  \n", __FUNCTION__, __LINE__);
    }

    /* Update Last Fetch Time in DML Structure */
    pTr69->LastFetchedTime = AnscGetTickInSeconds();

    // Free json objects.
    if (jrequest)
    {
        json_object_put(jrequest);
        jrequest = NULL;
    }

    if (jreply_msg)
    {
        json_object_put(jreply_msg);
        jreply_msg = NULL;
    }
    return rc;
}

ANSC_STATUS GponHal_get_omci(PDML_OMCI pOmci)
{
    ANSC_STATUS rc = ANSC_STATUS_SUCCESS;

    if (pOmci == NULL)
    {
        fprintf(stderr,"%s - %d Invalid argument \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }

    if ((AnscGetTickInSeconds() - DML_GTC_FETCH_INTERVAL) < pOmci->LastFetchedTime)
    {
        /* Last HAL Get call was less than DML_GTC_FETCH_INTERVAL sec pGtc already has recent data */
        return rc;
    }
    int total_param_count = 0;
    hal_param_t resp_param;
    json_object *jreply_msg = NULL;

    json_object *jrequest = create_json_request_message(GET_REQUEST_MESSAGE, GPON_QUERY_OMCI, NULL_TYPE , NULL);

    CHECK(jrequest != NULL);

    if (json_hal_client_send_and_get_reply(jrequest, &jreply_msg) == RETURN_ERR)
    {
        fprintf(stderr,"%s - %d Failed to get reply for the json request \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }

    total_param_count = json_hal_get_total_param_count(jreply_msg);


    for (int i = 0; i < total_param_count; ++i)
    {
        if (json_hal_get_param(jreply_msg, i, GET_RESPONSE_MESSAGE, &resp_param) != RETURN_OK)
        {
            fprintf(stderr,"%s - %d Failed to get the param from response message [index = %d] \n", __FUNCTION__, __LINE__, i);
            continue;
        }
        Map_hal_dml_omci(pOmci,resp_param.name,resp_param.value);
    }
    if (rc != ANSC_STATUS_SUCCESS)
    {
        fprintf(stderr,"%s - %d Failed to get statistics data  \n", __FUNCTION__, __LINE__);
    }

    /* Update Last Fetch Time in DML Structure */
    pOmci->LastFetchedTime = AnscGetTickInSeconds();

    // Free json objects.
    if (jrequest)
    {
        json_object_put(jrequest);
        jrequest = NULL;
    }

    if (jreply_msg)
    {
        json_object_put(jreply_msg);
        jreply_msg = NULL;
    }
    return rc;
}

ANSC_STATUS GponHal_get_ploam(PDML_PLOAM pPloam)
{
    ANSC_STATUS rc = ANSC_STATUS_SUCCESS;

    if (pPloam == NULL)
    {
        fprintf(stderr,"%s - %d Invalid argument \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }

    if ((AnscGetTickInSeconds() - DML_GTC_FETCH_INTERVAL) < pPloam->LastFetchedTime)
    {
        /* Last HAL Get call was less than DML_GTC_FETCH_INTERVAL sec pPloam already has recent data */
        return rc;
    }

    int total_param_count = 0;
    hal_param_t resp_param;
    json_object *jreply_msg = NULL;

    json_object *jrequest = create_json_request_message(GET_REQUEST_MESSAGE, GPON_QUERY_PLOAM, NULL_TYPE , NULL);

    CHECK(jrequest != NULL);

    if (json_hal_client_send_and_get_reply(jrequest, &jreply_msg) == RETURN_ERR)
    {
        fprintf(stderr,"%s - %d Failed to get reply for the json request \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }

    total_param_count = json_hal_get_total_param_count(jreply_msg);

    for (int i = 0; i < total_param_count; ++i)
    {
        if (json_hal_get_param(jreply_msg, i, GET_RESPONSE_MESSAGE, &resp_param) != RETURN_OK)
        {
            fprintf(stderr,"%s - %d Failed to get the param from response message [index = %d] \n", __FUNCTION__, __LINE__, i);
            continue;
        }
        Map_hal_dml_ploam(pPloam,resp_param.name,resp_param.value);
    }
    if (rc != ANSC_STATUS_SUCCESS)
    {
        fprintf(stderr,"%s - %d Failed to get statistics data  \n", __FUNCTION__, __LINE__);
    }

    /* Update Last Fetch Time in DML Structure */
    pPloam->LastFetchedTime = AnscGetTickInSeconds();

    // Free json objects.
    if (jrequest)
    {
        json_object_put(jrequest);
        jrequest = NULL;
    }

    if (jreply_msg)
    {
        json_object_put(jreply_msg);
        jreply_msg = NULL;
    }
    return rc;
}



ANSC_STATUS GponHal_get_gtc(PDML_GTC pGtc)
{
    ANSC_STATUS rc = ANSC_STATUS_SUCCESS;

    if (pGtc == NULL)
    {
        fprintf(stderr,"%s - %d Invalid argument \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }

    if ((AnscGetTickInSeconds() - DML_GTC_FETCH_INTERVAL) < pGtc->LastFetchedTime)
    {
        /* Last HAL Get call was less than DML_GTC_FETCH_INTERVAL sec pGtc already has recent data */
        return rc;
    }

    int total_param_count = 0;
    hal_param_t resp_param;
    json_object *jreply_msg = NULL;

    json_object *jrequest = create_json_request_message(GET_REQUEST_MESSAGE, GPON_QUERY_GTC, NULL_TYPE , NULL);

    CHECK(jrequest != NULL);

    if (json_hal_client_send_and_get_reply(jrequest, &jreply_msg) == RETURN_ERR)
    {
        fprintf(stderr,"%s - %d Failed to get reply for the json request \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }

    total_param_count = json_hal_get_total_param_count(jreply_msg);

    for (int i = 0; i < total_param_count; ++i)
    {
        if (json_hal_get_param(jreply_msg, i, GET_RESPONSE_MESSAGE, &resp_param) != RETURN_OK)
        {
            fprintf(stderr,"%s - %d Failed to get the param from response message [index = %d] \n", __FUNCTION__, __LINE__, i);
            continue;
        }
        Map_hal_dml_gtc(pGtc,resp_param.name,resp_param.value);
    }
    if (rc != ANSC_STATUS_SUCCESS)
    {
        fprintf(stderr,"%s - %d Failed to get statistics data  \n", __FUNCTION__, __LINE__);
    }

    /* Update Last Fetch Time in DML Structure */
    pGtc->LastFetchedTime = AnscGetTickInSeconds();

    // Free json objects.
    if (jrequest)
    {
        json_object_put(jrequest);
        jrequest = NULL;
    }

    if (jreply_msg)
    {
        json_object_put(jreply_msg);
        jreply_msg = NULL;
    }
    return rc;
}

ANSC_STATUS GponHal_get_pm(DML_PHY_MEDIA_LIST_T* pPhysicalMediaList)
{
    ANSC_STATUS rc = ANSC_STATUS_SUCCESS;
    int total_param_count = 0;
    hal_param_t resp_param;
    json_object *jreply_msg = NULL;


    json_object *jrequest = create_json_request_message(GET_REQUEST_MESSAGE, GPON_QUERY_PM, NULL_TYPE , NULL);

    CHECK(jrequest != NULL);

    if (json_hal_client_send_and_get_reply(jrequest, &jreply_msg) == RETURN_ERR)
    {
        fprintf(stderr,"%s - %d Failed to get reply for the json request \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }


    //mark list as outdated
    for (int i = 0; i < pPhysicalMediaList->ulQuantity; ++i)
    {
        if(pPhysicalMediaList->pdata[i] != NULL)
        {
            pPhysicalMediaList->pdata[i]->updated = false;
        }
    }

    //check all messages
    total_param_count = json_hal_get_total_param_count(jreply_msg);
    for (int i = 0; i < total_param_count; ++i)
    {
        if (json_hal_get_param(jreply_msg, i, GET_RESPONSE_MESSAGE, &resp_param) != RETURN_OK)
        {
            fprintf(stderr,"%s - %d Failed to get the param from response message [index = %d] \n", __FUNCTION__, __LINE__, i);
            continue;
        }

        Map_hal_dml_pm(pPhysicalMediaList, resp_param.name, resp_param.value);
    }


    //remove outdated
    for (int i = 0; i < pPhysicalMediaList->ulQuantity; ++i)
    {
        if(pPhysicalMediaList->pdata[i] != NULL)
        {
            if(pPhysicalMediaList->pdata[i]->updated == false)
            {
                AnscFreeMemory((ANSC_HANDLE) pPhysicalMediaList->pdata[i]);
                pPhysicalMediaList->pdata[i] = NULL;
                pPhysicalMediaList->ulQuantity--;
            }
        }
    }


    if (rc != ANSC_STATUS_SUCCESS)
    {
        fprintf(stderr,"%s - %d Failed to get statistics data  \n", __FUNCTION__, __LINE__);
    }

    if (jrequest)
    {
        json_object_put(jrequest);
        jrequest = NULL;
    }

    if (jreply_msg)
    {
        json_object_put(jreply_msg);
        jreply_msg = NULL;
    }
    return rc;
}


ANSC_STATUS GponHal_get_gem(DML_GEM_LIST_T* pGponGemList)
{
    ANSC_STATUS rc = ANSC_STATUS_SUCCESS;
    int total_param_count = 0;
    hal_param_t resp_param;
    json_object *jreply_msg = NULL;


    json_object *jrequest = create_json_request_message(GET_REQUEST_MESSAGE, GPON_QUERY_GEM, NULL_TYPE , NULL);

    CHECK(jrequest != NULL);

    if (json_hal_client_send_and_get_reply(jrequest, &jreply_msg) == RETURN_ERR)
    {
        fprintf(stderr,"%s - %d Failed to get reply for the json request \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }


    //mark list as outdated
    for (int i = 0; i < pGponGemList->ulQuantity; ++i)
    {
        if(pGponGemList->pdata[i] != NULL)
        {
            pGponGemList->pdata[i]->updated = false;
        }
    }

    //check all messages
    total_param_count = json_hal_get_total_param_count(jreply_msg);
    for (int i = 0; i < total_param_count; ++i)
    {
        if (json_hal_get_param(jreply_msg, i, GET_RESPONSE_MESSAGE, &resp_param) != RETURN_OK)
        {
            fprintf(stderr,"%s - %d Failed to get the param from response message [index = %d] \n", __FUNCTION__, __LINE__, i);
            continue;
        }

        Map_hal_dml_gem(pGponGemList, resp_param.name, resp_param.value);
    }


    //remove outdated
    for (int i = 0; i < pGponGemList->ulQuantity; ++i)
    {
        if(pGponGemList->pdata[i] != NULL)
        {
            if(pGponGemList->pdata[i]->updated == false)
            {
                AnscFreeMemory((ANSC_HANDLE) pGponGemList->pdata[i]);
                pGponGemList->pdata[i] = NULL;
                pGponGemList->ulQuantity--;
            }
        }
    }


    if (rc != ANSC_STATUS_SUCCESS)
    {
        fprintf(stderr,"%s - %d Failed to get statistics data  \n", __FUNCTION__, __LINE__);
    }

    if (jrequest)
    {
        json_object_put(jrequest);
        jrequest = NULL;
    }

    if (jreply_msg)
    {
        json_object_put(jreply_msg);
        jreply_msg = NULL;
    }
    return rc;
}


ANSC_STATUS GponHal_get_veip(DML_VEIP_LIST_T* pGponVeipList)
{
    ANSC_STATUS rc = ANSC_STATUS_SUCCESS;
    int total_param_count = 0;
    hal_param_t resp_param;
    json_object *jreply_msg = NULL;


    json_object *jrequest = create_json_request_message(GET_REQUEST_MESSAGE, GPON_QUERY_VEIP, NULL_TYPE , NULL);

    CHECK(jrequest != NULL);

    if (json_hal_client_send_and_get_reply(jrequest, &jreply_msg) == RETURN_ERR)
    {
        fprintf(stderr,"%s - %d Failed to get reply for the json request \n", __FUNCTION__, __LINE__);
        return ANSC_STATUS_FAILURE;
    }


    //mark list as outdated
    for (int i = 0; i < pGponVeipList->ulQuantity; ++i)
    {
        if(pGponVeipList->pdata[i] != NULL)
        {
            pGponVeipList->pdata[i]->updated = false;
        }
    }

    //check all messages
    total_param_count = json_hal_get_total_param_count(jreply_msg);
    for (int i = 0; i < total_param_count; ++i)
    {
        if (json_hal_get_param(jreply_msg, i, GET_RESPONSE_MESSAGE, &resp_param) != RETURN_OK)
        {
            fprintf(stderr,"%s - %d Failed to get the param from response message [index = %d] \n", __FUNCTION__, __LINE__, i);
            continue;
        }

        Map_hal_dml_veip(pGponVeipList, resp_param.name, resp_param.value);
    }


    //remove outdated
    for (int i = 0; i < pGponVeipList->ulQuantity; ++i)
    {
        if(pGponVeipList->pdata[i] != NULL)
        {
            if(pGponVeipList->pdata[i]->updated == false)
            {
                AnscFreeMemory((ANSC_HANDLE) pGponVeipList->pdata[i]);
                pGponVeipList->pdata[i] = NULL;
                pGponVeipList->ulQuantity--;
            }
        }
    }


    if (rc != ANSC_STATUS_SUCCESS)
    {
        fprintf(stderr,"%s - %d Failed to get statistics data  \n", __FUNCTION__, __LINE__);
    }

    if (jrequest)
    {
        json_object_put(jrequest);
        jrequest = NULL;
    }

    if (jreply_msg)
    {
        json_object_put(jreply_msg);
        jreply_msg = NULL;
    }
    return rc;
}


ANSC_STATUS GponHal_get_data(void)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;


    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        retStatus = ANSC_STATUS_SUCCESS;

        ret = GponHal_get_pm(&(pGponDmlData->gpon.PhysicalMedia));
        if(ret != ANSC_STATUS_SUCCESS)
        {
            retStatus = ANSC_STATUS_FAILURE;
        }

        ret = GponHal_get_gtc(&(pGponDmlData->gpon.Gtc));
        if(ret != ANSC_STATUS_SUCCESS)
        {
            retStatus = ANSC_STATUS_FAILURE;
        }

        ret = GponHal_get_ploam(&(pGponDmlData->gpon.Ploam));
        if(ret != ANSC_STATUS_SUCCESS)
        {
            retStatus = ANSC_STATUS_FAILURE;
        }

        ret = GponHal_get_omci(&(pGponDmlData->gpon.Omci));
        if(ret != ANSC_STATUS_SUCCESS)
        {
            retStatus = ANSC_STATUS_FAILURE;
        }

        ret = GponHal_get_gem(&(pGponDmlData->gpon.Gem));
        if(ret != ANSC_STATUS_SUCCESS)
        {
            retStatus = ANSC_STATUS_FAILURE;
        }

        ret = GponHal_get_veip(&(pGponDmlData->gpon.Veip));
        if(ret != ANSC_STATUS_SUCCESS)
        {
            retStatus = ANSC_STATUS_FAILURE;
        }

        ret = GponHal_get_tr69(&(pGponDmlData->gpon.Tr69));
        if(ret != ANSC_STATUS_SUCCESS)
        {
            retStatus = ANSC_STATUS_FAILURE;
        }


        GponMgrDml_GetData_release(pGponDmlData);
    }


    return retStatus;
}




ANSC_STATUS GponHal_send_config(void)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    char req_param_str[100];

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_PHY_MEDIA_LIST_T* pGponPhyMediaList = &(pGponDmlData->gpon.PhysicalMedia);
        DML_VEIP_LIST_T* pGponVeipList = &(pGponDmlData->gpon.Veip);

        //PM Thresholds
        for (int idx = 0; idx < pGponPhyMediaList->ulQuantity; ++idx)
        {
            if(pGponPhyMediaList->pdata[idx] != NULL)
            {
                DML_PHY_MEDIA* pPhyMedia = &(pGponPhyMediaList->pdata[idx]->dml);

                sprintf(req_param_str, "Device.X_RDK_ONT.PhysicalMedia.%ld.RxPower.SignalLevelLowerThreshold", pPhyMedia->uInstanceNumber);
                retStatus = GponHal_SetParamInt(&(pPhyMedia->RxPower.SignalLevelLowerThreshold), req_param_str, pPhyMedia->RxPower.SignalLevelLowerThreshold);
                if(retStatus != ANSC_STATUS_SUCCESS) break;

                sprintf(req_param_str, "Device.X_RDK_ONT.PhysicalMedia.%ld.RxPower.SignalLevelUpperThreshold", pPhyMedia->uInstanceNumber);
                retStatus = GponHal_SetParamInt(&(pPhyMedia->RxPower.SignalLevelUpperThreshold), req_param_str, pPhyMedia->RxPower.SignalLevelUpperThreshold);
                if(retStatus != ANSC_STATUS_SUCCESS) break;

                sprintf(req_param_str, "Device.X_RDK_ONT.PhysicalMedia.%ld.TxPower.SignalLevelLowerThreshold", pPhyMedia->uInstanceNumber);
                retStatus = GponHal_SetParamInt(&(pPhyMedia->TxPower.SignalLevelLowerThreshold), req_param_str, pPhyMedia->TxPower.SignalLevelLowerThreshold);
                if(retStatus != ANSC_STATUS_SUCCESS) break;

                sprintf(req_param_str, "Device.X_RDK_ONT.PhysicalMedia.%ld.TxPower.SignalLevelUpperThreshold", pPhyMedia->uInstanceNumber);
                retStatus = GponHal_SetParamInt(&(pPhyMedia->TxPower.SignalLevelUpperThreshold), req_param_str, pPhyMedia->TxPower.SignalLevelUpperThreshold);
                if(retStatus != ANSC_STATUS_SUCCESS) break;

            }
        }

        //Veip
        if(retStatus == ANSC_STATUS_SUCCESS)
        {
            for (int idx = 0; idx < pGponVeipList->ulQuantity; ++idx)
            {
                if(pGponVeipList->pdata[idx] != NULL)
                {
                    DML_VEIP* pVeip = &(pGponVeipList->pdata[idx]->dml);

                    sprintf(req_param_str, "Device.X_RDK_ONT.Veip.%ld.EthernetFlow.Ingress.Tagged", pVeip->uInstanceNumber);
                    retStatus = GponHal_SetIngressTagged(&(pVeip->EthernetFlow.Ingress.Tagged),req_param_str,pVeip->EthernetFlow.Ingress.Tagged);
                    if(retStatus != ANSC_STATUS_SUCCESS) break;

                    sprintf(req_param_str, "Device.X_RDK_ONT.Veip.%ld.EthernetFlow.Ingress.Q-VLAN.Vid", pVeip->uInstanceNumber);
                    retStatus = GponHal_SetParamUInt(&(pVeip->EthernetFlow.Ingress.QVLAN.Vid), req_param_str, pVeip->EthernetFlow.Ingress.QVLAN.Vid);
                    if(retStatus != ANSC_STATUS_SUCCESS) break;
                }
            }
        }
    }

    GponMgrDml_GetData_release(pGponDmlData);

    return retStatus;
}


ANSC_STATUS GponHal_Event_Subscribe(event_callback callback, const char* event_name, const char* event_notification_type)
{
    if(json_hal_client_subscribe_event(callback, event_name, event_notification_type) == 0)
    {
        return ANSC_STATUS_SUCCESS;
    }
    return ANSC_STATUS_FAILURE;
}

