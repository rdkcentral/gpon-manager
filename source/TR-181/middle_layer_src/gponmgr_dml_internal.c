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

#include "gponmgr_dml_internal.h"


ANSC_STATUS GponMgr_InternalControlInit(void)
{
    ANSC_STATUS returnStatus    = ANSC_STATUS_SUCCESS;


    /**
     * Initialise state machine.
     */
    if(GponMgr_Controller_Init() != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d Error: GponMgr_Controller_Init failed\n", __FUNCTION__, __LINE__ ));
    }


    return returnStatus;
}
