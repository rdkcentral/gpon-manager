/**
 * @file gponmgr_xpon_rbus.h
 * @brief RBUS connection for the BBF Device.XPON provider.
 */

#ifndef GPONMGR_XPON_RBUS_H
#define GPONMGR_XPON_RBUS_H

/**
 * @brief Open the "GponManager.XPON" RBUS handle and register Device.XPON.
 *
 * Safe to call again once initialized. On failure, nothing stays registered.
 *
 * @return RBUS_ERROR_SUCCESS or an RBUS error code.
 */
int GponMgrXponRbus_Init(void);

/**
 * @brief Unregister Device.XPON and close the RBUS handle. Safe if not initialized.
 */
void GponMgrXponRbus_Close(void);

/**
 * @brief Mark all cached Device.XPON HAL data stale so the next get reads the HAL.
 *
 * Implemented in tr181/gponmgr_xpon_tr181.c; declared here because this header
 * has no RBUS dependency. Call on ONU state changes. Safe with the DML lock held.
 */
void GponMgrXponTr181_InvalidateCache(void);

#endif
