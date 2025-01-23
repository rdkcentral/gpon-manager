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

#include "gponmgr_controller.h"
#include "gponmgr_link_state_machine.h"
#include "gponmgr_dml_hal.h"




ANSC_STATUS GponMgr_subscribe_hal_events(void)
{
    ANSC_STATUS ret = ANSC_STATUS_FAILURE;
    char strName[JSON_MAX_STR_ARR_SIZE]={0};
    ULONG idx = 0;

    GPON_DML_DATA* pGponDmlData = GponMgrDml_GetData_locked();
    if(pGponDmlData != NULL)
    {
        //PM
        for(idx = 0; idx < pGponDmlData->gpon.PhysicalMedia.ulQuantity; idx++)
        {
            DML_PHY_MEDIA_CTRL_T* pGponData = pGponDmlData->gpon.PhysicalMedia.pdata[idx];
            if(pGponData!= NULL)
            {
                DML_PHY_MEDIA* pGponPhy = &(pGponData->dml);

                sprintf(strName, GPON_HAL_PM_STATUS, pGponPhy->uInstanceNumber);

                ret = GponHal_Event_Subscribe(&eventcb_PhysicalMediaStatus, strName, JSON_SUBSCRIBE_ON_CHANGE);
                if(ret != ANSC_STATUS_SUCCESS)
                {
                    CcspTraceError(("%s %d - Failed to subscribe to %s\n", __FUNCTION__, __LINE__, strName ));
                    break;
                }

                sprintf(strName, GPON_HAL_PM_ALARM, pGponPhy->uInstanceNumber);
                ret = GponHal_Event_Subscribe(&eventcb_PhysicalMediaAlarmsAll, strName, JSON_SUBSCRIBE_ON_CHANGE);
                if(ret != ANSC_STATUS_SUCCESS)
                {
                    CcspTraceError(("%s %d - Failed to subscribe to %s\n", __FUNCTION__, __LINE__, strName ));
                    break;
                }
            }
        }

        //Veip
        if( ret == ANSC_STATUS_SUCCESS)
        {
            for(idx = 1; idx <= GPON_DATA_MAX_VEIP; idx++)
            {
                sprintf(strName, GPON_HAL_VEIP_ADMIN_STATE, idx);
                ret = GponHal_Event_Subscribe(&eventcb_VeipAdministrativeState, strName, JSON_SUBSCRIBE_ON_CHANGE);
                if(ret != ANSC_STATUS_SUCCESS)
                {
                    CcspTraceError(("%s %d - Failed to subscribe to %s\n", __FUNCTION__, __LINE__, strName ));
                    break;
                }

                sprintf(strName, GPON_HAL_VEIP_OPER_STATE, idx);
                ret = GponHal_Event_Subscribe(&eventcb_VeipOperationalState, strName, JSON_SUBSCRIBE_ON_CHANGE);
                if(ret != ANSC_STATUS_SUCCESS)
                {
                    CcspTraceError(("%s %d - Failed to subscribe to %s\n", __FUNCTION__, __LINE__, strName ));
                    break;
                }
            }
        }

        //Ploam
        if( ret == ANSC_STATUS_SUCCESS)
        {

            sprintf(strName, GPON_HAL_PLOAM_REG_STATE);
            ret = GponHal_Event_Subscribe(&eventcb_PloamRegistrationState, strName, JSON_SUBSCRIBE_ON_CHANGE);
            if(ret != ANSC_STATUS_SUCCESS)
            {
                CcspTraceError(("%s %d - Failed to subscribe to %s\n", __FUNCTION__, __LINE__, strName ));
            }
        }


        GponMgrDml_GetData_release(pGponDmlData);
    }

    return ret;
}


ANSC_STATUS GponMgr_send_hal_configuration(void)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    /******* Send initial config ********/
    ret = GponHal_send_config();

    return ret;
}



ANSC_STATUS GponMgr_Controller_Init()
{
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;

    //subscribe to events
    returnStatus = GponMgr_subscribe_hal_events();
    if(returnStatus != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d - Error in HAL subscribe events\n", __FUNCTION__, __LINE__ ));
        return returnStatus;
    }

    returnStatus = GponMgr_send_hal_configuration();
    if(returnStatus != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d - Error in sending HAL configuration\n", __FUNCTION__, __LINE__ ));
        return returnStatus;
    }

    return returnStatus;
}
