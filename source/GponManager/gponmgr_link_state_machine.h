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

#ifndef  _GPON_MGR_LINK_STATE_MACHINE_H
#define  _GPON_MGR_LINK_STATE_MACHINE_H

#include "ansc_platform.h"
#include "ansc_string_util.h"
#include "gponmgr_controller.h"
#include "gponmgr_dml_data.h"



/* SM states */
typedef enum
{
    GSM_LINK_DOWN = 0,
    GSM_LINK_UP
}
GPON_LINK_STATE_MACHINE_T;



typedef  struct
_GPON_LINK_SM_CTRL_
{
    int                         veip_hal_index;
    int                         veip_eth_instance;
    char                        veip_lower_layer[256];
    bool                        sm_running;
    GPON_LINK_STATE_MACHINE_T   sm_state;
    GPON_DML_DATA*              pGponData;
    bool                        publish_failure_reported;
}
GPON_LINK_SM_CTRL_T;



ANSC_STATUS GponMgr_Link_StateMachine_Start(DML_VEIP* pGponVeip);


#endif  //_GPON_MGR_LINK_STATE_MACHINE_H