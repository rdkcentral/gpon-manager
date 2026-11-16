/**
 * @file gponmgr_xpon_tr181.h
 * @brief BBF TR-181 Device.XPON data model served over RBUS.
 */

#ifndef GPONMGR_XPON_TR181_H
#define GPONMGR_XPON_TR181_H

#include <rbus.h>

/**
 * @brief Register all Device.XPON elements and table sync handlers.
 * @param handle Open RBUS handle.
 * @return RBUS_ERROR_SUCCESS or an RBUS error code.
 */
int GponMgrXponTr181_Init(rbusHandle_t handle);

/**
 * @brief Stop serving Device.XPON requests. Safe if not initialized.
 */
void GponMgrXponTr181_Close(void);

#endif
