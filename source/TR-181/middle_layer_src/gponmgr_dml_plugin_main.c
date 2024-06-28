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

/***********************************************************************

    module: gponmgr_dml_plugin_main.c

        Implement COSA Data Model Library Init and Unload apis.

    ---------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    ---------------------------------------------------------------

    revision:

        09/12/2019    initial revision.

**********************************************************************/

#include "ansc_platform.h"
#include "ansc_load_library.h"
#include "cosa_plugin_api.h"
#include "gponmgr_dml_plugin_main.h"
#include "gponmgr_dml_backendmgr.h"
#include "gponmgr_dml_func.h"

PBACKEND_MANAGER_OBJECT                   g_pBEManager;
void *                                    g_pDslhDmlAgent;
extern ANSC_HANDLE                        g_MessageBusHandle_Irep;
extern char                               g_SubSysPrefix_Irep[32];
extern COSARepopulateTableProc            g_COSARepopulateTable;

#define THIS_PLUGIN_VERSION                         1

int ANSC_EXPORT_API GponMgrDml_Init(ULONG uMaxVersionSupported, void* hCosaPlugInfo)
{
    PCOSA_PLUGIN_INFO               pPlugInfo                   = (PCOSA_PLUGIN_INFO                 )hCosaPlugInfo;
    COSAGetParamValueByPathNameProc pGetParamValueByPathNameProc = (COSAGetParamValueByPathNameProc)NULL;
    COSASetParamValueByPathNameProc pSetParamValueByPathNameProc = (COSASetParamValueByPathNameProc)NULL;
    COSAGetParamValueStringProc     pGetStringProc              = (COSAGetParamValueStringProc       )NULL;
    COSAGetParamValueUlongProc      pGetParamValueUlongProc     = (COSAGetParamValueUlongProc        )NULL;
    COSAGetParamValueIntProc        pGetParamValueIntProc       = (COSAGetParamValueIntProc          )NULL;
    COSAGetParamValueBoolProc       pGetParamValueBoolProc      = (COSAGetParamValueBoolProc         )NULL;
    COSASetParamValueStringProc     pSetStringProc              = (COSASetParamValueStringProc       )NULL;
    COSASetParamValueUlongProc      pSetParamValueUlongProc     = (COSASetParamValueUlongProc        )NULL;
    COSASetParamValueIntProc        pSetParamValueIntProc       = (COSASetParamValueIntProc          )NULL;
    COSASetParamValueBoolProc       pSetParamValueBoolProc      = (COSASetParamValueBoolProc         )NULL;
    COSAGetInstanceNumbersProc      pGetInstanceNumbersProc     = (COSAGetInstanceNumbersProc        )NULL;

    COSAValidateHierarchyInterfaceProc
                                    pValInterfaceProc           = (COSAValidateHierarchyInterfaceProc)NULL;
    COSAGetHandleProc               pGetRegistryRootFolder      = (COSAGetHandleProc                 )NULL;
    COSAGetInstanceNumberByIndexProc
                                    pGetInsNumberByIndexProc    = (COSAGetInstanceNumberByIndexProc  )NULL;
    COSAGetInterfaceByNameProc      pGetInterfaceByNameProc     = (COSAGetInterfaceByNameProc        )NULL;

    if ( uMaxVersionSupported < THIS_PLUGIN_VERSION )
    {
      /* this version is not supported */
        return -1;
    }

    pPlugInfo->uPluginVersion       = THIS_PLUGIN_VERSION;
    g_pDslhDmlAgent                 = pPlugInfo->hDmlAgent;

    pGetParamValueByPathNameProc = (COSAGetParamValueByPathNameProc)pPlugInfo->AcquireFunction("COSAGetParamValueByPathName");

    if( pGetParamValueByPathNameProc != NULL)
    {
        g_GetParamValueByPathNameProc = pGetParamValueByPathNameProc;
    }
    else
    {
        goto EXIT;
    }

    pSetParamValueByPathNameProc = (COSASetParamValueByPathNameProc)pPlugInfo->AcquireFunction("COSASetParamValueByPathName");

    if( pSetParamValueByPathNameProc != NULL)
    {
        g_SetParamValueByPathNameProc = pSetParamValueByPathNameProc;
    }
    else
    {
        goto EXIT;
    }

    pGetStringProc = (COSAGetParamValueStringProc)pPlugInfo->AcquireFunction("COSAGetParamValueString");

    if( pGetStringProc != NULL)
    {
        g_GetParamValueString = pGetStringProc;
    }
    else
    {
        goto EXIT;
    }

    pGetParamValueUlongProc = (COSAGetParamValueUlongProc)pPlugInfo->AcquireFunction("COSAGetParamValueUlong");

    if( pGetParamValueUlongProc != NULL)
    {
        g_GetParamValueUlong = pGetParamValueUlongProc;
    }
    else
    {
        goto EXIT;
    }


    pGetParamValueIntProc = (COSAGetParamValueIntProc)pPlugInfo->AcquireFunction("COSAGetParamValueInt");

    if( pGetParamValueIntProc != NULL)
    {
        g_GetParamValueInt = pGetParamValueIntProc;
    }
    else
    {
        goto EXIT;
    }

    pGetParamValueBoolProc = (COSAGetParamValueBoolProc)pPlugInfo->AcquireFunction("COSAGetParamValueBool");

    if( pGetParamValueBoolProc != NULL)
    {
        g_GetParamValueBool = pGetParamValueBoolProc;
    }
    else
    {
        goto EXIT;
    }

    pSetStringProc = (COSASetParamValueStringProc)pPlugInfo->AcquireFunction("COSASetParamValueString");

    if( pSetStringProc != NULL)
    {
        g_SetParamValueString = pSetStringProc;
    }
    else
    {
        goto EXIT;
    }

    pSetParamValueUlongProc = (COSASetParamValueUlongProc)pPlugInfo->AcquireFunction("COSASetParamValueUlong");

    if( pSetParamValueUlongProc != NULL)
    {
        g_SetParamValueUlong = pSetParamValueUlongProc;
    }
    else
    {
        goto EXIT;
    }


    pSetParamValueIntProc = (COSASetParamValueIntProc)pPlugInfo->AcquireFunction("COSASetParamValueInt");

    if( pSetParamValueIntProc != NULL)
    {
        g_SetParamValueInt = pSetParamValueIntProc;
    }
    else
    {
        goto EXIT;
    }

    pSetParamValueBoolProc = (COSASetParamValueBoolProc)pPlugInfo->AcquireFunction("COSASetParamValueBool");

    if( pSetParamValueBoolProc != NULL)
    {
        g_SetParamValueBool = pSetParamValueBoolProc;
    }
    else
    {
        goto EXIT;
    }

    pGetInstanceNumbersProc = (COSAGetInstanceNumbersProc)pPlugInfo->AcquireFunction("COSAGetInstanceNumbers");

    if( pGetInstanceNumbersProc != NULL)
    {
        g_GetInstanceNumbers = pGetInstanceNumbersProc;
    }
    else
    {
        goto EXIT;
    }

    pValInterfaceProc = (COSAValidateHierarchyInterfaceProc)pPlugInfo->AcquireFunction("COSAValidateHierarchyInterface");

    if ( pValInterfaceProc )
    {
        g_ValidateInterface = pValInterfaceProc;
    }
    else
    {
        goto EXIT;
    }
/*
#ifndef _ANSC_WINDOWSNT
#ifdef _SOFTWAREMODULES_SUPPORT_NAF
    CosaSoftwareModulesInit(hCosaPlugInfo);
#endif
#endif
*/
    pGetRegistryRootFolder = (COSAGetHandleProc)pPlugInfo->AcquireFunction("COSAGetRegistryRootFolder");

    if ( pGetRegistryRootFolder != NULL )
    {
        g_GetRegistryRootFolder = pGetRegistryRootFolder;
    }
    else
    {
        printf("!!! haha, catcha !!!\n");
        goto EXIT;
    }

    pGetInsNumberByIndexProc = (COSAGetInstanceNumberByIndexProc)pPlugInfo->AcquireFunction("COSAGetInstanceNumberByIndex");

    if ( pGetInsNumberByIndexProc != NULL )
    {
        g_GetInstanceNumberByIndex = pGetInsNumberByIndexProc;
    }
    else
    {
        goto EXIT;
    }

    pGetInterfaceByNameProc = (COSAGetInterfaceByNameProc)pPlugInfo->AcquireFunction("COSAGetInterfaceByName");

    if ( pGetInterfaceByNameProc != NULL )
    {
        g_GetInterfaceByName = pGetInterfaceByNameProc;
    }
    else
    {
        goto EXIT;
    }

    g_pPnmCcdIf = g_GetInterfaceByName(g_pDslhDmlAgent, CCSP_CCD_INTERFACE_NAME);

    if ( !g_pPnmCcdIf )
    {
        CcspTraceError(("g_pPnmCcdIf is NULL !\n"));

        goto EXIT;
    }

    g_RegisterCallBackAfterInitDml = (COSARegisterCallBackAfterInitDmlProc)pPlugInfo->AcquireFunction("COSARegisterCallBackAfterInitDml");

    if ( !g_RegisterCallBackAfterInitDml )
    {
        goto EXIT;
    }

    g_COSARepopulateTable = (COSARepopulateTableProc)pPlugInfo->AcquireFunction("COSARepopulateTable");

    if ( !g_COSARepopulateTable )
    {
        goto EXIT;
    }

    /* Get Message Bus Handle */
    //g_GetMessageBusHandle = (PFN_CCSPCCDM_APPLY_CHANGES)pPlugInfo->AcquireFunction("COSAGetMessageBusHandle");
    g_GetMessageBusHandle = (COSAGetHandleProc)pPlugInfo->AcquireFunction("COSAGetMessageBusHandle");
    if ( g_GetMessageBusHandle == NULL )
    {
        goto EXIT;
    }

    g_MessageBusHandle = (ANSC_HANDLE)g_GetMessageBusHandle(g_pDslhDmlAgent);
    if ( g_MessageBusHandle == NULL )
    {
        goto EXIT;
    }
    g_MessageBusHandle_Irep = g_MessageBusHandle;

    /* Get Subsystem prefix */
    g_GetSubsystemPrefix = (COSAGetSubsystemPrefixProc)pPlugInfo->AcquireFunction("COSAGetSubsystemPrefix");
    if ( g_GetSubsystemPrefix != NULL )
    {
        char*   tmpSubsystemPrefix;

        if ( (tmpSubsystemPrefix = g_GetSubsystemPrefix(g_pDslhDmlAgent)) )
        {
            AnscCopyString(g_SubSysPrefix_Irep, tmpSubsystemPrefix);
        }

        /* retrieve the subsystem prefix */
        g_SubsystemPrefix = g_GetSubsystemPrefix(g_pDslhDmlAgent);
    }

    /* register the back-end apis for the data model */
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_Synchronize", GponPhy_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_IsUpdated", GponPhy_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_GetEntryCount", GponPhy_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_GetEntry", GponPhy_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_Commit", GponPhy_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_Validate", GponPhy_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_Rollback", GponPhy_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_GetParamStringValue", GponPhy_GetParamStringValue);
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_SetParamStringValue", GponPhy_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_GetParamBoolValue", GponPhy_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_SetParamBoolValue", GponPhy_SetParamBoolValue);
#endif
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhy_GetParamUlongValue", GponPhy_GetParamUlongValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhyRxpwr_GetParamIntValue", GponPhyRxpwr_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhyRxpwr_SetParamIntValue", GponPhyRxpwr_SetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhyTxpwr_GetParamIntValue", GponPhyTxpwr_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhyTxpwr_SetParamIntValue", GponPhyTxpwr_SetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhyVoltage_GetParamIntValue", GponPhyVoltage_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhyBias_GetParamUlongValue", GponPhyBias_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhyTemperature_GetParamIntValue", GponPhyTemperature_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhyPerformanceThreshold_GetParamUlongValue", GponPhyPerformanceThreshold_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPhyAlarm_GetParamUlongValue", GponPhyAlarm_GetParamUlongValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGtc_GetParamUlongValue", GponGtc_GetParamUlongValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPloam_GetParamUlongValue", GponPloam_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPloam_GetParamStringValue", GponPloam_GetParamStringValue);
#if !defined(WAN_MANAGER_UNIFICATION_ENABLED)
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponPloamRegTmr_GetParamUlongValue", GponPloamRegTmr_GetParamUlongValue);
#endif

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGem_Synchronize", GponGem_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGem_IsUpdated", GponGem_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGem_GetEntryCount", GponGem_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGem_GetEntry", GponGem_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGem_GetParamUlongValue", GponGem_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGemEthFlowIngress_GetParamUlongValue", GponGemEthFlowIngress_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGemEthFlowIngressSvlan_GetParamUlongValue", GponGemEthFlowIngressSvlan_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGemEthFlowIngressCvlan_GetParamUlongValue", GponGemEthFlowIngressCvlan_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGemEthFlowEgress_GetParamUlongValue", GponGemEthFlowEgress_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGemEthFlowEgressSvlan_GetParamUlongValue", GponGemEthFlowEgressSvlan_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGemEthFlowEgressCvlan_GetParamUlongValue", GponGemEthFlowEgressCvlan_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGem_Commit", GponGem_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGem_Validate", GponGem_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponGem_Rollback", GponGem_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponOmci_GetParamIntValue", GponOmci_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponOmci_GetParamUlongValue", GponOmci_GetParamUlongValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeip_Synchronize", GponVeip_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeip_IsUpdated", GponVeip_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeip_GetEntryCount", GponVeip_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeip_GetEntry", GponVeip_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeip_GetParamStringValue", GponVeip_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeip_GetParamUlongValue", GponVeip_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeipEthFlowIngress_GetParamUlongValue", GponVeipEthFlowIngress_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeipEthFlowIngress_SetParamUlongValue", GponVeipEthFlowIngress_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeipEthFlowIngressQlan_GetParamUlongValue", GponVeipEthFlowIngressQlan_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeipEthFlowIngressQlan_SetParamUlongValue", GponVeipEthFlowIngressQlan_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeipEthFlowEgress_GetParamUlongValue", GponVeipEthFlowEgress_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeipEthFlowEgress_SetParamUlongValue", GponVeipEthFlowEgress_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeipEthFlowEgressQlan_GetParamUlongValue", GponVeipEthFlowEgressQlan_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeipEthFlowEgressQlan_SetParamUlongValue", GponVeipEthFlowEgressQlan_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeip_Commit", GponVeip_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeip_Validate", GponVeip_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "GponVeip_Rollback", GponVeip_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Gpontr69_GetParamStringValue", Gpontr69_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Gpontr69_GetParamUlongValue", Gpontr69_GetParamUlongValue);
    /* Create backend framework */
    g_pBEManager = (PBACKEND_MANAGER_OBJECT)GponMgrDml_BackEndManagerCreate();

    if ( g_pBEManager && g_pBEManager->Initialize )
    {
        g_pBEManager->hCosaPluginInfo = pPlugInfo;

        g_pBEManager->Initialize   ((ANSC_HANDLE)g_pBEManager);
    }

    return  0;

EXIT:

    return -1;
}
