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

#ifndef  _GPONMGR_DML_DATA_H
#define  _GPONMGR_DML_DATA_H

#include <stdbool.h>

#include "ansc_platform.h"
#include "ansc_string_util.h"
#include "gpon_apis.h"
#include "gponmgr_controller.h"


#define GPON_DATA_MAX_PM            2
#define GPON_DATA_MAX_GEM           32
#define GPON_DATA_MAX_VEIP          2





typedef  struct
_DML_PHY_MEDIA_CTRL_
{
    DML_PHY_MEDIA               dml;
    bool                        updated;
}
DML_PHY_MEDIA_CTRL_T;

typedef  struct
_DML_PHY_MEDIA_LIST_
{
    UINT                    ulQuantity;
    DML_PHY_MEDIA_CTRL_T*   pdata[GPON_DATA_MAX_PM];
}
DML_PHY_MEDIA_LIST_T;


typedef  struct
_DML_GEM_CTRL_
{
    DML_GEM     dml;
    bool        updated;
}
DML_GEM_CTRL_T;


typedef  struct
_DML_GEM_LIST_
{
    UINT                ulQuantity;
    DML_GEM_CTRL_T*     pdata[GPON_DATA_MAX_GEM];
}
DML_GEM_LIST_T;


typedef  struct
_DML_VEIP_CTRL_
{
    DML_VEIP    dml;
    bool        sm_created;
    bool        updated;
}
DML_VEIP_CTRL_T;

typedef  struct
_DML_VEIP_LIST_
{
    UINT                ulQuantity;
    DML_VEIP_CTRL_T*    pdata[GPON_DATA_MAX_VEIP];
}
DML_VEIP_LIST_T;




typedef  struct
_DML_X_RDK_GPON
{
    DML_PHY_MEDIA_LIST_T    PhysicalMedia;
    DML_GTC                 Gtc;
    DML_PLOAM               Ploam;
    DML_OMCI                Omci;
    DML_GEM_LIST_T          Gem;               
    DML_VEIP_LIST_T         Veip;
    DML_TR69                Tr69;
}
DML_X_RDK_GPON;


typedef  struct
_GPON_DML_DATA_
{    
    //GPON device data
    DML_X_RDK_GPON_DEVICE   device;

    //GPON data
    DML_X_RDK_GPON          gpon;
    
    //mutex
    pthread_mutex_t         mDataMutex;         
}
GPON_DML_DATA;



GPON_DML_DATA* GponMgrDml_GetData_locked(void);
void GponMgrDml_GetData_release(GPON_DML_DATA* pDmlData);


GPON_DML_DATA* GponMgrDml_GetData(void);
ANSC_STATUS GponMgrDml_DataInit(void);
ANSC_STATUS GponMgrDml_DataDelete(void);

void GponMgrDml_SetDefaultData(GPON_DML_DATA* pGponData);
ANSC_STATUS GponMgrDml_addPhysicalMedia(DML_PHY_MEDIA_LIST_T* gponPhyList, int pm_index);
ANSC_STATUS GponMgrDml_addGem(DML_GEM_LIST_T* gponGemList, int gem_index);
ANSC_STATUS GponMgrDml_addVeip(DML_VEIP_LIST_T* gponVeipList, int veip_index);
ULONG GponMgrDml_GetMonotonicSeconds(void);



#endif  //_GPONMGR_DML_DATA_H