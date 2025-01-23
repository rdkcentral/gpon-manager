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


#include "gponmgr_dml_data.h"


/******** GPON DML DATABASE ********/
static GPON_DML_DATA* pGponDmlData = NULL;


/******** FUNCTIONS ********/
GPON_DML_DATA* GponMgrDml_GetData_locked(void)
{
    GPON_DML_DATA* pDmlData = GponMgrDml_GetData();

    if(pDmlData != NULL)
    {
        if(pthread_mutex_lock(&(pDmlData->mDataMutex)) == 0)
        {
            return pDmlData;
        }
    }

    return NULL;
}

void GponMgrDml_GetData_release(GPON_DML_DATA* pDmlData)
{
    if(pDmlData != NULL)
    {
        pthread_mutex_unlock (&(pDmlData->mDataMutex));
    }
}


GPON_DML_DATA* GponMgrDml_GetData(void)
{
    if(pGponDmlData == NULL)
    {
        //create instance
        pGponDmlData = (GPON_DML_DATA*) AnscAllocateMemory(sizeof(GPON_DML_DATA));
    }

    return pGponDmlData;
}



ANSC_STATUS GponMgrDml_DataInit(void)
{
    ANSC_STATUS             result  = ANSC_STATUS_FAILURE;  
    GPON_DML_DATA*          pGponData = NULL;
    pthread_mutexattr_t     muttex_attr;
    
    pGponData = GponMgrDml_GetData();
    if(pGponData != NULL)
    {
        //default
        GponMgrDml_SetDefaultData(pGponData);

        //Initialise mutex
        pthread_mutexattr_init(&muttex_attr);
        pthread_mutexattr_settype(&muttex_attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&(pGponData->mDataMutex), &(muttex_attr));  
        
        result = ANSC_STATUS_SUCCESS;
    }

    return result;
}


ANSC_STATUS GponMgrDml_DataDelete(void)
{
    ANSC_STATUS     result  = ANSC_STATUS_FAILURE;    
    GPON_DML_DATA*  pGponData = NULL;
    int idx;


    pGponData = GponMgrDml_GetData_locked();
    if(pGponData != NULL)
    {
        //delete arrays
        for(idx = 0; idx < GPON_DATA_MAX_PM; idx++)
        {
            if(pGponData->gpon.PhysicalMedia.pdata[idx] != NULL)
            {
                AnscFreeMemory((ANSC_HANDLE)pGponData->gpon.PhysicalMedia.pdata[idx]);
                pGponData->gpon.PhysicalMedia.pdata[idx] = NULL;            
            }
        }
        
        for(idx = 0; idx < GPON_DATA_MAX_GEM; idx++)
        {
            if(pGponData->gpon.Gem.pdata[idx] != NULL)
            {
                AnscFreeMemory((ANSC_HANDLE)pGponData->gpon.Gem.pdata[idx]);
                pGponData->gpon.Gem.pdata[idx] = NULL;            
            }
        }
        
        for(idx = 0; idx < GPON_DATA_MAX_VEIP; idx++)
        {
            if(pGponData->gpon.Veip.pdata[idx] != NULL)
            {
                AnscFreeMemory((ANSC_HANDLE)pGponData->gpon.Veip.pdata[idx]);
                pGponData->gpon.Veip.pdata[idx] = NULL;            
            }
        }
        
        GponMgrDml_GetData_release(pGponData);
        
        //destroy the mutex        
        pthread_mutex_destroy(&(pGponData->mDataMutex));        

        //delete data
        AnscFreeMemory((ANSC_HANDLE)pGponDmlData);
        pGponDmlData = NULL;
        
        result = ANSC_STATUS_SUCCESS;
    }

    return result;
}

//Default values
void GponMgrDml_SetDefaultPhyMedia(DML_PHY_MEDIA_CTRL_T* gponPhyMediaData)
{
    if(gponPhyMediaData != NULL)
    {
        gponPhyMediaData->updated = false;
        
        DML_PHY_MEDIA* gponPhyMedia = &(gponPhyMediaData->dml);        
        
        gponPhyMedia->uInstanceNumber = 0;
        gponPhyMedia->Cage = BoB;
        memset(gponPhyMedia->ModuleVendor, 0, 256);
        memset(gponPhyMedia->ModuleName, 0, 256);
        memset(gponPhyMedia->ModuleVersion, 0, 256);
        memset(gponPhyMedia->ModuleFirmwareVersion, 0, 256);
        gponPhyMedia->PonMode = GPON;
        gponPhyMedia->Connector = LC;
        gponPhyMedia->NominalBitRateDownstream = 0;
        gponPhyMedia->NominalBitRateUpstream = 0;
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
        gponPhyMedia->Enable = false;
#endif
        gponPhyMedia->Status = NotPresent;
        gponPhyMedia->RedundancyState = Active;
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
        memset(gponPhyMedia->Alias, 0, 256);
        gponPhyMedia->LastChange = 0;
        memset(gponPhyMedia->LowerLayers, 0, 256);
        gponPhyMedia->Upstream = false;
#endif
        
        gponPhyMedia->PerformThreshold.SignalFail = 0;
        gponPhyMedia->PerformThreshold.SignalDegrade = 0;
        
        gponPhyMedia->RxPower.SignalLevel = 0;
        gponPhyMedia->RxPower.SignalLevelLowerThreshold = 0;
        gponPhyMedia->RxPower.SignalLevelUpperThreshold = 0;
        
        gponPhyMedia->TxPower.SignalLevel = 0;
        gponPhyMedia->TxPower.SignalLevelLowerThreshold = 0;
        gponPhyMedia->TxPower.SignalLevelUpperThreshold = 0;
        
        gponPhyMedia->Voltage.VoltageLevel = 0;
        
        gponPhyMedia->Bias.CurrentBias = 0;
        
        gponPhyMedia->Temperature.CurrentTemp = 0;
        
        gponPhyMedia->Alarm.RDI = INACTIVE;
        gponPhyMedia->Alarm.PEE = INACTIVE;
        gponPhyMedia->Alarm.LOS = INACTIVE;
        gponPhyMedia->Alarm.LOF = INACTIVE;
        gponPhyMedia->Alarm.DACT = INACTIVE;
        gponPhyMedia->Alarm.DIS = INACTIVE;
        gponPhyMedia->Alarm.MIS = INACTIVE;
        gponPhyMedia->Alarm.MEM = INACTIVE;
        gponPhyMedia->Alarm.SUF = INACTIVE;
        gponPhyMedia->Alarm.SF = INACTIVE;
        gponPhyMedia->Alarm.SD = INACTIVE;
        gponPhyMedia->Alarm.LCDG = INACTIVE;
        gponPhyMedia->Alarm.TF = INACTIVE;
        gponPhyMedia->Alarm.ROGUE = INACTIVE;        
    }
    else
    {
        CcspTraceWarning(("%s %d - Failed to set Default Values for Physical media\n", __FUNCTION__, __LINE__));
    }
}

void GponMgrDml_SetDefaultGtc(DML_GTC* gponGtcData)
{
    if(gponGtcData != NULL)
    {
        gponGtcData->CorrectedFecBytes = 0;
        gponGtcData->CorrectedFecCodeWords = 0;
        gponGtcData->UnCorrectedFecCodeWords = 0;
        gponGtcData->TotalFecCodeWords = 0;
        gponGtcData->HecErrorCount = 0;
        gponGtcData->PSBdHecErrors = 0;
        gponGtcData->FrameHecErrors = 0;
        gponGtcData->FramesLost = 0;
        gponGtcData->LastFetchedTime = 0;
    }    
    else
    {
        CcspTraceWarning(("%s %d - Failed to set Default values for GTC\n", __FUNCTION__, __LINE__));
    }
}

void GponMgrDml_SetDefaultPloam(DML_PLOAM* gponPloamData)
{
    if(gponPloamData != NULL)
    {
        gponPloamData->OnuId = 0;
        memset(gponPloamData->VendorId, 0, 128);
        memset(gponPloamData->SerialNumber, 0, 128);
        gponPloamData->RegistrationState = O1;
        gponPloamData->ActivationCounter = 0;
        gponPloamData->TxMessageCount = 0;
        gponPloamData->RxMessageCount = 0;
        gponPloamData->MicErrors = 0;
        gponPloamData->RegistrationTimers.TO1 = 0;
        gponPloamData->RegistrationTimers.TO2 = 0;
        gponPloamData->LastFetchedTime = 0;
    }    
    else
    {
        CcspTraceWarning(("%s %d - Failed to set default values for Ploam\n", __FUNCTION__, __LINE__));
    }
}

void GponMgrDml_SetDefaultOmci(DML_OMCI* gponOmciData)
{
    if(gponOmciData != NULL)
    {
        gponOmciData->RxBaseLineMessageCountValid = 0;
        gponOmciData->RxExtendedMessageCountValid = 0;
        gponOmciData->MicErrors = 0;
        gponOmciData->LastFetchedTime = 0;
    }    
    else
    {
        CcspTraceWarning(("%s %d - Failed to set Default values for Omci\n", __FUNCTION__, __LINE__));
    }
}

void GponMgrDml_SetDefaultGem(DML_GEM_CTRL_T* gponGemData)
{
    if(gponGemData != NULL)
    {
        gponGemData->updated = false;
        
        DML_GEM* gponGem = &(gponGemData->dml); 
        
        gponGem->uInstanceNumber = 0;
        gponGem->PortId = 0;
        gponGem->TrafficType = Unidirectional;
        gponGem->TransmittedFrames = 0;
        gponGem->ReceivedFrames = 0;

        gponGem->EthernetFlow.Ingress.Tagged = Single;
        gponGem->EthernetFlow.Ingress.SVLAN.Vid = 0;
        gponGem->EthernetFlow.Ingress.SVLAN.Pcp = 0;
        gponGem->EthernetFlow.Ingress.SVLAN.Dei = 0;
        gponGem->EthernetFlow.Ingress.CVLAN.Vid = 0;
        gponGem->EthernetFlow.Ingress.CVLAN.Pcp = 0;
        gponGem->EthernetFlow.Ingress.CVLAN.Dei = 0;

        gponGem->EthernetFlow.Egress.Tagged = Single;
        gponGem->EthernetFlow.Egress.SVLAN.Vid = 0;
        gponGem->EthernetFlow.Egress.SVLAN.Pcp = 0;
        gponGem->EthernetFlow.Egress.SVLAN.Dei = 0;
        gponGem->EthernetFlow.Egress.CVLAN.Vid = 0;
        gponGem->EthernetFlow.Egress.CVLAN.Pcp = 0;
        gponGem->EthernetFlow.Egress.CVLAN.Dei = 0;
        
    }
    else
    {
        CcspTraceWarning(("%s %d - Failed to set Default values for Gem\n", __FUNCTION__, __LINE__));
    }    
}

void GponMgrDml_SetDefaultVeip(DML_VEIP_CTRL_T* gponVeipData)
{
    if(gponVeipData != NULL)
    {
        gponVeipData->updated = false;
        gponVeipData->sm_created = false;
        
        DML_VEIP* gponVeip = &(gponVeipData->dml);
        
        gponVeip->uInstanceNumber = 0;
        gponVeip->MeId = 0;
        gponVeip->AdministrativeState = Lock;
        gponVeip->OperationalState = veip_Unknown;
        memset(gponVeip->InterDomainName, 0, 25);
        memset(gponVeip->InterfaceName, 0, 256);
        
        gponVeip->EthernetFlow.Ingress.Tagged = veip_Untagged;
        gponVeip->EthernetFlow.Ingress.QVLAN.Vid = 0;
        gponVeip->EthernetFlow.Ingress.QVLAN.Pcp = 0;
        gponVeip->EthernetFlow.Ingress.QVLAN.Dei = 0;
        gponVeip->EthernetFlow.Egress.Tagged = veip_Untagged;
        gponVeip->EthernetFlow.Egress.QVLAN.Vid = 0;
        gponVeip->EthernetFlow.Egress.QVLAN.Pcp = 0;
        gponVeip->EthernetFlow.Egress.QVLAN.Dei = 0;
    }
    else
    {
        CcspTraceWarning(("%s %d - Failed to set default values for VEIP\n", __FUNCTION__, __LINE__));
    }
}

void GponMgrDml_SetDefaultTr69(DML_TR69* gponTr69Data)
{
    if(gponTr69Data != NULL)
    {
        memset(gponTr69Data->url, 0, 256);
        gponTr69Data->AssociatedTag = 0;
        gponTr69Data->LastFetchedTime = 0;
    }    
    else
    {
        CcspTraceWarning(("%s %d - Failed to set Default values for TR69\n", __FUNCTION__, __LINE__));
    }
}


void GponMgrDml_SetDefaultData(GPON_DML_DATA* pGponData)
{
    int idx;
    
    if(pGponData != NULL)
    {
        pGponData->gpon.PhysicalMedia.ulQuantity = 0;
        for(idx = 0; idx < GPON_DATA_MAX_PM; idx++)
        {
            pGponData->gpon.PhysicalMedia.pdata[idx] = NULL;
        }
        
        GponMgrDml_SetDefaultGtc(&( pGponData->gpon.Gtc));
        
        GponMgrDml_SetDefaultPloam(&( pGponData->gpon.Ploam));
        
        GponMgrDml_SetDefaultOmci(&( pGponData->gpon.Omci));
        
        pGponData->gpon.Gem.ulQuantity = 0;
        for(idx = 0; idx < GPON_DATA_MAX_GEM; idx++)
        {
            pGponData->gpon.Gem.pdata[idx] = NULL;
        }
        
        pGponData->gpon.Veip.ulQuantity = 0;
        for(idx = 0; idx < GPON_DATA_MAX_VEIP; idx++)
        {
            pGponData->gpon.Veip.pdata[idx] = NULL;
        }
        
        GponMgrDml_SetDefaultTr69(&( pGponData->gpon.Tr69));
    }
    else
    {
        CcspTraceWarning(("%s %d - Failed to set Default Data\n", __FUNCTION__, __LINE__));
    }
}




ANSC_STATUS GponMgrDml_addPhysicalMedia(DML_PHY_MEDIA_LIST_T* gponPhyList, int pm_index)
{
    ANSC_STATUS     result  = ANSC_STATUS_FAILURE;
    
    if(gponPhyList != NULL)
    {
        if(pm_index < GPON_DATA_MAX_PM)
        {
            //delete old
            if(gponPhyList->pdata[pm_index] != NULL)
            {
                AnscFreeMemory((ANSC_HANDLE) gponPhyList->pdata[pm_index]);
                gponPhyList->pdata[pm_index] = NULL;
                gponPhyList->ulQuantity--;
            }
            
            //create new
            gponPhyList->pdata[pm_index] = (DML_PHY_MEDIA_CTRL_T*) AnscAllocateMemory(sizeof(DML_PHY_MEDIA_CTRL_T));
            
            //set default
            if(gponPhyList->pdata[pm_index] != NULL)
            {
                GponMgrDml_SetDefaultPhyMedia(gponPhyList->pdata[pm_index]);
                
                gponPhyList->ulQuantity++;
            }            
        }        
    }    
   
    return result; 
    
    
}

ANSC_STATUS GponMgrDml_addGem(DML_GEM_LIST_T* gponGemList, int gem_index)
{
    ANSC_STATUS     result  = ANSC_STATUS_FAILURE;
    
    if(gponGemList != NULL)
    {
        if(gem_index < GPON_DATA_MAX_GEM)
        {
            //delete old
            if(gponGemList->pdata[gem_index] != NULL)
            {
                AnscFreeMemory((ANSC_HANDLE) gponGemList->pdata[gem_index]);
                gponGemList->pdata[gem_index] = NULL;
                gponGemList->ulQuantity--;
            }
            
            //create new
            gponGemList->pdata[gem_index] = (DML_GEM_CTRL_T*) AnscAllocateMemory(sizeof(DML_GEM_CTRL_T));
            
            //set default
            if(gponGemList->pdata[gem_index] != NULL)
            {
                GponMgrDml_SetDefaultGem(gponGemList->pdata[gem_index]);
                
                gponGemList->ulQuantity++;
            }            
        }        
    }    
   
    return result;  
}

ANSC_STATUS GponMgrDml_addVeip(DML_VEIP_LIST_T* gponVeipList, int veip_index)
{
    ANSC_STATUS     result  = ANSC_STATUS_FAILURE;
    
    if(gponVeipList != NULL)
    {
        if(veip_index < GPON_DATA_MAX_VEIP)
        {
            //delete old
            if(gponVeipList->pdata[veip_index] != NULL)
            {
                AnscFreeMemory((ANSC_HANDLE) gponVeipList->pdata[veip_index]);
                gponVeipList->pdata[veip_index] = NULL;
                gponVeipList->ulQuantity--;
            }
            
            //create new
            gponVeipList->pdata[veip_index] = (DML_VEIP_CTRL_T*) AnscAllocateMemory(sizeof(DML_VEIP_CTRL_T));
            
            //set default
            if(gponVeipList->pdata[veip_index] != NULL)
            {
                GponMgrDml_SetDefaultVeip(gponVeipList->pdata[veip_index]);
                
                gponVeipList->ulQuantity++;
            }            
        }        
    }    
   
    return result; 
}

