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

#ifndef __GPONMGR_DML_HAL_PARAM_H__
#define __GPONMGR_DML_HAL_PARAM_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_hal_common.h"
#include "json_hal_client.h"

#include "gpon_apis.h"
#include "gponmgr_dml_obj.h"

#define MAX_STR_LEN 256

//Tagged specific information
#define IEEE_802_1Q_ETHERTYPE_STR_SINGLE   "Single"
#define IEEE_802_1Q_ETHERTYPE_STR_DOUBLE   "Double"
#define IEEE_802_1Q_ETHERTYPE_STR_UNTAGGED "Untagged"

ANSC_STATUS Map_hal_dml_pm(DML_PHY_MEDIA_LIST_T* gponPhyList, char* ParamName, char* pValue);
ANSC_STATUS Map_hal_dml_gtc(DML_GTC* gponGtc,char* ParamName, char* pValue);
ANSC_STATUS Map_hal_dml_ploam(DML_PLOAM* gponPloam, char* ParamName, char* pValue);
ANSC_STATUS Map_hal_dml_omci(DML_OMCI* gponOmci, char* ParamName, char* pValue);
ANSC_STATUS Map_hal_dml_gem(DML_GEM_LIST_T* gponGemList,char* ParamName, char* pValue);
ANSC_STATUS Map_hal_dml_veip(DML_VEIP_LIST_T* gponVeipList, char* ParamName, char* pValue);
ANSC_STATUS Map_hal_dml_tr69(PDML_TR69 gponTr69,char * ParamName, char * pValue);



ANSC_STATUS gpon_hal_get_pm_index(char* ParamName, int* index);
ANSC_STATUS gpon_hal_get_gem_index(char* ParamName, int* index);
ANSC_STATUS gpon_hal_get_veip_index(char* ParamName, int* index);

#endif //__GPONMGR_DML_HAL_PARAM_H__
