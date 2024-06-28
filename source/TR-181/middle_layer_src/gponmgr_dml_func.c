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

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

#include "ansc_platform.h"
#include "gpon_apis.h"
#include "gponmgr_dml_func.h"
#include "gponmgr_dml_backendmgr.h"
#include "gponmgr_dml_plugin_main.h"
#include "gponmgr_dml_obj.h"
#include "gponmgr_dml_hal.h"

#if     CFG_USE_CCSP_SYSLOG
    #include <ccsp_syslog.h>
#endif




/***********************************************************************
 IMPORTANT NOTE:

 According to TR69 spec:
 On successful receipt of a SetParameterValues RPC, the CPE MUST apply
 the changes to all of the specified Parameters atomically. That is, either
 all of the value changes are applied together, or none of the changes are
 applied at all. In the latter case, the CPE MUST return a fault response
 indicating the reason for the failure to apply the changes.

 The CPE MUST NOT apply any of the specified changes without applying all
 of them.

 In order to set parameter values correctly, the back-end is required to
 hold the updated values until "Validate" and "Commit" are called. Only after
 all the "Validate" passed in different objects, the "Commit" will be called.
 Otherwise, "Rollback" will be called instead.

 The sequence in COSA Data Model will be:

 SetParamBoolValue/SetParamIntValue/SetParamUlongValue/SetParamStringValue
 -- Backup the updated values;

 if( Validate_XXX())
 {
     Commit_XXX();    -- Commit the update all together in the same object
 }
 else
 {
     Rollback_XXX();  -- Remove the update at backup;
 }

***********************************************************************/
/***********************************************************************

 APIs for Object:


***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL GponPhy_Validate(ANSC_HANDLE hInsContext,char* pReturnParamName,ULONG* puLength);

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name.

    return:     TRUE if there's no validation.

**********************************************************************/

BOOL GponPhy_Validate (ANSC_HANDLE hInsContext,char* pReturnParamName,ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG Gpon_Commit(ANSC_HANDLE hInsContext);

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/

ULONG GponPhy_Commit(ANSC_HANDLE hInsContext)
{
    ANSC_STATUS                returnStatus  = ANSC_STATUS_SUCCESS;
    return returnStatus;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG Gpon_Rollback(ANSC_HANDLE hInsContext);

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/

BOOL GponPhy_IsUpdated(ANSC_HANDLE hInsContext)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
    BOOL        bIsUpdated = FALSE;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        ret = GponHal_get_pm(&(pGponDmlData->gpon.PhysicalMedia));
        if(ret == ANSC_STATUS_SUCCESS)
        {
            bIsUpdated = TRUE;
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return bIsUpdated;
}

ULONG GponPhy_Synchronize(ANSC_HANDLE hInsContext)
{

    ANSC_STATUS            returnStatus   = ANSC_STATUS_SUCCESS;

    return returnStatus;
}

ULONG GponPhy_GetEntryCount(ANSC_HANDLE hInsContext)
{
    ULONG count = 0;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        count = pGponDmlData->gpon.PhysicalMedia.ulQuantity;

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return count;
}

ANSC_HANDLE GponPhy_GetEntry(ANSC_HANDLE hInsContext,ULONG nIndex,ULONG* pInsNumber)
{
    ANSC_HANDLE pDmlEntry = NULL;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        if(nIndex < pGponDmlData->gpon.PhysicalMedia.ulQuantity)
        {
            DML_PHY_MEDIA_CTRL_T* pGponData = pGponDmlData->gpon.PhysicalMedia.pdata[nIndex];
            if(pGponData != NULL)
            {
                *pInsNumber = nIndex + 1;
                pDmlEntry = (ANSC_HANDLE) pGponData;
            }
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return pDmlEntry;
}

ULONG GponPhy_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
}

ULONG GponPhy_GetParamStringValue(ANSC_HANDLE hInsContext,char* ParamName,char* pValue,ULONG* pUlSize )
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    ULONG ret = -1;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "ModuleVendor") == 0)
            {
                AnscCopyString(pValue, pGponPhy->ModuleVendor);
                ret = 0;
            }
            else if (strcmp(ParamName, "ModuleName") == 0)
            {
                AnscCopyString(pValue, pGponPhy->ModuleName);
                ret = 0;
            }
            else if (strcmp(ParamName, "ModuleVersion") == 0)
            {
                AnscCopyString(pValue, pGponPhy->ModuleVersion);
                ret = 0;
            }
            else if (strcmp(ParamName, "ModuleFirmwareVersion") == 0)
            {
                AnscCopyString(pValue, pGponPhy->ModuleFirmwareVersion);
                ret = 0;
            }
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
            else if (strcmp(ParamName, "Alias") == 0)
            {
                AnscCopyString(pValue, pGponPhy->Alias);
                ret = 0;
            }
            else if (strcmp(ParamName, "LowerLayers") == 0)
            {
                AnscCopyString(pValue, pGponPhy->LowerLayers);
                ret = 0;
            }
#endif
            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
BOOL GponPhy_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};
    char strName[JSON_MAX_STR_ARR_SIZE]={0};
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Alias") == 0)
            {
		if (AnscSizeOfString(pString) >= sizeof(pGponPhy->Alias))
        	{
                	ret = FALSE;
        	}
                else
		{
                    	AnscCopyString(strValue, pString);
		}
                sprintf(strName, GPON_HAL_PM_ALIAS, pGponPhy->uInstanceNumber);
                if (GponHal_setParam(strName, PARAM_STRING, strValue) == ANSC_STATUS_SUCCESS)
                {
                    AnscCopyString(pGponPhy->Alias, pString);
                    ret = TRUE;
                }
            }
            else if (strcmp(ParamName, "LowerLayers") == 0)
            {
                if (AnscSizeOfString(pString) >= sizeof(pGponPhy->LowerLayers))
                {
                        ret = FALSE;
                }
                else
                {
                        AnscCopyString(strValue, pString);
                }
                sprintf(strName, GPON_HAL_PM_LOWERLAYERS, pGponPhy->uInstanceNumber);
                if (GponHal_setParam(strName, PARAM_STRING, strValue) == ANSC_STATUS_SUCCESS)
                {
                    AnscCopyString(pGponPhy->LowerLayers, pString);
                    ret = TRUE;
                }
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}
#endif

BOOL GponPhy_GetParamUlongValue (ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Cage") == 0)
            {
                *puLong = (ULONG) pGponPhy->Cage;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "PonMode") == 0)
            {
                *puLong = (ULONG) pGponPhy->PonMode;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Connector") == 0)
            {
                *puLong = (ULONG) pGponPhy->Connector;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "NominalBitRateDownstream") == 0)
            {
                *puLong = (ULONG) pGponPhy->NominalBitRateDownstream;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "NominalBitRateUpstream") == 0)
            {
                *puLong = (ULONG) pGponPhy->NominalBitRateUpstream;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Status") == 0)
            {
                *puLong = (ULONG) pGponPhy->Status;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "RedundancyState") == 0)
            {
                *puLong = (ULONG) pGponPhy->RedundancyState;
                ret = TRUE;
            }
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
            else if (strcmp(ParamName, "LastChange") == 0)
            {
                *puLong = (ULONG) pGponPhy->LastChange;
                ret = TRUE;
            }
#endif

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
BOOL GponPhy_GetParamBoolValue(ANSC_HANDLE hInsContext,char* ParamName,BOOL* pBool)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Enable") == 0)
            {
                *pBool = pGponPhy->Enable;
                ret = TRUE;
            }

            else if (strcmp(ParamName, "Upstream") == 0)
            {
                *pBool = pGponPhy->Upstream;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponPhy_SetParamBoolValue(ANSC_HANDLE hInsContext,char* ParamName,BOOL bValue)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};
    char strName[JSON_MAX_STR_ARR_SIZE]={0};
    BOOL ret = FALSE;


    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Enable") == 0)
            {
                if (bValue==TRUE)
                {
                    strncpy(strValue,JSON_STR_TRUE,strlen(JSON_STR_TRUE)+1);
                }
                else
                {
                    strncpy(strValue,JSON_STR_FALSE,strlen(JSON_STR_FALSE)+1);
                }

                sprintf(strName, GPON_HAL_PM_ENABLE, pGponPhy->uInstanceNumber);
                if (GponHal_setParam(strName, PARAM_BOOLEAN, strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponPhy->Enable = bValue;
                    ret = TRUE;
                }
            }
            else if (strcmp(ParamName, "Upstream") == 0)
            {
                if (bValue==TRUE)
                {
                    strncpy(strValue,JSON_STR_TRUE,strlen(JSON_STR_TRUE)+1);
                }
                else
                {
                    strncpy(strValue,JSON_STR_FALSE,strlen(JSON_STR_FALSE)+1);
                }

                sprintf(strName, GPON_HAL_PM_UPSTREAM, pGponPhy->uInstanceNumber);
                if (GponHal_setParam(strName, PARAM_BOOLEAN, strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponPhy->Upstream = bValue;
                    ret = TRUE;
                }
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}
#endif

BOOL GponPhyRxpwr_GetParamIntValue(ANSC_HANDLE hInsContext,char* ParamName,int* pInt)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "SignalLevel") == 0)
            {
                *pInt = (int) pGponPhy->RxPower.SignalLevel;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "SignalLevelLowerThreshold") == 0)
            {
                *pInt = (int) pGponPhy->RxPower.SignalLevelLowerThreshold;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "SignalLevelUpperThreshold") == 0)
            {
                *pInt = (int) pGponPhy->RxPower.SignalLevelUpperThreshold;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponPhyRxpwr_SetParamIntValue(ANSC_HANDLE hInsContext,char* ParamName,int iValue)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};
    char strName[JSON_MAX_STR_ARR_SIZE]={0};
    BOOL ret = FALSE;


    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "SignalLevelLowerThreshold") == 0)
            {
                sprintf(strName, GPON_HAL_PM_RX_POWER_LOWER_THR, pGponPhy->uInstanceNumber);
                snprintf(strValue, JSON_MAX_VAL_ARR_SIZE, "%d", iValue);
                if (GponHal_setParam(strName,PARAM_INTEGER,strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponPhy->RxPower.SignalLevelLowerThreshold=iValue;
                    ret = TRUE;
                }
            }
            else if (strcmp(ParamName, "SignalLevelUpperThreshold") == 0)
            {
                sprintf(strName, GPON_HAL_PM_RX_POWER_UPPER_THR, pGponPhy->uInstanceNumber);
                snprintf(strValue, JSON_MAX_VAL_ARR_SIZE, "%d", iValue);
                if (GponHal_setParam(strName,PARAM_INTEGER,strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponPhy->RxPower.SignalLevelUpperThreshold=iValue;
                    ret = TRUE;
                }
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponPhyTxpwr_GetParamIntValue(ANSC_HANDLE hInsContext,char* ParamName,int* pInt)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "SignalLevel") == 0)
            {
                *pInt = (int) pGponPhy->TxPower.SignalLevel;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "SignalLevelLowerThreshold") == 0)
            {
                *pInt = (int) pGponPhy->TxPower.SignalLevelLowerThreshold;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "SignalLevelUpperThreshold") == 0)
            {
                *pInt = (int) pGponPhy->TxPower.SignalLevelUpperThreshold;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponPhyTxpwr_SetParamIntValue(ANSC_HANDLE hInsContext,char* ParamName,int iValue)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};
    char strName[JSON_MAX_STR_ARR_SIZE]={0};
    BOOL ret = FALSE;


    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "SignalLevelLowerThreshold") == 0)
            {
                sprintf(strName, GPON_HAL_PM_TX_POWER_LOWER_THR, pGponPhy->uInstanceNumber);
                snprintf(strValue, JSON_MAX_VAL_ARR_SIZE, "%d", iValue);
                if (GponHal_setParam(strName, PARAM_INTEGER, strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponPhy->TxPower.SignalLevelLowerThreshold=iValue;
                    ret = TRUE;
                }
            }
            else if (strcmp(ParamName, "SignalLevelUpperThreshold") == 0)
            {
                sprintf(strName, GPON_HAL_PM_TX_POWER_UPPER_THR, pGponPhy->uInstanceNumber);
                snprintf(strValue, JSON_MAX_VAL_ARR_SIZE, "%d", iValue);
                if (GponHal_setParam(strName, PARAM_INTEGER, strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponPhy->TxPower.SignalLevelUpperThreshold=iValue;
                    ret = TRUE;
                }
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponPhyVoltage_GetParamIntValue(ANSC_HANDLE hInsContext,char* ParamName,int* pInt)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
            if (strcmp(ParamName, "CurrentVoltage") == 0)
            {
                *pInt = (int) pGponPhy->Voltage.CurrentVoltage;
                ret = TRUE;
            }
#else
            if (strcmp(ParamName, "VoltageLevel") == 0)
            {
                *pInt = (int) pGponPhy->Voltage.VoltageLevel;
                ret = TRUE;
            }
#endif
            
            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponPhyBias_GetParamUlongValue( ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "CurrentBias") == 0)
            {
                *puLong = pGponPhy->Bias.CurrentBias;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponPhyTemperature_GetParamIntValue(ANSC_HANDLE hInsContext,char* ParamName,int* pInt)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "CurrentTemp") == 0)
            {
                *pInt = (int) pGponPhy->Temperature.CurrentTemp;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponPhyPerformanceThreshold_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_PHY_MEDIA* pGponPhy = &(pGponCtrl->dml);

            if (strcmp(ParamName, "SignalFail") == 0)
            {
                *puLong= pGponPhy->PerformThreshold.SignalFail;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "SignalDegrade") == 0)
            {
                *puLong= pGponPhy->PerformThreshold.SignalDegrade;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponPhyAlarm_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_PHY_MEDIA_CTRL_T* pGponCtrl = (DML_PHY_MEDIA_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_ALARM* pGponPhyAlarm = &(pGponCtrl->dml.Alarm);

            if (strcmp(ParamName, "RDI") == 0)
            {
                *puLong = pGponPhyAlarm->RDI;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "PEE") == 0)
            {
                *puLong =pGponPhyAlarm->PEE;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "LOS") == 0)
            {
                *puLong = pGponPhyAlarm->LOS;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "LOF") == 0)
            {
                *puLong = pGponPhyAlarm->LOF;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "DACT") == 0)
            {
                *puLong = pGponPhyAlarm->DACT;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "DIS") == 0)
            {
                *puLong = pGponPhyAlarm->DIS;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "MIS") == 0)
            {
                *puLong = pGponPhyAlarm->MIS;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "MEM") == 0)
            {
                *puLong = pGponPhyAlarm->MEM;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "SUF") == 0)
            {
                *puLong = pGponPhyAlarm->SUF;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "SF") == 0)
            {
                *puLong = pGponPhyAlarm->SF;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "SD") == 0)
            {
                *puLong = pGponPhyAlarm->SD;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "LCDG") == 0)
            {
                *puLong = pGponPhyAlarm->LCDG;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "TF") == 0)
            {
                *puLong = pGponPhyAlarm->TF;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "ROGUE") == 0)
            {
                *puLong = pGponPhyAlarm->ROGUE;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponGtc_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    BOOL ret = FALSE;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_GTC* pGponGtc = &(pGponDmlData->gpon.Gtc);
        if(GponHal_get_gtc(pGponGtc) == ANSC_STATUS_SUCCESS)
        {
            if (strcmp(ParamName, "CorrectedFecBytes") == 0)
            {
                *puLong= pGponGtc->CorrectedFecBytes;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "CorrectedFecCodeWords") == 0)
            {
                *puLong= pGponGtc->CorrectedFecCodeWords;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "UnCorrectedFecCodeWords") == 0)
            {
                *puLong= pGponGtc->UnCorrectedFecCodeWords;
                ret = TRUE;
            }
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
            else if (strcmp(ParamName, "GtcTotalFecCodeWords") == 0)
            {
                *puLong= pGponGtc->GtcTotalFecCodeWords;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "GtcHecErrorCount") == 0)
            {
                *puLong= pGponGtc->GtcHecErrorCount;
                ret = TRUE;
            }
#else
            else if (strcmp(ParamName, "TotalFecCodeWords") == 0)
            {
                *puLong= pGponGtc->TotalFecCodeWords;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "HecErrorCount") == 0)
            {
                *puLong= pGponGtc->HecErrorCount;
                ret = TRUE;
            }
#endif
            else if (strcmp(ParamName, "PSBdHecErrors") == 0)
            {
                *puLong= pGponGtc->PSBdHecErrors;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "FrameHecErrors") == 0)
            {
                *puLong= pGponGtc->FrameHecErrors;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "FramesLost") == 0)
            {
                *puLong= pGponGtc->FramesLost;
                ret = TRUE;
            }
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return ret;
}


BOOL GponPloam_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    BOOL ret = FALSE;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_PLOAM* pGponPloam = &(pGponDmlData->gpon.Ploam);

        if ( GponHal_get_ploam(pGponPloam) == ANSC_STATUS_SUCCESS)
        {
            if (strcmp(ParamName, "OnuId") == 0)
            {
                *puLong= pGponPloam->OnuId;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "RegistrationState") == 0)
            {
                *puLong= pGponPloam->RegistrationState;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "ActivationCounter") == 0)
            {
                *puLong= pGponPloam->ActivationCounter;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "TxMessageCount") == 0)
            {
                *puLong= pGponPloam->TxMessageCount;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "RxMessageCount") == 0)
            {
                *puLong= pGponPloam->RxMessageCount;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "MicErrors") == 0)
            {
                *puLong= pGponPloam->MicErrors;
                ret = TRUE;
            }
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
            else if (strcmp(ParamName, "TO1Timer") == 0)
            {
                *puLong= pGponPloam->TO1Timer;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "TO2Timer") == 0)
            {
                *puLong= pGponPloam->TO2Timer;
                ret = TRUE;
            }
#endif
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return ret;
}

ULONG GponPloam_GetParamStringValue(ANSC_HANDLE hInsContext,char* ParamName,char* pValue,ULONG* pUlSize)
{
    ULONG ret = -1;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_PLOAM* pGponPloam = &(pGponDmlData->gpon.Ploam);

        if ( GponHal_get_ploam(pGponPloam) == ANSC_STATUS_SUCCESS)
        {
            if (strcmp(ParamName, "SerialNumber") == 0)
            {
                AnscCopyString(pValue, pGponPloam->SerialNumber);
                ret = 0;
            }
            else if (strcmp(ParamName, "VendorId") == 0)
            {
                AnscCopyString(pValue, pGponPloam->VendorId);
                ret = 0;
            }
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return ret;
}

#if !defined(WAN_MANAGER_UNIFICATION_ENABLED)
BOOL GponPloamRegTmr_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    BOOL ret = FALSE;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_PLOAM* pGponPloam = &(pGponDmlData->gpon.Ploam);

        if (strcmp(ParamName, "TO1") == 0)
        {
            *puLong= pGponPloam->RegistrationTimers.TO1;
            ret = TRUE;
        }
        else if (strcmp(ParamName, "TO2") == 0)
        {
            *puLong= pGponPloam->RegistrationTimers.TO2;
            ret = TRUE;
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return ret;
}
#endif

BOOL GponGem_IsUpdated(ANSC_HANDLE hInsContext)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
    BOOL        bIsUpdated = FALSE;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        ret = GponHal_get_gem(&(pGponDmlData->gpon.Gem));
        if(ret == ANSC_STATUS_SUCCESS)
        {
            bIsUpdated = TRUE;
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return bIsUpdated;
}

ULONG GponGem_Synchronize(ANSC_HANDLE hInsContext)
{
    ANSC_STATUS            returnStatus   = ANSC_STATUS_SUCCESS;
    return returnStatus;
}

ULONG GponGem_GetEntryCount(ANSC_HANDLE hInsContext)
{
    ULONG count = 0;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        count = pGponDmlData->gpon.Gem.ulQuantity;

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return count;
}

ANSC_HANDLE GponGem_GetEntry(ANSC_HANDLE hInsContext,ULONG nIndex,ULONG* pInsNumber)
{
    ANSC_HANDLE pDmlEntry = NULL;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        if(nIndex < pGponDmlData->gpon.Gem.ulQuantity)
        {
            DML_GEM_CTRL_T* pGponData = pGponDmlData->gpon.Gem.pdata[nIndex];
            if(pGponData != NULL)
            {
                *pInsNumber = nIndex + 1;
                pDmlEntry = (ANSC_HANDLE) pGponData;
            }
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return pDmlEntry;
}

BOOL GponGem_Validate(ANSC_HANDLE hInsContext,char* pReturnParamName,ULONG* puLength)
{
    return TRUE;
}

ULONG GponGem_Commit(ANSC_HANDLE hInsContext)
{
    return 0;
}

ULONG GponGem_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
}

BOOL GponGem_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_GEM_CTRL_T* pGponCtrl = (DML_GEM_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_GEM* pGponGem = &(pGponCtrl->dml);

            if (strcmp(ParamName, "PortId") == 0)
            {
                *puLong= pGponGem->PortId;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "TrafficType") == 0)
            {
                *puLong= pGponGem->TrafficType;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "TransmittedFrames") == 0)
            {
                *puLong= pGponGem->TransmittedFrames;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "ReceivedFrames") == 0)
            {
                *puLong= pGponGem->ReceivedFrames;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponGemEthFlowIngress_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_GEM_CTRL_T* pGponCtrl = (DML_GEM_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_GEM* pGponGem = &(pGponCtrl->dml);
            if (strcmp(ParamName, "Tagged") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Ingress.Tagged;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponGemEthFlowIngressSvlan_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_GEM_CTRL_T* pGponCtrl = (DML_GEM_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_GEM* pGponGem = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Vid") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Ingress.SVLAN.Vid;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Pcp") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Ingress.SVLAN.Pcp;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Dei") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Ingress.SVLAN.Dei;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponGemEthFlowIngressCvlan_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_GEM_CTRL_T* pGponCtrl = (DML_GEM_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_GEM* pGponGem = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Vid") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Ingress.CVLAN.Vid;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Pcp") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Ingress.CVLAN.Pcp;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Dei") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Ingress.CVLAN.Dei;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponGemEthFlowEgress_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_GEM_CTRL_T* pGponCtrl = (DML_GEM_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_GEM* pGponGem = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Tagged") == 0)
            {
                *puLong = pGponGem->EthernetFlow.Egress.Tagged;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponGemEthFlowEgressSvlan_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_GEM_CTRL_T* pGponCtrl = (DML_GEM_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_GEM* pGponGem = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Vid") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Egress.SVLAN.Vid;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Pcp") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Egress.SVLAN.Pcp;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Dei") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Egress.SVLAN.Dei;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponGemEthFlowEgressCvlan_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_GEM_CTRL_T* pGponCtrl = (DML_GEM_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_GEM* pGponGem = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Vid") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Egress.CVLAN.Vid;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Pcp") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Egress.CVLAN.Pcp;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Dei") == 0)
            {
                *puLong= pGponGem->EthernetFlow.Egress.CVLAN.Dei;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}


BOOL GponOmci_GetParamIntValue(ANSC_HANDLE hInsContext,char* ParamName,int* pInt)
{
    BOOL ret = FALSE;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_OMCI* pGponOmci = &(pGponDmlData->gpon.Omci);
        if ( GponHal_get_omci(pGponOmci) == ANSC_STATUS_SUCCESS)
        {
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
            if (strcmp(ParamName, "BaselineMessageCount") == 0)
            {
                *pInt = (int) pGponOmci->BaselineMessageCount;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "ExtendedMessageCount") == 0)
            {
                *pInt = (int) pGponOmci->ExtendedMessageCount;
                ret = TRUE;
            }
#else
            if (strcmp(ParamName, "RxBaseLineMessageCountValid") == 0)
            {
                *pInt = (int) pGponOmci->RxBaseLineMessageCountValid;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "RxExtendedMessageCountValid") == 0)
            {
                *pInt = (int) pGponOmci->RxExtendedMessageCountValid;
                ret = TRUE;
            }
#endif
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return ret;
}

BOOL GponOmci_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
{
    BOOL ret = FALSE;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_OMCI* pGponOmci = &(pGponDmlData->gpon.Omci);
        if ( GponHal_get_omci(pGponOmci) == ANSC_STATUS_SUCCESS)
        {
            if (strcmp(ParamName, "MicErrors") == 0)
            {
                *puLong = pGponOmci->MicErrors;
                ret = TRUE;
            }
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return ret;
}

BOOL GponVeip_IsUpdated(ANSC_HANDLE hInsContext)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
    BOOL        bIsUpdated = FALSE;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        ret = GponHal_get_veip(&(pGponDmlData->gpon.Veip));
        if(ret == ANSC_STATUS_SUCCESS)
        {
            bIsUpdated = TRUE;
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return bIsUpdated;
}

ULONG GponVeip_Synchronize(ANSC_HANDLE hInsContext)
{

    ANSC_STATUS            returnStatus   = ANSC_STATUS_SUCCESS;

    return returnStatus;
}

ULONG GponVeip_GetEntryCount(ANSC_HANDLE hInsContext)
{
    ULONG count = 0;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        count = pGponDmlData->gpon.Veip.ulQuantity;

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return count;
}

ANSC_HANDLE GponVeip_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber)
{
    ANSC_HANDLE pDmlEntry = NULL;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        if(nIndex < pGponDmlData->gpon.Veip.ulQuantity)
        {
            DML_VEIP_CTRL_T* pGponData = pGponDmlData->gpon.Veip.pdata[nIndex];
            if(pGponData != NULL)
            {
                *pInsNumber = nIndex + 1;
                pDmlEntry = (ANSC_HANDLE) pGponData;
            }
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return pDmlEntry;
}

BOOL GponVeip_Validate(ANSC_HANDLE hInsContext,char* pReturnParamName,ULONG* puLength)
{
    return TRUE;
}

ULONG GponVeip_Commit(ANSC_HANDLE hInsContext)
{
    return 0;
}

ULONG GponVeip_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
}

ULONG GponVeip_GetParamStringValue(ANSC_HANDLE hInsContext,char* ParamName,char* pValue,ULONG* pUlSize)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    ULONG ret = -1;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

            if (strcmp(ParamName, "InterDomainName") == 0)
            {
                AnscCopyString(pValue, pGponVeip->InterDomainName);
                ret = 0;
            }
            else if (strcmp(ParamName, "InterfaceName") == 0)
            {
                AnscCopyString(pValue, pGponVeip->InterfaceName);
                ret = 0;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponVeip_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
            if (strcmp(ParamName, "ManagedEntityId") == 0)
            {
                *puLong = pGponVeip->ManagedEntityId;
                ret = TRUE;
            }
#else
            if (strcmp(ParamName, "MeId") == 0)
            {
                *puLong = pGponVeip->MeId;
                ret = TRUE;
            }
#endif
            else if (strcmp(ParamName, "AdministrativeState") == 0)
            {
                *puLong = pGponVeip->AdministrativeState;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "OperationalState") == 0)
            {
                *puLong = pGponVeip->OperationalState;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponVeipEthFlowIngress_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Tagged") == 0)
            {
                *puLong= pGponVeip->EthernetFlow.Ingress.Tagged;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponVeipEthFlowIngress_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};
    char strName[JSON_MAX_STR_ARR_SIZE]={0};
    char taggedValue[3][16]={"Untagged","Single","Double"};
    BOOL ret = FALSE;


    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

            snprintf(strValue,JSON_MAX_VAL_ARR_SIZE,"%s",taggedValue[uValue]);

            /* check the parameter name and return the corresponding value */
            if (strcmp(ParamName, "Tagged") == 0)
            {
                snprintf(strName,JSON_MAX_STR_ARR_SIZE,GPON_HAL_VEIP_ETH_INGRESS_TAGGED,pGponVeip->uInstanceNumber);
                if (GponHal_setParam(strName, PARAM_STRING, strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponVeip->EthernetFlow.Ingress.Tagged = uValue;
                    ret = TRUE;
                }
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponVeipEthFlowIngressQlan_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Vid") == 0)
            {
                *puLong= pGponVeip->EthernetFlow.Ingress.QVLAN.Vid;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Pcp") == 0)
            {
                *puLong= pGponVeip->EthernetFlow.Ingress.QVLAN.Pcp;
                ret = TRUE;
            }
            else if (strcmp(ParamName, "Dei") == 0)
            {
                *puLong= pGponVeip->EthernetFlow.Ingress.QVLAN.Dei;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponVeipEthFlowIngressQlan_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName,ULONG uValue)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};
    char strName[JSON_MAX_STR_ARR_SIZE]={0};
    BOOL ret = FALSE;


    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

            snprintf(strValue,JSON_MAX_VAL_ARR_SIZE,"%ld",uValue);

            /* check the parameter name and return the corresponding value */
            if (strcmp(ParamName, "Vid") == 0)
            {
                snprintf(strName,JSON_MAX_STR_ARR_SIZE,GPON_HAL_VEIP_ETH_INGRESS_QVLAN_VID,pGponVeip->uInstanceNumber);
                if (GponHal_setParam(strName,PARAM_UNSIGNED_INTEGER,strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponVeip->EthernetFlow.Ingress.QVLAN.Vid = uValue;
                    ret = TRUE;
                }
            }
            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponVeipEthFlowEgress_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Tagged") == 0)
            {
                *puLong= pGponVeip->EthernetFlow.Egress.Tagged;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponVeipEthFlowEgress_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};
    char strName[JSON_MAX_STR_ARR_SIZE]={0};
    char taggedValue[3][16]={"Untagged","Single","Double"};
    BOOL ret = FALSE;


    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

            snprintf(strValue,JSON_MAX_VAL_ARR_SIZE,"%s",taggedValue[uValue]);

            /* check the parameter name and return the corresponding value */
            if (strcmp(ParamName, "Tagged") == 0)
            {
                snprintf(strName,JSON_MAX_STR_ARR_SIZE,GPON_HAL_VEIP_ETH_EGRESS_TAGGED,pGponVeip->uInstanceNumber);
                if (GponHal_setParam(strName, PARAM_STRING, strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponVeip->EthernetFlow.Ingress.Tagged = uValue;
                    ret = TRUE;
                }
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}


BOOL GponVeipEthFlowEgressQlan_GetParamUlongValue(ANSC_HANDLE hInsContext,char* ParamName,ULONG* puLong)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    BOOL ret = FALSE;

    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

            if (strcmp(ParamName, "Vid") == 0)
            {
                *puLong= pGponVeip->EthernetFlow.Egress.QVLAN.Vid;
                ret = TRUE;
            }
            if (strcmp(ParamName, "Pcp") == 0)
            {
                *puLong= pGponVeip->EthernetFlow.Egress.QVLAN.Pcp;
                ret = TRUE;
            }
            if (strcmp(ParamName, "Dei") == 0)
            {
                *puLong= pGponVeip->EthernetFlow.Egress.QVLAN.Dei;
                ret = TRUE;
            }

            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

BOOL GponVeipEthFlowEgressQlan_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName,ULONG uValue)
{
    DML_VEIP_CTRL_T* pGponCtrl = (DML_VEIP_CTRL_T*) hInsContext;
    char strValue[JSON_MAX_VAL_ARR_SIZE]={0};
    char strName[JSON_MAX_STR_ARR_SIZE]={0};
    BOOL ret = FALSE;


    if(pGponCtrl != NULL)
    {
        GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
        if(pGponDmlData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponCtrl->dml);

            snprintf(strValue,JSON_MAX_VAL_ARR_SIZE,"%ld",uValue);

            /* check the parameter name and return the corresponding value */
            if (strcmp(ParamName, "Vid") == 0)
            {
                snprintf(strName,JSON_MAX_STR_ARR_SIZE,GPON_HAL_VEIP_ETH_EGRESS_QVLAN_VID,pGponVeip->uInstanceNumber);
                if (GponHal_setParam(strName,PARAM_UNSIGNED_INTEGER,strValue) == ANSC_STATUS_SUCCESS)
                {
                    pGponVeip->EthernetFlow.Egress.QVLAN.Vid = uValue;
                    ret = TRUE;
                }
            }
            GponMgrDml_GetData_release(pGponDmlData);
        }
    }

    return ret;
}

ULONG Gpontr69_GetParamStringValue(ANSC_HANDLE hInsContext,char* ParamName,char* pValue,ULONG* pUlSize)
{
    ULONG ret = -1;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_TR69* pGponTr69 = &(pGponDmlData->gpon.Tr69);
        if( GponHal_get_tr69(pGponTr69) == ANSC_STATUS_SUCCESS)
        {
            if (strcmp(ParamName, "url") == 0)
            {
                AnscCopyString(pValue, pGponTr69->url);

                ret = 0;
            }
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return ret;
}

BOOL Gpontr69_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
{
    BOOL ret = FALSE;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_TR69* pGponTr69 = &(pGponDmlData->gpon.Tr69);

        if (strcmp(ParamName, "AssociatedTag") == 0)
        {
            *puLong= pGponTr69->AssociatedTag;

            ret = TRUE;
        }

        GponMgrDml_GetData_release(pGponDmlData);
    }

    return ret;
}

