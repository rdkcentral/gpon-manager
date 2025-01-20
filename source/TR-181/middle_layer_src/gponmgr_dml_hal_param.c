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
/*****************************************************************************
* PROJECT-SPECIFIC INCLUDE FILES
*****************************************************************************/
#include "json_hal_client.h"
#include "ansc_platform.h"

#include "gpon_apis.h"
#include "gponmgr_dml_hal_param.h"




ANSC_STATUS gpon_hal_get_pm_index(char* ParamName, int* index)
{
    sscanf(ParamName,"Device.X_RDK_ONT.PhysicalMedia.%d.", index);
    return ANSC_STATUS_SUCCESS;    
}

ANSC_STATUS gpon_hal_get_gem_index(char* ParamName, int* index)
{    
    sscanf(ParamName,"Device.X_RDK_ONT.Gem.%d.", index);
    return ANSC_STATUS_SUCCESS;    
}

ANSC_STATUS gpon_hal_get_veip_index(char* ParamName, int* index)
{    
    sscanf(ParamName,"Device.X_RDK_ONT.Veip.%d.", index);
    return ANSC_STATUS_SUCCESS;    
}

ANSC_STATUS Map_hal_dml_alarm(DML_ALARM* pGponAlarm, char* ParamName, char* pValue)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;

    if (strstr(ParamName, "RDI"))
    {
        pGponAlarm->RDI = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "PEE"))
    {
        pGponAlarm->PEE = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "LOS"))
    {
        pGponAlarm->LOS = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "LOF"))
    {
        pGponAlarm->LOF = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "DACT"))
    {
        pGponAlarm->DACT = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "DIS"))
    {
        pGponAlarm->DIS = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "MIS"))
    {
        pGponAlarm->MIS = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "MEM"))
    {
        pGponAlarm->MEM = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "SUF"))
    {
        pGponAlarm->SUF = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "SD"))
    {
        pGponAlarm->SD = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "SF"))
    {
        pGponAlarm->SF = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "LCDG"))
    {
        pGponAlarm->LCDG = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "TF"))
    {
        pGponAlarm->TF = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "ROGUE"))
    {
        pGponAlarm->ROGUE = strstr(pValue, "Active")?ACTIVE:INACTIVE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    
    if(retStatus == ANSC_STATUS_FAILURE)
    {
        AnscTraceError(("%s:%d:: Invalid ParamName[%s] paramValue[%s].\n", __FUNCTION__, __LINE__, ParamName, pValue));
    }
    
    return retStatus;
}

ANSC_STATUS Map_hal_dml_pm(DML_PHY_MEDIA_LIST_T* gponPhyList, char* ParamName, char* pValue)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    char *err;
    int hal_index = 0;
    int pm_index = 0;
  
    gpon_hal_get_pm_index(ParamName, &hal_index);    
    if(hal_index <= 0)
    {
        AnscTraceError(("%s:%d:: Invalid index ParamName[%s]\n", __FUNCTION__, __LINE__, ParamName));
        return ANSC_STATUS_FAILURE;
    }
    
    //get array index
    pm_index = hal_index - 1;
    
    DML_PHY_MEDIA_CTRL_T* pPhyMediaData = gponPhyList->pdata[pm_index];

    if(pPhyMediaData == NULL)
    {
        //create new Veip
        GponMgrDml_addPhysicalMedia(gponPhyList, pm_index);
        
        pPhyMediaData = gponPhyList->pdata[pm_index];
        if(pPhyMediaData == NULL)
        {
            return ANSC_STATUS_FAILURE;
        }        
    }

    DML_PHY_MEDIA* pPhyMedia = &(pPhyMediaData->dml);
    
    pPhyMedia->uInstanceNumber = hal_index;
    
    if( strstr(ParamName, "ModuleVendor"))
    {
        strncpy(pPhyMedia->ModuleVendor,pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "ModuleName"))
    {
        strncpy(pPhyMedia->ModuleName,pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "ModuleVersion"))
    {
        strncpy(pPhyMedia->ModuleVersion, pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "ModuleFirmwareVersion"))
    {
        strncpy(pPhyMedia->ModuleFirmwareVersion, pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "Cage"))
    {
        pPhyMedia->Cage = strstr(pValue,"SFP")?SFP:BoB;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "PonMode"))
    {
        //GPON(0),XGPON(1),NGPON2(2),XGPON2(3)
        if(strstr(pValue,"GPON"))
        {
            pPhyMedia->PonMode = GPON;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"XGPON"))
        {
            if (strstr(pValue,"XGPON2"))
            {
                pPhyMedia->PonMode = XGPON2;
            retStatus = ANSC_STATUS_SUCCESS;
            }
            else
            {
                pPhyMedia->PonMode = XGPON;
            retStatus = ANSC_STATUS_SUCCESS;
            }
        }
        else if (strstr(pValue,"NGPON2"))
        {
            pPhyMedia->PonMode = NGPON2;
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }
    else if( strstr(ParamName, "Connector"))
    {
        //LC(0),ST(1),FC(2),SC(3), MT-RJ(4)
        
        if(strstr(pValue,"LC"))
        {
            pPhyMedia->Connector = LC;
            retStatus = ANSC_STATUS_SUCCESS;
        }        
        else if(strstr(pValue,"ST"))
        {
            pPhyMedia->Connector = ST;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"FC"))
        {
            pPhyMedia->Connector = FC;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"SC"))
        {
            pPhyMedia->Connector = SC;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"MT-RJ"))
        {
            pPhyMedia->Connector = MT_RJ;
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }
    else if( strstr(ParamName, "NominalBitRateDownstream"))
    {
        pPhyMedia->NominalBitRateDownstream = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "NominalBitRateUpstream"))
    {
        pPhyMedia->NominalBitRateUpstream = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    else if( strstr(ParamName, "Enable"))
    {
        pPhyMedia->Enable = strstr(pValue,"1")?TRUE:FALSE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
#endif
    else if( strstr(ParamName, "Status"))
    {
        //Up(0),Down(1),Unknown(2),Dormant(3),NotPresent(4),LowerLayerDown(5),Error(6)
        if(strstr(pValue,"Up"))
        {
            pPhyMedia->Status = Up;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"Down"))
        {
            pPhyMedia->Status = Down;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"Unknown"))
        {
            pPhyMedia->Status = Unknown;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"Dormant"))
        {
            pPhyMedia->Status = Dormant;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"NotPresent"))
        {
            pPhyMedia->Status = NotPresent;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"LowerLayerDown"))
        {
            pPhyMedia->Status = LowerLayerDown;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"Error"))
        {
            pPhyMedia->Status = Error;
            retStatus = ANSC_STATUS_SUCCESS;;
        }
    }
    else if( strstr(ParamName, "RedundancyState"))
    {
        if(strstr(pValue,"Active"))
        {
            pPhyMedia->RedundancyState = Active;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"Standby"))
        {
            pPhyMedia->RedundancyState = Standby;
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    else if( strstr(ParamName, "Alias"))
    {
        strncpy(pPhyMedia->Alias,pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "LastChange"))
    {
        pPhyMedia->LastChange = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "LowerLayers"))
    {
        strncpy(pPhyMedia->LowerLayers,pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "Upstream"))
    {
        pPhyMedia->Upstream = strstr(pValue,"1")?TRUE:FALSE;
        retStatus = ANSC_STATUS_SUCCESS;
    }
#endif

    /* RxPower */
    else if(strstr(ParamName,"RxPower"))
    {
        if( strstr(ParamName, "SignalLevelLowerThreshold"))
        {
            pPhyMedia->RxPower.SignalLevelLowerThreshold  = strtol(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if( strstr(ParamName, "SignalLevelUpperThreshold"))
        {
            pPhyMedia->RxPower.SignalLevelUpperThreshold  = strtol(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if( strstr(ParamName, "SignalLevel"))
        {
            pPhyMedia->RxPower.SignalLevel = strtol(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }

    }

    /* TXPower */
    else if(strstr(ParamName,"TxPower"))
    {
        if( strstr(ParamName, "SignalLevelLowerThreshold"))
        {
            pPhyMedia->TxPower.SignalLevelLowerThreshold  = strtol(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if( strstr(ParamName, "SignalLevelUpperThreshold"))
        {
            pPhyMedia->TxPower.SignalLevelUpperThreshold  = strtol(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if( strstr(ParamName, "SignalLevel"))
        {
            pPhyMedia->TxPower.SignalLevel  = strtol(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }

    /* Voltage */
    else if( strstr(ParamName, "VoltageLevel"))
    {
        pPhyMedia->Voltage.VoltageLevel  = strtol(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }

    /* Bias */
    else if( strstr(ParamName, "Bias"))
    {
        if( strstr(ParamName, "CurrentBias"))
        {
            pPhyMedia->Bias.CurrentBias  = strtoull(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }

    /* Temperature */
    else if( strstr(ParamName, "Temperature"))
    {
        if( strstr(ParamName, "CurrentTemp"))
        {
            pPhyMedia->Temperature.CurrentTemp = strtol(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }

    /* PerformThreshold */
    else if( strstr(ParamName, "PerformanceThreshold"))
    {
        if( strstr(ParamName, "SignalFail"))
        {
            pPhyMedia->PerformThreshold.SignalFail  = strtoull(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if( strstr(ParamName, "SignalDegrade"))
        {
            pPhyMedia->PerformThreshold.SignalDegrade = strtoull(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }
    else
    {
        retStatus = Map_hal_dml_alarm(&(pPhyMedia->Alarm), ParamName, pValue);        
    }
    
    if(retStatus == ANSC_STATUS_SUCCESS)
    {
        pPhyMediaData->updated = true;
    }
    else
    {
        AnscTraceError(("%s:%d:: Invalid ParamName[%s] paramValue[%s].\n", __FUNCTION__, __LINE__, ParamName, pValue));
    }
    
    return retStatus;
}

ANSC_STATUS Map_hal_dml_gtc(DML_GTC* gponGtc,char* ParamName, char* pValue)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    char *err;
    
    /* check the parameter name and return the corresponding value */
    if( strstr(ParamName, "CorrectedFecBytes"))
    {
        gponGtc->CorrectedFecBytes = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "UnCorrectedFecCodeWords"))
    {
        gponGtc->UnCorrectedFecCodeWords = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "CorrectedFecCodeWords"))
    {
        gponGtc->CorrectedFecCodeWords = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "TotalFecCodeWords"))
    {
        gponGtc->TotalFecCodeWords = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "HecErrorCount"))
    {
        gponGtc->HecErrorCount = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "PSBdHecErrors"))
    {
        gponGtc->PSBdHecErrors = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "FrameHecErrors"))
    {
        gponGtc->FrameHecErrors = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "FramesLost"))
    {
        gponGtc->FramesLost = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    
    if(retStatus == ANSC_STATUS_FAILURE)
    {
        AnscTraceError(("%s:%d:: Invalid ParamName[%s] paramValue[%s].\n", __FUNCTION__, __LINE__, ParamName, pValue));
    }
    
    return retStatus;
}

ANSC_STATUS Map_hal_dml_ploam(DML_PLOAM* gponPloam, char* ParamName, char* pValue)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    char *err;
    
    if( strstr(ParamName, "OnuId"))
    {
        gponPloam->OnuId = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "VendorId"))
    {
        strncpy(gponPloam->VendorId, pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "SerialNumber"))
    {
        strncpy(gponPloam->SerialNumber, pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "ActivationCounter"))
    {
        gponPloam->ActivationCounter = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "TxMessageCount"))
    {
        gponPloam->TxMessageCount = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "RxMessageCount"))
    {
        gponPloam->RxMessageCount = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "MicErrors"))
    {
        gponPloam->MicErrors = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }    
    else if( strstr(ParamName, "RegistrationState"))
    {
        //O1(0),O2(1),O3(2),O4(3),O5(4),O6(5),O7(6),O8(7),O9(8)
        if(strstr(pValue,"O1"))
        {
            gponPloam->RegistrationState = O1;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"O2"))
        {
            gponPloam->RegistrationState = O2;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"O3"))
        {
            gponPloam->RegistrationState = O3;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"O4"))
        {
            gponPloam->RegistrationState = O4;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"O5"))
        {
            gponPloam->RegistrationState = O5;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"O6"))
        {
            gponPloam->RegistrationState = O6;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"O7"))
        {
            gponPloam->RegistrationState = O7;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"O8"))
        {
            gponPloam->RegistrationState = O8;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(pValue,"O9"))
        {
            gponPloam->RegistrationState = O9;
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }
    else if( strstr(ParamName, "RegistrationTimers"))
    {
        if( strstr(ParamName, "TO1"))
        {
            gponPloam->RegistrationTimers.TO1 = strtoull(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
        if( strstr(ParamName, "TO2"))
        {
            gponPloam->RegistrationTimers.TO2 = strtoull(pValue, &err, 10);
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }
    
    if(retStatus == ANSC_STATUS_FAILURE)
    {
        AnscTraceError(("%s:%d:: Invalid ParamName[%s] paramValue[%s].\n", __FUNCTION__, __LINE__, ParamName, pValue));
    }
    
    return retStatus;
}

ANSC_STATUS Map_hal_dml_omci(DML_OMCI* gponOmci, char* ParamName, char* pValue)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    char *err;
    
    if( strstr(ParamName, "RxBaseLineMessageCountValid"))
    {
        gponOmci->RxBaseLineMessageCountValid = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "RxExtendedMessageCountValid"))
    {
        gponOmci->RxExtendedMessageCountValid = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "MicErrors"))
    {
        gponOmci->MicErrors = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    
    if(retStatus == ANSC_STATUS_FAILURE)
    {
        AnscTraceError(("%s:%d:: Invalid ParamName[%s] paramValue[%s].\n", __FUNCTION__, __LINE__, ParamName, pValue));
    }
    
    return retStatus;
}


ANSC_STATUS Map_hal_dml_gem(DML_GEM_LIST_T* gponGemList,char* ParamName, char* pValue)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    char *err;
    int hal_index = 0;
    int gem_index = 0;

    
    gpon_hal_get_gem_index(ParamName, &hal_index);    
    if(hal_index <= 0)
    {
        AnscTraceError(("%s:%d:: Invalid index ParamName[%s]\n", __FUNCTION__, __LINE__, ParamName));
        return ANSC_STATUS_FAILURE;
    }
    
    //get array index
    gem_index = hal_index - 1;
    
    DML_GEM_CTRL_T* pGemData = gponGemList->pdata[gem_index];

    if(pGemData == NULL)
    {
        //create new Gem
        GponMgrDml_addGem(gponGemList, gem_index);
        
        pGemData = gponGemList->pdata[gem_index];
        if(pGemData == NULL)
        {
            return ANSC_STATUS_FAILURE;
        }        
    }

    DML_GEM* pGem = &(pGemData->dml);
    
    pGem->uInstanceNumber = hal_index;

    if (strstr(ParamName, "PortId"))
    {
        pGem->PortId = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "TrafficType"))
    {
        pGem->TrafficType = strstr(pValue,"Unidirectional")?Unidirectional:Bidirectional;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "TransmittedFrames"))
    {
        pGem->TransmittedFrames = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "ReceivedFrames"))
    {
        pGem->ReceivedFrames = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "Ingress"))
    {
        if (strstr(ParamName, "Tagged"))
        {
            pGem->EthernetFlow.Ingress.Tagged = strstr(pValue,"Single")?Single:Double;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(ParamName, "S-VLAN"))
        {
            if (strstr(ParamName, "Vid"))
            {
                pGem->EthernetFlow.Ingress.SVLAN.Vid = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
            else if (strstr(ParamName, "Pcp"))
            {
                pGem->EthernetFlow.Ingress.SVLAN.Pcp = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
            else if (strstr(ParamName, "Dei"))
            {
                pGem->EthernetFlow.Ingress.SVLAN.Dei = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
        }
        else if (strstr(ParamName, "C-VLAN"))
        {
            if (strstr(ParamName, "Vid"))
            {
                pGem->EthernetFlow.Ingress.CVLAN.Vid = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
            else if (strstr(ParamName, "Pcp"))
            {
                pGem->EthernetFlow.Ingress.CVLAN.Pcp = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
            else if (strstr(ParamName, "Dei"))
            {
                pGem->EthernetFlow.Ingress.CVLAN.Dei = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
        }
    }
    else if (strstr(ParamName, "Egress"))
    {
        if (strstr(ParamName, "Tagged"))
        {
            pGem->EthernetFlow.Egress.Tagged = strstr(pValue,"Single")?Single:Double;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if (strstr(ParamName, "S-VLAN"))
        {
            if (strstr(ParamName, "Vid"))
            {
                pGem->EthernetFlow.Egress.SVLAN.Vid = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
            else if (strstr(ParamName, "Pcp"))
            {
                pGem->EthernetFlow.Egress.SVLAN.Pcp = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
            else if (strstr(ParamName, "Dei"))
            {
                pGem->EthernetFlow.Egress.SVLAN.Dei = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
        }
        else if (strstr(ParamName, "C-VLAN"))
        {
            if (strstr(ParamName, "Vid"))
            {
                pGem->EthernetFlow.Egress.CVLAN.Vid = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
            else if (strstr(ParamName, "Pcp"))
            {
                pGem->EthernetFlow.Egress.CVLAN.Pcp = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
            else if (strstr(ParamName, "Dei"))
            {
                pGem->EthernetFlow.Egress.CVLAN.Dei = strtoull(pValue, &err, 10);
                retStatus = ANSC_STATUS_SUCCESS;
            }
        }
    }
    
    if(retStatus == ANSC_STATUS_SUCCESS)
    {
        pGemData->updated = true;
    }
    else
    {
        AnscTraceError(("%s:%d:: Invalid ParamName[%s] paramValue[%s].\n", __FUNCTION__, __LINE__, ParamName, pValue));
    }

    return retStatus;
}

void parseStrInput_InterDomainName( DML_VEIP* pVeip, char *pValue)
{
    char strInterDomainName[MAX_STR_LEN] = {0};
    char *token, *err;
    
    if(pValue == NULL || pVeip == NULL)
    {
        fprintf(stderr, "Invalid Input\n");
    }

    strncpy(strInterDomainName, pValue, MAX_STR_LEN);

   /* get InterDomainName from Hal */
   token = strtok(strInterDomainName, "#");

   if( token != NULL )
   {
      strcpy(pVeip->InterDomainName, token );
   }

   /* get gress value from Hal */
   if( token != NULL )
   {

       /* parse gress Example Tagged-Vid-Pcp-Dei */
       token = strtok(NULL, "-");

       if (token != NULL)
       {
           if (strstr(token,IEEE_802_1Q_ETHERTYPE_STR_SINGLE))
           {
               pVeip->EthernetFlow.Ingress.Tagged = veip_Single;
               pVeip->EthernetFlow.Egress.Tagged  = veip_Single;
           }
           else if (strstr(token,IEEE_802_1Q_ETHERTYPE_STR_DOUBLE))
           {
               pVeip->EthernetFlow.Ingress.Tagged = veip_Double;
               pVeip->EthernetFlow.Egress.Tagged  = veip_Double;
           }
           else if (strstr(token,IEEE_802_1Q_ETHERTYPE_STR_UNTAGGED))
           {
               pVeip->EthernetFlow.Ingress.Tagged = veip_Untagged;
               pVeip->EthernetFlow.Egress.Tagged  = veip_Untagged;
           }
       }

       token = strtok(NULL, "-");

       if (token != NULL)
       {
           pVeip->EthernetFlow.Ingress.QVLAN.Vid = strtoull(token, &err, 10);
           pVeip->EthernetFlow.Egress.QVLAN.Vid  = strtoull(token, &err, 10);
       }

       token = strtok(NULL, "-");

       if (token != NULL)
       {
           pVeip->EthernetFlow.Ingress.QVLAN.Pcp = strtoull(token, &err, 10);
           pVeip->EthernetFlow.Egress.QVLAN.Pcp  = strtoull(token, &err, 10);
       }

       token = strtok(NULL, "-");

       if (token != NULL)
       {
           pVeip->EthernetFlow.Ingress.QVLAN.Dei = strtoull(token, &err, 10);
           pVeip->EthernetFlow.Egress.QVLAN.Dei  = strtoull(token, &err, 10);
       }
   }
}

ANSC_STATUS Map_hal_dml_veip(DML_VEIP_LIST_T* gponVeipList, char* ParamName, char* pValue)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    char *err;
    int hal_index = 0;
    int veip_index = 0;

    gpon_hal_get_veip_index(ParamName, &hal_index);
    
    if(hal_index <= 0)
    {
        AnscTraceError(("%s:%d:: Invalid index ParamName[%s]\n", __FUNCTION__, __LINE__, ParamName));
        return ANSC_STATUS_FAILURE;
    }
    
    
    //get array index
    veip_index = hal_index - 1;
    
    DML_VEIP_CTRL_T* pVeipData = gponVeipList->pdata[veip_index];

    if(pVeipData == NULL)
    {
        //create new Veip
        GponMgrDml_addVeip(gponVeipList, veip_index);
        
        pVeipData = gponVeipList->pdata[veip_index];
        if(pVeipData == NULL)
        {
            return ANSC_STATUS_FAILURE;
        }        
    }

    DML_VEIP* pVeip = &(pVeipData->dml);
    
    pVeip->uInstanceNumber = hal_index;
    
    if (strstr(ParamName, "MeId"))
    {
        pVeip->MeId = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "AdministrativeState"))
    {
        pVeip->AdministrativeState = strstr(pValue,"Lock")?Lock:Unlock;
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "OperationalState"))
    {
        //Up(0),Down(1),Unknown(2),Dormant(3),NotPresent(4),LowerLayerDown(5),Error(6)
        if(strstr(pValue,"LowerLayerDown"))
        {
            pVeip->OperationalState=veip_LowerLayerDown;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"Down"))
        {
            pVeip->OperationalState = veip_Down;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"Unknown"))
        {
            pVeip->OperationalState = veip_Unknown;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"Dormant"))
        {
            pVeip->OperationalState = veip_Dormant;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"NotPresent"))
        {
            pVeip->OperationalState = veip_NotPresent;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"Error"))
        {
            pVeip->OperationalState = veip_Error;
            retStatus = ANSC_STATUS_SUCCESS;
        }
        else if(strstr(pValue,"Up"))
        {
            pVeip->OperationalState = veip_Up;
            retStatus = ANSC_STATUS_SUCCESS;
        }
    }
    else if (strstr(ParamName, "InterDomainName"))
    {
        parseStrInput_InterDomainName(pVeip, pValue);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "InterfaceName"))
    {
        strncpy(pVeip->InterfaceName,pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if (strstr(ParamName, "EthernetFlow"))
    {
	//handled in parseStrInput_InterDomainName
        retStatus = ANSC_STATUS_SUCCESS;
    }

    
    if(retStatus == ANSC_STATUS_SUCCESS)
    {
        pVeipData->updated = true;
    }
    else
    {
        AnscTraceError(("%s:%d:: Invalid ParamName[%s] paramValue[%s].\n", __FUNCTION__, __LINE__, ParamName, pValue));
    }
    
    return retStatus;
}


ANSC_STATUS Map_hal_dml_tr69(PDML_TR69 gponTr69,char * ParamName, char * pValue)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    char *err;
    
    if( strstr(ParamName, "url"))
    {
        strncpy(gponTr69->url,pValue,strlen(pValue)+1);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    else if( strstr(ParamName, "AssociatedTag"))
    {
        gponTr69->AssociatedTag = strtoull(pValue, &err, 10);
        retStatus = ANSC_STATUS_SUCCESS;
    }
    
    if(retStatus == ANSC_STATUS_FAILURE)
    {
        AnscTraceError(("%s:%d:: Invalid ParamName[%s] paramValue[%s].\n", __FUNCTION__, __LINE__, ParamName, pValue));
    }
    
    return retStatus;
}
