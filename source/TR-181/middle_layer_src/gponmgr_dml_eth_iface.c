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
#include "gponmgr_dml_eth_iface.h"
#include "gponmgr_dml_obj.h"

#define MAX_STR_ARR_SIZE 256
#define STR_TRUE "true"
#define STR_FALSE "false"

static ANSC_STATUS Gponmgr_eth_getParams(char *pComponent, char *pBus, char *pParamName, char *pReturnVal)
{
    parameterValStruct_t **retVal;
    char *ParamName[1];
    int ret = 0,
        nval;

    //Assign address for get parameter name
    ParamName[0] = pParamName;

    ret = CcspBaseIf_getParameterValues(
        bus_handle,
        pComponent,
        pBus,
        ParamName,
        1,
        &nval,
        &retVal);

    //Copy the value
    if (CCSP_SUCCESS == ret)
    {
        CcspTraceWarning(("%s parameterValue[%s]\n", __FUNCTION__, retVal[0]->parameterValue));

        if (NULL != retVal[0]->parameterValue)
        {
            memcpy(pReturnVal, retVal[0]->parameterValue, strlen(retVal[0]->parameterValue) + 1);
        }

        if (retVal)
        {
            free_parameterValStruct_t(bus_handle, nval, retVal);
        }

        return ANSC_STATUS_SUCCESS;
    }

    if (retVal)
    {
        free_parameterValStruct_t(bus_handle, nval, retVal);
    }

    return ANSC_STATUS_FAILURE;
}

#if !defined(WAN_MANAGER_UNIFICATION_ENABLED)
static ANSC_STATUS isGponExistsInEthAgent(char *pInterface, int  *iEthVeipIndex)
{
    char acTmpReturnValue[256] = {0};
    INT iLoopCount;
    INT iTotalNoofEntries;
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;

    if(ANSC_STATUS_FAILURE == Gponmgr_eth_getParams(ETH_MGR_COMPONENT_NAME, ETH_MGR_DBUS_PATH, ETH_MGR_NO_OF_IFACE,acTmpReturnValue))
    {
        CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    //Total count
    iTotalNoofEntries = strtol(acTmpReturnValue, NULL, 10);
    CcspTraceInfo(("%s %d - TotalNoofEntries:%d\n", __FUNCTION__, __LINE__, iTotalNoofEntries));

    if (0 >= iTotalNoofEntries) 
    {
        return ANSC_STATUS_FAILURE;
    }

    //Traverse from loop
    for (iLoopCount = 0; iLoopCount < iTotalNoofEntries; iLoopCount++)
    {
        char acTmpQueryParam[256] = {0};
        //Query
        snprintf(acTmpQueryParam, sizeof(acTmpQueryParam), ETH_MGR_IFACE_NAME, iLoopCount + 1);

        memset(acTmpReturnValue, 0, sizeof(acTmpReturnValue));
        if(ANSC_STATUS_FAILURE == Gponmgr_eth_getParams(ETH_MGR_COMPONENT_NAME, ETH_MGR_DBUS_PATH,acTmpQueryParam,acTmpReturnValue))
        {
            CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
            continue;
        }

        if (NULL != strstr(acTmpReturnValue, pInterface)) 
        {
            *iEthVeipIndex = iLoopCount + 1;
            retStatus = ANSC_STATUS_SUCCESS;
            break;
        }
    }

    return retStatus;
}
#endif

ANSC_STATUS Gponmgr_eth_setParams( char *pComponent, char *pBus, char *pParamName,
                                          char *pParamVal, enum dataType_e type,
                                          unsigned int bCommitFlag)
{
    CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    parameterValStruct_t param_val[1] = {0};
    char *faultParam = NULL;
    int ret = 0;

    if( (pComponent == NULL) || (pBus == NULL) || (pParamName == NULL) || (pParamVal == NULL))
    {
        CcspTraceError(("%s %d - Invalid Input\n", __FUNCTION__,__LINE__));
        return ANSC_STATUS_FAILURE;

    }

    param_val[0].parameterName = pParamName;

    param_val[0].parameterValue = pParamVal;

    param_val[0].type = type;

    ret = CcspBaseIf_setParameterValues(
        bus_handle,
        pComponent,
        pBus,
        0,
        0,
        param_val,
        1,
        bCommitFlag,
        &faultParam);

    if ((ret != CCSP_SUCCESS) && (faultParam != NULL))
    {
        CcspTraceError(("%s-%d Failed to set %s\n", __FUNCTION__, __LINE__, pParamName));
        bus_info->freefunc(faultParam);
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}

#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
ANSC_STATUS CosaDmlGetLowerLayersInstanceInWanManager(char *pLowerLayers, INT *piInstanceNumber)
{
    char acTmpReturnValue[256] = {0};
    INT iLoopCount = 0;
    INT iTotalNoofEntries = 0;

    if (ANSC_STATUS_FAILURE == Gponmgr_eth_getParams(WAN_MGR_COMPONENT_NAME, WAN_MGR_DBUS_PATH, WAN_NOE_PARAM_NAME, acTmpReturnValue))
    {
        CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    //Total count
    iTotalNoofEntries = atoi(acTmpReturnValue);
    CcspTraceInfo(("%s %d - TotalNoofEntries:%d\n", __FUNCTION__, __LINE__, iTotalNoofEntries));

    if (0 >= iTotalNoofEntries)
    {
        return ANSC_STATUS_SUCCESS;
    }

    //Traverse from loop
    for (iLoopCount = 0; iLoopCount < iTotalNoofEntries; iLoopCount++)
    {
        char acTmpQueryParam[256] = {0};

        //Query
        snprintf(acTmpQueryParam, sizeof(acTmpQueryParam), WAN_IF_NAME_PARAM_NAME, iLoopCount + 1);

        memset(acTmpReturnValue, 0, sizeof(acTmpReturnValue));
        if (ANSC_STATUS_FAILURE == Gponmgr_eth_getParams(WAN_MGR_COMPONENT_NAME, WAN_MGR_DBUS_PATH, acTmpQueryParam, acTmpReturnValue))
        {
            CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
            continue;
        }

        //Compare name
        if (0 == strcmp(acTmpReturnValue, pLowerLayers))
        {
            *piInstanceNumber = iLoopCount + 1;
            break;
        }
    }

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS CosaDmlGponSetPhyStatusForWanManager(int iVeipIndex ,char *LowerLayers, char *PhyStatus)
{
    char ParamName[256] = {0};
    char ParamVal[256] = {0};
    char ifname[256] = {0};
    PDML_VEIP pGponVeip = NULL;
    INT iWANInstance = -1;

    if (PhyStatus == NULL)
    {
        CcspTraceError(("%s Invalid Physical Status\n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_VEIP_CTRL_T* pGponVeipCtrl = pGponDmlData->gpon.Veip.pdata[iVeipIndex];
        if(pGponVeipCtrl!= NULL)
        {
            pGponVeip = &(pGponVeipCtrl->dml);

            // get veip interface structure
            if (pGponVeip == NULL)
            {
                CcspTraceError(("Error: Null Veip Interface\n"));
                GponMgrDml_GetData_release(pGponDmlData);
                return ANSC_STATUS_FAILURE;
            }

            //get Veip InterDomain Name
            snprintf(ifname, sizeof(ifname), "%s", pGponVeip->InterfaceName);
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    //Get Instance for corresponding name
    if (ANSC_STATUS_FAILURE == CosaDmlGetLowerLayersInstanceInWanManager(ifname, &iWANInstance))
    {
        CcspTraceError(("%s %d Failed to get Instance value\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if (iWANInstance == -1)
    {
        CcspTraceError(("%s %d WAN instance not present\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d WAN Instance:%d\n", __FUNCTION__, __LINE__, iWANInstance));

    memset(ParamName, 0, sizeof(ParamName));
    snprintf(ParamName, sizeof(ParamName),WAN_BASE_INTERFACE_PARAM_NAME, iWANInstance);
    if (ANSC_STATUS_FAILURE == Gponmgr_eth_getParams(WAN_MGR_COMPONENT_NAME, WAN_MGR_DBUS_PATH,ParamName,ParamVal))
    {
        CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if (strncmp(ParamVal, LowerLayers,sizeof(ParamVal)) != 0)
    {
        CcspTraceError(("%s %d BaseInterface is not matching with LowerLayer\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;      
    }
	
    //Set Link Status
    memset(ParamName, 0, sizeof(ParamName));
    snprintf(ParamName, sizeof(ParamName),WAN_LINK_STATUS_PARAM_NAME, iWANInstance);

    if (Gponmgr_eth_setParams(WAN_MGR_COMPONENT_NAME, WAN_MGR_DBUS_PATH, ParamName, PhyStatus, ccsp_string, TRUE) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d: Unable to set param name %s\n", __FUNCTION__, __LINE__,ParamName));
        return ANSC_STATUS_FAILURE;
    }
 
    CcspTraceInfo(("%s %d Successfully notified %s event to WAN Manager for %s interface\n", __FUNCTION__, __LINE__, PhyStatus, ifname));

    return ANSC_STATUS_SUCCESS;

}
#else
ANSC_STATUS Gponmgr_eth_addInterface(int iVeipIndex, char *LowerLayers, int *iVeipInstance)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
    char acSetParamName[MAX_STR_ARR_SIZE] = {0};
    char iDomainName[256] = {0};
    PDML_VEIP pGponVeip = NULL;
    INT iNewTableInstance = -1;

    *iVeipInstance = -1;


    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_VEIP_CTRL_T* pGponVeipCtrl = pGponDmlData->gpon.Veip.pdata[iVeipIndex];
        if(pGponVeipCtrl!= NULL)
        {
            pGponVeip = &(pGponVeipCtrl->dml);

            // get veip interface structure
            if (pGponVeip == NULL)
            {
                CcspTraceError(("Error: Null Veip Interface\n"));
                GponMgrDml_GetData_release(pGponDmlData);
                return ANSC_STATUS_FAILURE;
            }

            //get Veip Interface Name
            snprintf(iDomainName, sizeof(iDomainName), "%s", pGponVeip->InterfaceName);
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    if(isGponExistsInEthAgent(iDomainName, &iNewTableInstance) != ANSC_STATUS_SUCCESS) 
    {
        // Create table entry for veip
        if (CCSP_SUCCESS != CcspBaseIf_AddTblRow(
                            bus_handle,
                            ETH_MGR_COMPONENT_NAME,
                            ETH_MGR_DBUS_PATH,
                            0, /* session id */
                            ETH_MGR_IFACE_TABLE_NAME,
                            &iNewTableInstance))
        {
            CcspTraceError(("%s Failed to add table %s\n", __FUNCTION__, ETH_MGR_IFACE_TABLE_NAME));
            return ANSC_STATUS_FAILURE;
        }
    }

    //Assign new instance
    *iVeipInstance = iNewTableInstance;

    //Set Name - Eth Manager
    memset(acSetParamName, 0, MAX_STR_ARR_SIZE);
    snprintf(acSetParamName, MAX_STR_ARR_SIZE, ETH_MGR_IFACE_NAME, iNewTableInstance);
    Gponmgr_eth_setParams(ETH_MGR_COMPONENT_NAME, ETH_MGR_DBUS_PATH, acSetParamName, iDomainName, ccsp_string, FALSE);

    //Set Name - Wan Manager
    Gponmgr_eth_setParams(WAN_MGR_COMPONENT_NAME, WAN_MGR_DBUS_PATH, WAN_MGR_GPON_CPE_NAME, iDomainName, ccsp_string, TRUE);

    //Set LowerLayers - Eth Manager
    memset(acSetParamName,  0, MAX_STR_ARR_SIZE);
    snprintf(acSetParamName,  MAX_STR_ARR_SIZE, ETH_MGR_IFACE_LOWERLAYERS, iNewTableInstance);
    Gponmgr_eth_setParams(ETH_MGR_COMPONENT_NAME, ETH_MGR_DBUS_PATH, acSetParamName, LowerLayers, ccsp_string, FALSE);

    CcspTraceInfo(("%s.%d SUCCESS iVeipInstance[%d] - iVeipIndex[%d] \n", __FUNCTION__, __LINE__, *iVeipInstance, iVeipIndex ));

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS Gponmgr_eth_setEnableInterface(int iVeipInstance, BOOL bflag)
{
    char acSetParamName[MAX_STR_ARR_SIZE] = {0};

    snprintf(acSetParamName,  MAX_STR_ARR_SIZE, ETH_MGR_IFACE_ENABLE, iVeipInstance);

    if(bflag)
    {
        Gponmgr_eth_setParams(ETH_MGR_COMPONENT_NAME, ETH_MGR_DBUS_PATH, acSetParamName, STR_TRUE, ccsp_boolean, TRUE);
    }
    else
    {
        Gponmgr_eth_setParams(ETH_MGR_COMPONENT_NAME, ETH_MGR_DBUS_PATH, acSetParamName, STR_FALSE, ccsp_boolean, TRUE);
    }

    CcspTraceInfo(("%s.%d SUCCESS veipNum[%d]\n", __FUNCTION__, __LINE__, iVeipInstance));

    return ANSC_STATUS_SUCCESS;
}
#endif
