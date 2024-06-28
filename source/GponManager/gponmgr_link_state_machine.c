/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "gponmgr_dml_data.h"
#include "gponmgr_dml_hal.h"
#include "gponmgr_link_state_machine.h"
#include "gponmgr_dml_eth_iface.h"


#define GPON_LINK_SM_LOOP_TIMEOUT 500000 // timeout in milliseconds. This is the state machine loop interval


///////////////////////////////////////////////////
///////////////////  ACTIONS    ///////////////////
///////////////////////////////////////////////////
ANSC_STATUS gpon_controller_init(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    gpon_sm_ctrl->veip_hal_index = -1;
    gpon_sm_ctrl->veip_eth_instance = -1;
    gpon_sm_ctrl->veip_lower_layer[0] = 0;
    gpon_sm_ctrl->sm_running = FALSE;
    gpon_sm_ctrl->sm_state = GSM_LINK_DOWN;
    gpon_sm_ctrl->pGponData = NULL;

    return ANSC_STATUS_SUCCESS;
}

bool check_gpon_physical_media_alarm_los(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    int pm_idx = 0;
    if(gpon_sm_ctrl->pGponData != NULL)
    {
        for(pm_idx = 0; pm_idx < gpon_sm_ctrl->pGponData->gpon.PhysicalMedia.ulQuantity; pm_idx++)
        {

            DML_PHY_MEDIA_CTRL_T* pGponPMData = gpon_sm_ctrl->pGponData->gpon.PhysicalMedia.pdata[pm_idx];

            //check Physical Media
            if(pGponPMData != NULL)
            {
                DML_PHY_MEDIA* pGponPhy = &(pGponPMData->dml);

                //check Veip Administrative State
                if(pGponPhy->RedundancyState == Active)
                {
                    if(pGponPhy->Alarm.LOS == ACTIVE)
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

ANSC_STATUS check_gpon_veip_interface_enabled(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    if(gpon_sm_ctrl->pGponData != NULL)
    {
        DML_VEIP_CTRL_T* pGponVeipData = gpon_sm_ctrl->pGponData->gpon.Veip.pdata[gpon_sm_ctrl->veip_hal_index -1];

        //check Veip interface
        if(pGponVeipData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponVeipData->dml);

            if(pGponVeip->AdministrativeState == Unlock)
            {
                return ANSC_STATUS_SUCCESS;
            }
        }
    }

    return ANSC_STATUS_FAILURE;
}

ANSC_STATUS check_gpon_veip_interface_up(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    bool veip_up = false;
    bool pm_los = false;

    if(gpon_sm_ctrl->pGponData != NULL)
    {
        DML_VEIP_CTRL_T* pGponVeipData = gpon_sm_ctrl->pGponData->gpon.Veip.pdata[gpon_sm_ctrl->veip_hal_index -1];

        //check Veip interface
        if(pGponVeipData != NULL)
        {
            DML_VEIP* pGponVeip = &(pGponVeipData->dml);

            //check Veip Operational State is UP
            if(pGponVeip->OperationalState == veip_Up)
            {
                veip_up = true;
            }
        }
    }

    pm_los = check_gpon_physical_media_alarm_los(gpon_sm_ctrl);

    if((veip_up == true) && (pm_los == false))
    {
        return ANSC_STATUS_SUCCESS;
    }


    return ANSC_STATUS_FAILURE;
}


ANSC_STATUS gpon_config_veip_interface(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
    int veip_index;

    //update veip inteface
    if( gpon_sm_ctrl != NULL && gpon_sm_ctrl->pGponData != NULL)
    {
        ret = GponHal_get_veip(&(gpon_sm_ctrl->pGponData->gpon.Veip));
        if(ret == ANSC_STATUS_SUCCESS)
        {
            //Set parameters
            veip_index = gpon_sm_ctrl->veip_hal_index - 1;
            sprintf(gpon_sm_ctrl->veip_lower_layer, GPON_HAL_VEIP_NODE, gpon_sm_ctrl->veip_hal_index);

#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
            ret =  CosaDmlGponSetPhyStatusForWanManager(veip_index,gpon_sm_ctrl->veip_lower_layer,INTERFACE_UP);
#else
            //add GW interface
            ret = Gponmgr_eth_addInterface(veip_index, gpon_sm_ctrl->veip_lower_layer, &(gpon_sm_ctrl->veip_eth_instance));

            //Enable GW interface 
            Gponmgr_eth_setEnableInterface(gpon_sm_ctrl->veip_eth_instance, TRUE);
#endif

        }
    }


    return ret;
}


ANSC_STATUS gpon_disable_veip_interface(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    if( gpon_sm_ctrl != NULL )
    {
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
        int veip_index = gpon_sm_ctrl->veip_hal_index - 1;
        ret = CosaDmlGponSetPhyStatusForWanManager(veip_index, gpon_sm_ctrl->veip_lower_layer, INTERFACE_DOWN);
#else
        ret = Gponmgr_eth_setEnableInterface(gpon_sm_ctrl->veip_eth_instance, FALSE);
#endif
    }

    return ret;
}



///////////////////////////////////////////////////
/////////////////  TRANSITIONS    /////////////////
///////////////////////////////////////////////////
GPON_LINK_STATE_MACHINE_T gpon_sm_transition_Start(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    gpon_sm_ctrl->sm_running = true;

    CcspTraceInfo(("%s %d - Start Gpon state machine\n", __FUNCTION__, __LINE__));

    return GSM_LINK_DOWN;
}


GPON_LINK_STATE_MACHINE_T gpon_sm_transition_LinkDown_to_LinkUp(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    //add veip interface
    ret = gpon_config_veip_interface(gpon_sm_ctrl);
    if(ret == ANSC_STATUS_SUCCESS)
    {
        return GSM_LINK_UP;
    }

    return GSM_LINK_DOWN;
}

GPON_LINK_STATE_MACHINE_T gpon_sm_transition_LinkUp_to_LinkDown(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    //disable veip interface
    ret = gpon_disable_veip_interface(gpon_sm_ctrl);
    if(ret == ANSC_STATUS_SUCCESS)
    {
        return GSM_LINK_DOWN;
    }

    return GSM_LINK_UP;
}

GPON_LINK_STATE_MACHINE_T gpon_sm_transition_Exit(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    gpon_sm_ctrl->sm_running = false;

    return GSM_LINK_DOWN;
}


///////////////////////////////////////////////////
////////////// OPERATIONAL STATE    ///////////////
///////////////////////////////////////////////////
ANSC_STATUS GSM_Operational_State_Handler(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    return ret;
}

///////////////////////////////////////////////////
///////////////////  STATES    ////////////////////
///////////////////////////////////////////////////
GPON_LINK_STATE_MACHINE_T Gpon_Link_Down_State(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    //check Veip Enabled
    ret = check_gpon_veip_interface_enabled(gpon_sm_ctrl);
    if(ret == ANSC_STATUS_SUCCESS)
    {
        //check Veip Interface Up
        ret = check_gpon_veip_interface_up(gpon_sm_ctrl);
        if(ret == ANSC_STATUS_SUCCESS)
        {
            return  gpon_sm_transition_LinkDown_to_LinkUp(gpon_sm_ctrl);
        }

    }
    else
    {
        return gpon_sm_transition_Exit(gpon_sm_ctrl);
    }

    return GSM_LINK_DOWN;
}


GPON_LINK_STATE_MACHINE_T Gpon_Link_Up_State(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    //check Veip Enabled
    ret = check_gpon_veip_interface_enabled(gpon_sm_ctrl);
    if(ret != ANSC_STATUS_SUCCESS)
    {
        return  gpon_sm_transition_LinkUp_to_LinkDown(gpon_sm_ctrl);
    }

    //check Veip Interface
    ret = check_gpon_veip_interface_up(gpon_sm_ctrl);
    if(ret != ANSC_STATUS_SUCCESS)
    {
        return  gpon_sm_transition_LinkUp_to_LinkDown(gpon_sm_ctrl);
    }

    //Operational State
    ret = GSM_Operational_State_Handler(gpon_sm_ctrl);
    if(ret != ANSC_STATUS_SUCCESS)
    {
        return  gpon_sm_transition_LinkUp_to_LinkDown(gpon_sm_ctrl);
    }

    return GSM_LINK_UP;
}


///////////////////////////////////////////////////
///////////////////  HANDLER    ///////////////////
///////////////////////////////////////////////////
GPON_LINK_SM_CTRL_T* GponMgr_Link_SM_Init(void)
{
    GPON_LINK_SM_CTRL_T* gpon_sm_ctrl = NULL;
    
    //Init GPON SM ctrl
    gpon_sm_ctrl = (GPON_LINK_SM_CTRL_T*) AnscAllocateMemory(sizeof(GPON_LINK_SM_CTRL_T));
    if(gpon_sm_ctrl != NULL)
    {
        // initialise variables
        gpon_controller_init(gpon_sm_ctrl);
    }

    return gpon_sm_ctrl;
}


void GponMgr_Link_SM_Cleanup(GPON_LINK_SM_CTRL_T* gpon_sm_ctrl)
{
    
    //update Veip Ctrl 
    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        DML_VEIP_CTRL_T* pGponCtrl = pGponDmlData->gpon.Veip.pdata[gpon_sm_ctrl->veip_hal_index - 1];
        if(pGponCtrl!= NULL)
        {
            pGponCtrl->sm_created = FALSE;
        }

        //release GPON data
        GponMgrDml_GetData_release(gpon_sm_ctrl->pGponData);
    }


    //Delete GPON Ctrl
    AnscFreeMemory((ANSC_HANDLE) gpon_sm_ctrl);    
}




///////////////////////////////////////////////////
////////////////  STATE MACHINE    ////////////////
///////////////////////////////////////////////////
void* GponMgr_Link_SM_Thread(void *arg)
{
    GPON_LINK_SM_CTRL_T* gpon_sm_ctrl = NULL;
    // event handler
    int sel = 0;
    struct timeval tv;

    //detach thread from caller stack
    pthread_detach(pthread_self());

    CcspTraceInfo(("%s %d - Enter GPON state machine\n", __FUNCTION__, __LINE__));

    gpon_sm_ctrl = (GPON_LINK_SM_CTRL_T*)arg;
    if(gpon_sm_ctrl != NULL)
    {
        //Start
        gpon_sm_ctrl->sm_state = gpon_sm_transition_Start(gpon_sm_ctrl);

        while (gpon_sm_ctrl->sm_running == TRUE)
        {
            /* Wait up to 500 milliseconds */
            tv.tv_sec = 0;
            tv.tv_usec = GPON_LINK_SM_LOOP_TIMEOUT;

            sel = select(0, NULL, NULL, NULL, &tv);
            if (sel < 0) {
                /* interrupted by signal or something, continue */
                continue;
            }


            gpon_sm_ctrl->pGponData = GponMgrDml_GetData_locked();
            if(gpon_sm_ctrl->pGponData != NULL)
            {
                // process state
                switch (gpon_sm_ctrl->sm_state)
                {
                    case GSM_LINK_DOWN:
                        gpon_sm_ctrl->sm_state = Gpon_Link_Down_State(gpon_sm_ctrl);
                        break;
                        break;
                    case GSM_LINK_UP:
                        gpon_sm_ctrl->sm_state = Gpon_Link_Up_State(gpon_sm_ctrl);
                        break;
                    default:
                        CcspTraceInfo(("%s %d - Case: default (%d) \n", __FUNCTION__, __LINE__, gpon_sm_ctrl->sm_state));
                        gpon_sm_ctrl->sm_running = FALSE;
                }

                //release GPON data
                GponMgrDml_GetData_release(gpon_sm_ctrl->pGponData);
            }
        }
        
        //Finalize GPON SM Ctrl
        GponMgr_Link_SM_Cleanup(gpon_sm_ctrl);
    }

    // Exit thread
    CcspTraceInfo(("%s %d - Exit from state machine\n", __FUNCTION__, __LINE__));
    pthread_exit(NULL);

    return NULL;
}


ANSC_STATUS GponMgr_Link_StateMachine_Start(DML_VEIP* pGponVeip)
{
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    GPON_LINK_SM_CTRL_T* gpon_sm_ctrl = NULL;
    pthread_t StateMachineThread;
    int iErrorCode = 0;

    if(pGponVeip == NULL)
    {
        CcspTraceInfo(("%s %d Error: Veip interface is invalid. \n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }

    //Init GPON SM ctrl
    gpon_sm_ctrl = GponMgr_Link_SM_Init();
    if(gpon_sm_ctrl == NULL)
    {
        CcspTraceInfo(("%s %d Error: Failed to allocate memory. \n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }

    //store veip index
    gpon_sm_ctrl->veip_hal_index = pGponVeip->uInstanceNumber;

    //Starts gpon thread
    iErrorCode = pthread_create( &StateMachineThread, NULL, &GponMgr_Link_SM_Thread, (void*) gpon_sm_ctrl);
    if( 0 != iErrorCode )
    {
        CcspTraceInfo(("%s %d Error: Failed to start State Machine Thread error code: %d \n", __FUNCTION__, __LINE__, iErrorCode ));
    }

    return returnStatus;
}
