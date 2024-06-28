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

#include "gpon_apis.h"
#include "gponmgr_dml_obj.h"
#include "gponmgr_dml_backendmgr.h"
#include "poam_irepfo_interface.h"
#include "sys_definitions.h"
#include "gponmgr_dml_hal.h"
#include "gponmgr_dml_data.h"
#include "gponmgr_dml_internal.h"

extern void * g_pDslhDmlAgent;

/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE GponMgrDml_create();

    description:

        This function constructs gpon object and return handle.

    argument:

    return:     newly created gpon object.

**********************************************************************/

ANSC_HANDLE GponMgrDml_create ( VOID )
{
    PDATAMODEL_GPON_SERVICES    pMyObject    = (PDATAMODEL_GPON_SERVICES)NULL;

    /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PDATAMODEL_GPON_SERVICES)AnscAllocateMemory(sizeof(DATAMODEL_GPON_SERVICES));
    if ( !pMyObject )
    {
        return  (ANSC_HANDLE)NULL;
    }


    pMyObject->data = GponMgrDml_GetData();
    if ( !pMyObject->data )
    {
        return  (ANSC_HANDLE)NULL;
    }

    /*
     * Initialize the common variables and functions for a container object.
     */
    pMyObject->Create            = GponMgrDml_create;
    pMyObject->Remove            = GponMgrDml_Remove;
    pMyObject->Initialize        = GponMgrDml_Initialize;

    //Initiliaze DML data
    pMyObject->Initialize((ANSC_HANDLE)pMyObject);

    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        GponMgrDml_Initialize(ANSC_HANDLE hThisObject);

    description:

        This function initiate  cosa gpon object and return handle.

    argument:	ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS GponMgrDml_Initialize ( ANSC_HANDLE hThisObject )
{
    ANSC_STATUS                 returnStatus    = ANSC_STATUS_SUCCESS;

    //Initialise dml data handler
    returnStatus = GponMgrDml_DataInit();
    if(returnStatus != ANSC_STATUS_SUCCESS)
    {
        return returnStatus;
    }

    //Wait HAL initilisation
    returnStatus = GponHal_Init();
    if(returnStatus != ANSC_STATUS_SUCCESS)
    {
        return returnStatus;
    }

    //Get HAL data
    returnStatus = GponHal_get_init_data();
    if(returnStatus != ANSC_STATUS_SUCCESS)
    {
        return returnStatus;
    }

    // Initialise GPON MANAGER control
    returnStatus = GponMgr_InternalControlInit();
    if(returnStatus != ANSC_STATUS_SUCCESS)
    {
        return returnStatus;
    }

    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        GponMgrDml_Remove(ANSC_HANDLE hThisObject);

    description:

        This function initiate  gpon object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS GponMgrDml_Remove ( ANSC_HANDLE hThisObject )
{

    ANSC_STATUS                 returnStatus    = ANSC_STATUS_SUCCESS;
    PDATAMODEL_GPON_SERVICES    pMyObject       = (PDATAMODEL_GPON_SERVICES)hThisObject;

    //delete DML data
    GponMgrDml_DataDelete();

    AnscFreeMemory((ANSC_HANDLE)pMyObject);

    return returnStatus;
}
